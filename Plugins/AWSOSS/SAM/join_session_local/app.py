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

# this is the entry point of the join_session lambda.this one is made to work with local Gamelift (SDK4.0)
def lambda_handler(event, context):
	
	print("in join_session local")

	# getting back the post request's body
	body = json.loads(event["body"])
		
	print("sending back player session for", body["GameSessionId"], ".\n")
		
	# we succeeded at creating our game_session, making our player session and sending it as response
	return get_player_session_from_game_session(body["GameSessionId"],body)
	
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

def raw_converter(obj):
	return obj.__str__()