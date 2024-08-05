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

# this is the entry point of the find_session lambda. this one is made to work with gamelift anywhere or normal gamelift.
def lambda_handler(event, context):
	
	print("in find_session")

	# the final response of this request
	response = {}
	
	# the fleet id that we'll create our game session in
	my_fleet_id = get_my_fleet_id(response)
	
	# error on getting the fleet id, give up and send error response
	if response:
		return response
		
	print("get fleet id succeeded: ", my_fleet_id, "now searching for game sessions")

	location = game_lift.list_compute(FleetId = my_fleet_id)["ComputeList"][0]["Location"]

	print("get location result: ", location)

	# what we're trying to do bnelow is what should be done to preserve resource, but on local gamelift (SDK 4 or earlier at least), search game_session does not work.
	
	# trying to find an existing game session in our fleet
	game_sessions = game_lift.search_game_sessions(FleetId = my_fleet_id, Location = location, FilterExpression = "hasAvailablePlayerSessions=true")

	print("game session found: ", game_sessions.__str__(), " sending them as-is")
		
	# this is a simple GET request, we just need to show to the client what is runnning for the moment.
	# we'll then just sennd this raw answer
	return {
		"statusCode": 200,
		"body" : json.dumps(game_sessions["GameSessions"], default = raw_converter).encode("UTF-8")
		}

#returns first fleet id of current region or 0 in case of error, with error response filled up.
def get_my_fleet_id(response):
	# in our example, we only use a single fleet in our case, so we just need to retrieve said fleet
	#(do not forget to set it up beforehand, you could also replace this code with the fleetid directly.
	# cf. https://github.com/aws-samples/amazon-gamelift-unreal-engine/blob/main/lambda/GameLiftUnreal-StartGameLiftSession.py)
	try :
		# ir could be interesting to implement the "find build id" for a final release with real servers, but for local testing, this won't be a problem.
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
	
	return 0

def raw_converter(obj):
	return obj.__str__()