# Create Session

With the Online Subsystem able to send http request, let us see how we can make a system to create a distant game session and connect to it.

## Table of Contents

- [Create Session](#create-session)
  - [Table of Contents](#table-of-contents)
  - [Understand the Solution's Architecture](#understand-the-solutions-architecture)
    - [Gamelift Anywhere Architecture](#gamelift-anywhere-architecture)
    - [Legacy Gamelift Architecture](#legacy-gamelift-architecture)
  - [Create Session method](#create-session-method)
  - [Start Session Request method](#start-session-request-method)
  - [Create Session lambda](#create-session-lambda)
    - [Create Session Anywhere SDK](#create-session-anywhere-sdk)
    - [Create Session legacy SDK](#create-session-legacy-sdk)
  - [Start Session Response method](#start-session-response-method)

## Understand the Solution's Architecture

Firstly, let's review how the solution is architected for both Gamelift Anywhere and Legacy Local Gamelift (further refered as Legacy)

### Gamelift Anywhere Architecture

![gamelift anywhere diagram](../Media/gamelift_anywhere_architecture.png)

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client and <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway's back and forth is done through http requests between the OSS (using the URL's to point to SAM's API Gateway).

2. <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> SAM API Gateway will use Docker to initialize its <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdas and execute them

3. <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdas accessing <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift using the credentials in the AWS CLI

4. <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> Gamelift directly redirecting to <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> local machine's server thanks to [the configuration of gamelift anywhere using the AWS Console and AWS CLI, and with the game server's successful "Gamelift->InitSDK()" call in GameInstance](../Usage/Run.md#build-your-games-server-build).

5. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> server then gets connection to the same machine's  <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client on a different port

The architecture is then fairly cumbersome to setup, but not that difficult to run when everyhting is setup.

What we'll go over is:

- [client's side http request](#start-session-request-method)
- [the lambda execution](#create-session-lambda)
- [the connection between the client and server](#start-session-response-method)

The rest is automatic from the setup.

### Legacy Gamelift Architecture

![legacy gamelift diagram](../Media/legacy_gamelift_architecture.png)

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client and <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway's back and forth is done through http requests between the OSS (using the URL's to point to SAM's API Gateway).

2. <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> SAM API Gateway will use Docker to initialize its <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdas and execute them

3. <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdas accessing <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift by directly pointing to the port on the machine where the legacy gamelift is ran

4. <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> Gamelift directly redirecting to <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> local machine's server thanks to  [the game server's successful "Gamelift->InitSDK()" call in GameInstance](../Usage/Run.md#build-your-games-server-build).

5. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> server then gets connection to the same machine's  <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client on a different port

Compared to Anywhere only 3 and 4 changes.
The one advantage of the legacy implementation is to never get out of the local machine, even if it means having methods you can call.

Setup is also faster, but less close to actual implementation when wanting to ship.

We'll go over the same things as for gamelift anywhere, which are:

- [client's side http request](#start-session-request-method)
- [the lambda execution](#create-session-lambda)
- [the connection between the client and server](#start-session-response-method)

Only lambdas change when using legacy or anywhere solutions.

## Create Session method

Before we can send an http request user need to create a Named Session in the Session Interface to fill with the data we're interested in (like the Max Number of Players).

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L) is the Create Game Session method

```cpp
bool FOnlineSessionAWS::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	uint32 Result = ONLINE_FAIL;

	/* Check for an existing session in both LAN and distant */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (Session != NULL)/* distant check */
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Cannot create session '%s': distant session already exists."), *SessionName.ToString());
	}
	else if (LANSession != NULL)/* LAN check */
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Cannot create session '%s': local session already exists."), *SessionName.ToString());
	}
	else
	{
		if (NewSessionSettings.bIsLANMatch)/* LAN fallback */
		{
			return AWSSubsystem->GetLANSessionInterface()->CreateSession(HostingPlayerNum, SessionName, NewSessionSettings);
		}
		else/* distant session creation, this is kind of a copy of lan creation with very small differences */
		{
			// Create a new session and deep copy the game settings
			Session = AddNamedSession(SessionName, NewSessionSettings);
			check(Session);
			Session->SessionState = EOnlineSessionState::Creating;

			/* in our situation, this is excepcted to be change by receiving */
			Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
			Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections;	// always start with full public connections, local player will register later

			Session->HostingPlayerNum = HostingPlayerNum;

			check(AWSSubsystem);
			IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
			if (Identity.IsValid())
			{
				Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
				Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
			}

			// if did not get a valid one, just use something
			if (!Session->OwningUserId.IsValid())
			{
				Session->OwningUserId = FUniqueNetIdAWS::Create(FString::Printf(TEXT("%d"), HostingPlayerNum));
				if (!NewSessionSettings.Get(TEXT("Name"), Session->OwningUserName))
					Session->OwningUserName = FString(TEXT("NullUser"));
			}

			// Unique identifier of this build for compatibility
			Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

			// Setup the host session info
			FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
			//NewSessionInfo->Init(*NullSubsystem);
			Session->SessionInfo = MakeShareable(NewSessionInfo);

			/* technically for us with AWS, creating the session is just creating the pseudo struct 
			 * that represents the Interface between AWS and the Client on Unreal's side, so nothing is really done at creation step */
			Session->SessionState = EOnlineSessionState::Pending;
			Result = ONLINE_SUCCESS;
		}
	}

	if (Result != ONLINE_IO_PENDING)
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_IO_PENDING || Result == ONLINE_SUCCESS;
}
```

We don't do anything as when creating this session, while it has not started, the session is still in "writable mode" where user can change the data inside.

This is why we wait for the erquest to start the session before actually creating the session on server's side.

Actually, very few datas are actually supported by this implementation:

- Max Number of Players
- Name of the Session's Owner
- Id of the Session's Owner
- the client's build id

That's about it. but it can be expanded if needed.
The one thing that's important is to send it in the http request afterward in Start Session, for all players to get it back.

## Start Session Request method

Once the session is set, user calls start to create the distant game session and connects himself to it

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L265) is what launching an http request looks like

```cpp
bool FOnlineSessionAWS::StartSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;
	/* Grab the session information by name */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->StartSession(SessionName);
	else if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			/* this is where this is very different between LAN and AWS, HTTP requests comes in place */
			Session->SessionState = EOnlineSessionState::Starting;
			Result = ONLINE_IO_PENDING;

			/* The actual request object, on client it is equivalent to starting the session, but on server it is a create request. */
			IAWSHttpRequest StartSessionRequest = AWSSubsystem->MakeRequest(StartSessionURI, TEXT("POST"));

			/* the actual content of the request */
			FString JsonRequest;
			TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();
			
			FString SessionName;
			if (Session->SessionSettings.Get<FString>(TEXT("Name"), SessionName))
				JsonRequestObject->SetStringField(TEXT("session_name"), SessionName);// the name of the session that needs to be created on the server
			else
				JsonRequestObject->SetStringField(TEXT("session_name"), FString::Printf(TEXT("%s's Session"), *UKismetSystemLibrary::GetPlatformUserName()));//if we can't get back the name, a generic one.
			JsonRequestObject->SetNumberField(TEXT("build_id"), Session->SessionSettings.BuildUniqueId);//to check compaticility with a server
			JsonRequestObject->SetStringField(TEXT("uuid"), Session->OwningUserId->ToString());//to check compatibility with a server
			/* we won't handle private connections on AWS, and the implmentation makes that there is not Private and Public connections at the same time,
			 * so adding it will always give the right number necessary */
			JsonRequestObject->SetNumberField(TEXT("num_connections"), Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections);
			
			/* the object that will format the json request */
			TSharedRef<TJsonWriter<>> StartSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
			FJsonSerializer::Serialize(JsonRequestObject, StartSessionWriter);

			/* adding our content and sending the request to the server */
			StartSessionRequest->SetContentAsString(JsonRequest);
			StartSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnStartSessionResponseReceived);
			StartSessionRequest->ProcessRequest();
		}
		else
		{
			UE_LOG_ONLINE_UC(Warning, TEXT("Can't start a distant online session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Can't start a distant online game for session (%s) that hasn't been created"), *SessionName.ToString());
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}
```

Let's go through it step by step

```cpp
bool FOnlineSessionAWS::StartSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;
	/* Grab the session information by name */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->StartSession(SessionName);
	else if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			/* this is where this is very different between LAN and AWS, HTTP requests comes in place */
			Session->SessionState = EOnlineSessionState::Starting;
			Result = ONLINE_IO_PENDING;

			/* The actual request object, on client it is equivalent to starting the session, but on server it is a create request. */
			IAWSHttpRequest StartSessionRequest = AWSSubsystem->MakeRequest(StartSessionURI, TEXT("POST"));

```

First check if it is a LAN session and fallback if it is, and also check if we actually can start the session.

If we can, we create the body of an http request.
We choose a POST request, as we need to send the session's data to Gamel;ift for it to create the proper game session.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L61) is how we create the http requests body

```cpp
IAWSHttpRequest FOnlineSubsystemAWS::MakeRequest(const FString& RequestURI, const FString& Verb)
{
	IAWSHttpRequest newRequest = HTTPModule->CreateRequest();

	/* empty URI would not make any sense in this context */
	if (!ensureAlwaysMsgf(!RequestURI.IsEmpty(), TEXT("FOnlineSubsystemAWS::MakeRequest was called with an invalid (empty) RequestURI. please make sure your request URIs are set in Engine.ini")))
		return newRequest;

	newRequest->SetURL(APIGatewayURL + RequestURI);
	newRequest->SetHeader("Content-Type", "application/json");

	if (!Verb.IsEmpty())
		newRequest->SetVerb(Verb);

	return newRequest;
}

```

The API Gateway works with multiple content types, but it is fairly common to use json, and as Unreal has a Json formatter library, this is convenient.

[The definition](../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L22) of the http request is fairly simple, it is just a SharedRef for the actual object.

```cpp
/** define Http request type that will be used throughout AWS Online Subsystem pipeline*/
typedef TSharedRef<class IHttpRequest, ESPMode::ThreadSafe> IAWSHttpRequest;
```

Then we just need to fill up the content of our json that will be processed by the lambda (we make the senfding and receiving end, so we have no rules to follow in terms of naming)

```cpp
    /* the actual content of the request */
    FString JsonRequest;
    TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

    FString SessionName;
    if (Session->SessionSettings.Get<FString>(TEXT("Name"), SessionName))
        JsonRequestObject->SetStringField(TEXT("session_name"), SessionName);// the name of the session that needs to be created on the server
    else
        JsonRequestObject->SetStringField(TEXT("session_name"), FString::Printf(TEXT("%s's Session"), *UKismetSystemLibrary::GetPlatformUserName()));//if we can't get back the name, a generic one.

    JsonRequestObject->SetNumberField(TEXT("build_id"), Session->SessionSettings.BuildUniqueId);//to check compaticility with a server
    JsonRequestObject->SetStringField(TEXT("uuid"), Session->OwningUserId->ToString());//to check compatibility with a server

    /* we won't handle private connections on AWS, and the implmentation makes that there is not Private and Public connections at the same time,
    * so adding it will always give the right number necessary */
    JsonRequestObject->SetNumberField(TEXT("num_connections"), Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections);

    /* the object that will format the json request */
    TSharedRef<TJsonWriter<>> StartSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
    FJsonSerializer::Serialize(JsonRequestObject, StartSessionWriter);

    /* adding our content and sending the request to the server */
    StartSessionRequest->SetContentAsString(JsonRequest);
    StartSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnStartSessionResponseReceived);
    StartSessionRequest->ProcessRequest();
```

After filling the content in a Json Object, we format it and send it off in the http request.

We also bind a callback to the response for when API Gateway will respond.

## Create Session lambda

There is two version of this lambda, the [anywhere sdk](#create-session-anywhere-sdk) version and the [legacy sdk](#create-session-legacy-sdk) version.

Let's see both in this order.

### Create Session Anywhere SDK

Lambdas supports multiple languages but I choose python, mainly because that's the only one that I knew.

You can also define your entry point's name in the associated yaml file for the SAM API gateway.

[Here](../../Plugins/AWSOSS/SAM/create_session/app.py#L23) is what the function looks like

```py
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
# My user calling for a game_session server should be redirected to the closest server 
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
	
	# if we could not find a game session to repurpose, last resort, we'll make a new one.
	my_game_session = create_game_session(my_fleet_id,location,body,response)
	
	# error an creating new game session, give up and send error response
	if response:
		return response
		
	print("successful in creating game session, sending player session.\n")
		
	# we succeeded at creating our game_session, making our player session and sending it as response
	return get_player_session_from_game_session(my_game_session["GameSessionId"],body)

```

There's quite a lot to cover !

Firstly, for the import, some you may know, but boto3 and botocore must sound new : this is because this is the python package for talking programatically with AWS Cloud system in python.

As you may see, I take out an interface from boto3, which is the only one that we really need : the gamelift interface.

In this lambda we need to do two things

- create a game session with the data we got from the client
- create a player session for the client to connect to the server

Player Sessions are the only way a client can connect to a server on AWS Gamelift, and is basically the object representing the client's connection to AWS.

Player Session's are created from game sessions, so first we'll create the game session.

To do this we need a fleet to create the game session's on.

[Here](../../Plugins/AWSOSS/SAM/create_session/app.py#L76) is the declaration of get_my_fleet_id

```py
#returns first fleet id of current region or 0 in case of error, with error response filled up.
def get_my_fleet_id(body,response):
	# in our example, we only use a single fleet in our case, so we just need to retrieve said fleet
	#(do not forget to set it up beforehand, you could also replace this code with the fleetid directly.
	# cf. https://github.com/aws-samples/amazon-gamelift-unreal-engine/blob/main/lambda/GameLiftUnreal-StartGameLiftSession.py)
	try :
		# it could be interesting to implement the "find build id" for a final release with real servers, but for local testing, this will be a problem.
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
```

If you configured your AWS Gamelift the same way it is written in the [guide](../Usage/Run.md#set-up-new-sdk-local-gamelift), you should have only one fleet, so just retrieving the first fleet actually works.

To create an anywhere game session you need both fleet_id and the location, so after having both by just looking up for the first one, we can finally try to [create the game session](../../Plugins/AWSOSS/SAM/create_session/app.py#L156).

```py
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
```

We can now create the game session, using the data in the json body, using the fleet id and location we got earlier.

The creation should be straightforward, but we need the server to be active to create the player session, so we need to wait until it is ready.

Here is the declaration of is_game_session_activating.

```py
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
```

Game sessions have status that communicates in what state they are, this is what we are looking for.

Usually on the Game server's side you would change the state by calling something like [this](../../Source/Private/TestAWSGameInstance.cpp#L113)

```cpp
gameLiftSdkModule->ActivateGameSession();
```

Once it is called, the state change on Gamelift's side, allowing us to continue execution.

The downside is that it takes a few second for the Gamelift to send the game session creation method to the server and the server, that is not on the AWS cloud, to answer it back.

If we were to do it properly, different lambda should handle those interaction, but for the sake of simplicity and brevety, we do everything in a single lambda, which needs then a longer timeout than the default three seconds.

Once we have an active game session, we just have to create and give back the player session to the client.

[Here](../../Plugins/AWSOSS/SAM/create_session/app.py#L198) is what get_player_session_from_game_session looks like.

```py
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
```

This is pretty straightforward, we create the player session, and encode it in json to send it as a response.

That's it !

It is a pretty long lambda, which is absolutely not advised, but it successfully creates a games session and a player session for the caller.

### Create Session legacy SDK

The logic is quite the same as the anywhere sdk, but as gamelift is not distant but on the machine, it is slightly different.

[Here](../../Plugins/AWSOSS/SAM/create_session_local/app.py#L19) is the lambda.

```py
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

```

The functions are the same than gamelift anywhere sdk, the two things that need to be talked about is the game_lift interface and the fleet_id.

Because in legacy gamelift sdk, gamelift is simulated by a java script on the machine, fleet actually does not exists, but the method still requires fleet ids.
So as long as your fleet id fits the requirement (basically being called "fleet-[insert numbers here]") you can put pretty much anything.

And for the Gamelift Interface, you genuinely can use the simulated gamelift in your lambda, but you need to make your lambda point to your gamelift on your machine, thus changing the endpoint url.
In this case, the ip address is the equivalent of localhost of the machine when executing a program form inside Docker.
The port can be defined by the user when executing the script.

That's about it, let's see how to use the response in Unreal.

## Start Session Response method

The response method can be found [here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L325)

```cpp
void FOnlineSessionAWS::OnStartSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();

	FString IpAdress  = responseObject->GetStringField(TEXT("IpAddress"));
	FString Port	  = responseObject->GetStringField(TEXT("Port"));
	FString SessionId = responseObject->GetStringField(TEXT("PlayerSessionId"));

	if (IpAdress.IsEmpty() || Port.IsEmpty() || SessionId.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("HTTP request %s responded with unvalid data. Dump of response:\n%s\n\nElapsed Time: %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());

		return;
	}
	
	FNamedOnlineSession* gameSession = GetNamedSession(NAME_GameSession);
	if (FOnlineSessionInfoAWS* SessionInfo = (FOnlineSessionInfoAWS*)(gameSession->SessionInfo.Get()))
	{
		SessionInfo->Init(*AWSSubsystem, IpAdress, Port, SessionId);
		Result = ONLINE_SUCCESS;
	}
	

	TriggerOnStartSessionCompleteDelegates(NAME_GameSession, (Result == ONLINE_SUCCESS) ? true : false);
}
```

Because most method will be about sending and receiving http request, I've made a little macro for error management.

That's what is [ProcessHttpRequest()](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L27)

```cpp
#define ProcessHttpRequest() \
	uint32 Result = ONLINE_FAIL;\
	\
	if (!bConnectedSuccessfully)\
		UE_LOG_ONLINE_UC(Warning, TEXT("Connection for the HTTP request %s did not succeed. Elapsed Time: %f"), *Request->GetURL(), Request->GetElapsedTime());\
	\
	TSharedPtr<FJsonValue> response;\
	TSharedRef<TJsonReader<>> responseReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());\
	if (!FJsonSerializer::Deserialize(responseReader, response))\
		UE_LOG_ONLINE_UC(Warning, TEXT("HTTP request %s reponse was not JSON object as expected. Dump of Response:\n%s\n\nElapsed Time: %f"), *Request->GetURL(), *Response->GetContentAsString(),Request->GetElapsedTime());\
	TSharedPtr<FJsonObject> responseObject = response.IsValid() ? response->AsObject() : nullptr;\
	if (!responseObject.IsValid() || responseObject->HasField(TEXT("error")))\
		UE_LOG_ONLINE_UC(Warning, TEXT("HTTP request %s reponse was not JSON object as expected. Dump of Response:\n%s\n\nElapsed Time: %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());\

```

After it we just get back the data from the response (we know that we get back the connection info), and create a Session Info out of it.

There is no much more than that.

The only thing necessary is for the user to then ask for connection using the session ifo for the client to connect to the server, like it is pictured in the [callback method of Start Session in the Game Instance](../../Source/Private/TestAWSGameInstance.cpp#L303)

```cpp
void UTestAWSGameInstance::JoinAfterStart(FName sessionName, bool startSuceeded)
{
	if (startSuceeded && bShouldJoin)
	{
		const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
		FNamedOnlineSession* namedSession  = sessions->GetNamedSession(sessionName);

		/* crated a session in lan, make it a listen server */
		if (namedSession->bHosting && namedSession->SessionSettings.bIsLANMatch)
		{
			ToListenServer(GetWorld()->GetMapName());
		}
		else
		{
			/* created a session through AWS, need to join the dedicated server created */
			FString travelURL;
			if (GetPrimaryPlayerController()  && sessions->GetResolvedConnectString(sessionName, travelURL))
			{
				GetPrimaryPlayerController()->ClientTravel(travelURL, ETravelType::TRAVEL_Absolute);
			}
		}

		bShouldJoin = false;
	}
}
```

Using GetResolvedConnectString, gives user the ip address and port of the session info, which user can directly use to ClientTravel, which attempts to connect to server.

So basically, Unreal does all the work.

The cool thing about this implementation is that it works for distant connection too.

Doing all this, we were able to create a distant game session on the AWS Cloud and a player session to connect client to the server.

But now, that is a multiplayer session, so we want other player to be able to join in, and for this, you need to be able to find game sessions.
[Let see how to do this](FindSession.md).
