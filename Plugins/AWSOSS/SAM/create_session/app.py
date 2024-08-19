# for https requests
import json
# for env variables
import os
# basically, python aws toolkit
import boto3
# for error and exceptions handling of aws calls
import botocore
# to make a wait on the lambda's thread
import time

# this is the AWS_REGION environment variable,
# it should change depending on the server's this lambda is executed on.
# (eg. : I have one server in Ireland, and one in Osaka. 
# My user calling for a gqme_session server should be redirected to the closest server 
# and get back eu-west-1 if in europe, and ap-northeast-3 if in japan)
my_region = os.environ['AWS_REGION']

#the gamelift interface (you can define your own endpoint here, a normal endpoint would be: https://gamelift.ap-northeast-1.amazonaws.com
game_lift = boto3.client("gamelift")

# this is the entry point of the create_session lambda. this one is made to work with gamelift anywhere or nomal gamelift.
def lambda_handler(event, context):
	
	print("in create_session")

	# getting back the post request's body
	body = json.loads(event["body"])

	# the final response of this request
	response = {}
	
	# the fleet id that we'll create our game session in
	my_fleet_id = get_my_fleet_id(body,response)
	

	# error on getting the fleet id, give up and send error response
	if response:
		return response
		
	print("get fleet id succeeded: ", my_fleet_id, " now trying to repurpose existing game_session.\n")

	location = game_lift.list_compute(FleetId = my_fleet_id)["ComputeList"][0]["Location"]

	print("get location result: ", location)
	
	# what we're trying to do bnelow is what should be done to preserve resource, but on local gamelift (SDK 4 or earlier at least), search game_session does not work.
	
	## trying to find an existing game session in our fleet that we could rpurpes to suit our needs (instead of creating a new resource)
	#my_game_session = repurpose_available_game_session(my_fleet_id,location,body,response)
	#
	## error on getting an old game session, give up and send error response
	#if response:
	#	return response
	#
	## we were successful in repurposing a game session ! sending it back to client as response
	## at this point client receives connection information, but is not connected, it is client's role to ask for a connection afterwards.
	#if my_game_session:
	#	return get_player_session_from_game_session(my_game_session["GameSessionsId"],body)
	#	
	#print("unsuccessful in reuprposing existing game session, trying to make a new one.\n")
	
	# if we could not find a game session to repurpose, last resort, we'll make a new one.
	my_game_session = create_game_session(my_fleet_id,location,body,response)
	
	# error an creating new game session, give up and send error response
	if response:
		return response
		
	print("successful in creating game session, sending player session.\n")
		
	# we succeeded at creating our game_session, making our player session and sending it as response
	return get_player_session_from_game_session(my_game_session["GameSessionId"],body)

#returns first fleet id of current region or 0 in case of error, with error response filled up.
def get_my_fleet_id(body,response):
	# in our example, we only use a single fleet in our case, so we just need to retrieve said fleet
	#(do not forget to set it up beforehand, you could also replace this code with the fleetid directly.
	# cf. https://github.com/aws-samples/amazon-gamelift-unreal-engine/blob/main/lambda/GameLiftUnreal-StartGameLiftSession.py)
	try :
		# ir could be interesting to implement the "find build id" for a final release with real servers, but for local testing, this will be a problem.
		# list_fleet_response = game_lift.list_fleets(BuildId=body["build_id"],Limit=1)
		list_fleet_response = game_lift.list_fleets(Limit=1)
		
		# we'll just get the first one that suits our need.
		return list_fleet_response["FleetIds"][0]
		
	except botocore.exceptions.ClientError as error:
		#we'll leave AWS describe the problem themselves
		print(error.response["Error"]["Message"])
		# 500 to indicate server error, and we'll explain what went wrong 
		#(on official release it may be better to not disclose, but for development purpose, this is invaluable)
		response = {
		"statusCode": 500,
		"body": json.dumps(error.response["Error"]["Message"], default = raw_converter).encode("UTF-8")
		}
		raise error
	
# finding a game session wit no one else in it, and repurpose it with the client's session info
def repurpose_available_game_session(fleet_id, location, body, response):
	# trying to find a game session in the game fleet that could be rupurposed to our needs.
	# here the problem is that we are looking for a game session with no players anymore, 
	# but there can be a race between two players creating a game session at the same time.
	# this need to be accounted for
	game_sessions = game_lift.search_game_sessions(FleetId = fleet_id, Location = location, FilterExpression = "playerSessionCount=0 AND hasAvailablePlayerSessions=true")
	
	# were we fortunate enough to find one?
	if (len(game_sessions['GameSessions']) != 0):

		print("found game sessions, trying to repurpose them")

		# lucky us, there was a game session doing nothing, 
		# we'll use the first one in the list, as any of it would work.
		game_session = game_sessions['GameSessions'][0]
		
		# we don't want to take an activating game session, at it 100% belongs to someone else, 
		# but we can't search for another, because we can't exclude activating sessions from search, 
		# so we'll just create another one
		if is_game_session_activating(game_session, response):
			print("game session was activating, give up on repurposing")
			return {}
		
		try :
			# now, we need to repurpose it with the value the request gave
			game_session = game_lift.update_game_session(GameSessionId = game_session["GameSessionId"], MaximumPlayerSessionCount = body["num_connections"], Name = body["session_name"])["GameSession"]
			
			# it worked ! we're good to go, return the repurposed game session, and get on with the connection process.
			return game_session
			
		except botocore.exceptions.ClientError as error:
			
			# this is how we check the update works, and that we were not beat in some kind of race, a conflict exception is created in that case
			if error["Error"]["Code"] == GameLift.Client.exceptions.ConflictException:
				# compared to the other exceptions, in this one we'll just say that, 
				# "we didn't find a game session to repurpose", 
				# and proceed to try to find another one as this exception is more that someone else took it rather that we could not find one.
				
				# recursive call to see if we can't find another game session we could repurpose.
				# there should be no risk of infinite loop, as this conflict exception should be raised only if there is the same empty session changed by two lambdas
				# but if this one lost, that means it has been changed afterwards, and in the next search, it should not popup.
				return repurpose_available_game_session(fleet_id,body,response)
				# ^ THIS IS VERY BAD, THIS SHOULD BE CHANGED AND IS NOT RECOMMENDED IN LAMBDA BEST PRACTICES
			else:
				#we'll leave AWS describe the problem themselves
				print(error.response["Error"]["Message"])
				# 500 to indicate server error, and we'll explain what went wrong 
				#(on official release it may be better to not disclose, but for development purpose, this is invaluable)
				response = {
				"statusCode": 500,
				"body": json.dumps(error.response["Error"]["Message"], default = raw_converter).encode("UTF-8")
				}
				raise error
		
	return {}
	
def create_game_session(fleet_id, location, body, response):
	# the game session we will return
	game_session = {}

	try :
		# trying to create a game session on the fleet we found
		game_session = game_lift.create_game_session(FleetId = fleet_id, Location = location, MaximumPlayerSessionCount = body["num_connections"], Name = body["session_name"])['GameSession']
		
		# when creating a game session, it takes time between being created and being usable we'll wait until it can be used
		game_session_activating = True
		while (game_session_activating):
		
			# checking game activation
			game_session_activating = is_game_session_activating(game_session, response)
			
			print("game_session is activating.\n")

			# error happened, returning
			if response:
				break
				
			# we'll sleep 10 ms between describe calls
			time.sleep(0.01)
		
		# it is not activating anymore, so it can be used, we'll return it
		print("game session is activated.\n")
  
	except botocore.exceptions.ClientError as error:

		print("ERROR\n")
		# we'll leave AWS describe the problem themselves
		print(error.response["Error"]["Message"])
		# 500 to indicate server error, and we'll explain what went wrong 
		#(on official release it may be better to not disclose, but for development purpose, this is invaluable)
		response = {
		"statusCode": 500,
		"body": json.dumps(error.response["Error"]["Message"], default = raw_converter).encode("UTF-8")
		}
		raise error

	return game_session
	
def get_player_session_from_game_session(game_session_id, body):
	try :
		#trying to create a player session, an interface between the player and the server
		player_session = game_lift.create_player_sessions(GameSessionId = game_session_id, PlayerIds = [ body["uuid"] ])
		
		# get the the connection info (which is what player_sessions are), in json format
		connection_info = json.dumps(player_session["PlayerSessions"][0], default = raw_converter).encode("UTF-8")

		print(connection_info)

		# respond with the connection info if succeeded
		return {
		"statusCode": 200,
		"body" : connection_info
		}
		
	except botocore.exceptions.ClientError as error:
		#we'll leave AWS describe the problem themselves
		print(error.response["Error"]["Message"])
		# 500 to indicate server error, and we'll explain what went wrong 
		#(on official release it may be better to not disclose, but for development purpose, this is invaluable)
		return {
		"statusCode": 500,
		"body": json.dumps(error.response["Error"]["Message"], default = raw_converter).encode("UTF-8")
		}

def is_game_session_activating(game_session,response):
	game_session_status = "ACTIVATING"
	try:
		# getting infos on the game session
		session_details = game_lift.describe_game_sessions(GameSessionId = game_session['GameSessionId'])
		game_session_status = session_details['GameSessions'][0]['Status']
	except botocore.exceptions.ClientError as error:
		# we'll leave AWS describe the problem themselves
		print(error.response["Error"]["Message"])
		# 500 to indicate server error, and we'll explain what went wrong 
		#(on official release it may be better to not disclose, but for development purpose, this is invaluable)
		response = {
		"statusCode": 500,
		"body": json.dumps(error.response["Error"]["Message"], default = raw_converter).encode("UTF-8")
		}
		raise error
	return game_session_status == "ACTIVATING"

def raw_converter(obj):
	return obj.__str__()