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

# this is the entry point of the find_session lambda. this is the local implementation (for SDK4.0).
def lambda_handler(event, context):
	
	print("in find_session")

	# the final response of this request
	response = {}
	
	# the fleet id that we'll create our game session in.
	# in local Gamelift (SDK4.0), it actually does not matter, as long as we have the same throughout.
	my_fleet_id = global_fleet_id
	
	# error on getting the fleet id, give up and send error response
	if response:
		return response
		
	print("get fleet id succeeded: ", my_fleet_id, "now searching for game sessions")
	
	# what we're trying to do bnelow is what should be done to preserve resource, but on local gamelift (SDK 4 or earlier at least), search game_session does not work.
	
	# trying to find an existing game session in our fleet
	# search session does not exist in gamelift local, so we'll use describe game_sessions
	# note that we don't have a filter. this is one problem with this implementation, but it is too small of an environment to really matter.
	game_sessions = game_lift.describe_game_sessions(FleetId = my_fleet_id)

	print("game session found: ", game_sessions.__str__(), " sending them as-is")
		
	# this is a simple GET request, we just need to show to the client what is runnning for the moment.
	# we'll then just sennd this raw answer
	return {
		"statusCode": 200,
		"body" : json.dumps(game_sessions["GameSessions"], default = raw_converter).encode("UTF-8")
		}

def raw_converter(obj):
	return obj.__str__()