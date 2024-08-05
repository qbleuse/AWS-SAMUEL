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

# a fleet id for local testing (but can be used even in final released if a real fleet id is put here)
global_fleet_id = "fleet-123"

#the gamelift interface (you can define your own endpoint here, a normal endpoint would be: https://gamelift.ap-northeast-1.amazonaws.com, but we're using a local address for our use case to use Gamelift Local)
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')

# this is the entry point of the create_session local lambda. this one is made to work with local Gamelift (SDK4.0)
def lambda_handler(event, context):
	
	print("in create_session_local")

	# getting back the post request's body
	body = json.loads(event["body"])

	# the final response of this request
	response = {}
	
	# as in local gamelift, fleet ids are useless and does not matter, you can litterally put any string that begins by "fleet-"
	# and as lonmg it's the same throughout, you shouldn't have any problem.
	my_fleet_id = global_fleet_id
		
	print("get fleet id succeeded: ", my_fleet_id, " now creating new game_session.\n")
	
	# in gamelift local, the method to repurpose game sessions are not supported, we'll juste make new ones everytime.
	# in the implementation of the game, we destroy the game session when we don't have any player anymore anyway.
	my_game_session = create_game_session(my_fleet_id,body,response)
	
	# error an creating new game session, give up and send error response
	if response:
		return response
		
	print("successful in creating game session, sending player session.\n")
		
	# we succeeded at creating our game_session, making our player session and sending it as response
	return get_player_session_from_game_session(my_game_session["GameSessionId"],body)
	
# creates a game_session on the fleet given in parameter, and waits for it to be activated before returing it
def create_game_session(fleet_id, body, response):
	# the game session we will return
	game_session = {}

	try :
		# trying to create a game session on the fleet we found
		game_session = game_lift.create_game_session(FleetId = fleet_id, MaximumPlayerSessionCount = body["num_connections"], Name = body["session_name"])['GameSession']
		
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

		raise error

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