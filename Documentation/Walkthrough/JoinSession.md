# Join Session

The user chose a session, by selecting amongst the SessionSearchResult available.

Let's see how we make the user's client join the distant game session.

In terms of implementation, we already saw how one may connect to a distant session in the [create game session lambda](CreateSession.md#create-session-lambda) and [start session response method](CreateSession.md#start-session-response-method) chapter, but we will still go through the solution here as they are some minor differences.

## Table Of Contents

- [Join Session](#join-session)
  - [Table Of Contents](#table-of-contents)
  - [The solution's architecture](#the-solutions-architecture)
  - [Find Session Request method](#find-session-request-method)
  - [Join Session Lambda](#join-session-lambda)
  - [Join Session Response method](#join-session-response-method)
  - [Conclusion](#conclusion)

## The solution's architecture

This is exactly the same as for Game Session's architecture, please refer yourself to [this chapter](CreateSession.md#understand-the-solutions-architecture) if you have not seen it before.

## Find Session Request method

The objective here is to create a POST request and send the GameSessionId to create the PlayerSessionId on.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L624) is what that looks like

```cpp
bool FOnlineSessionAWS::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	uint32 Return = ONLINE_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);

	// Don't join a session if already in one or hosting one
	if (LANSession != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Local Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}
	else if (Session != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Distant Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}
	else
	{
		// if LAN session, fall back on nullsubsystem
		if (DesiredSession.IsValid() && DesiredSession.Session.SessionSettings.bIsLANMatch)
		{
			return AWSSubsystem->GetLANSessionInterface()->JoinSession(PlayerNum, SessionName, DesiredSession);
		}

		// Create a named session from the search result data, all info should be already in it, as it is the reponsability of find sessions.
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		//Get a Player Id for AWS Player Session
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// Create Internet or LAN match, the port and address willl be recovered by the htt prequest that we will send below
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// turn off advertising on Join, to avoid clients advertising it over LAN
		Session->SessionSettings.bShouldAdvertise = false;

		/* this is where this is very different between LAN and AWS, HTTP requests comes in place */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;

		/* The actual request object, on client it is equivalent to starting the session, but on server it is a create request. */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* the actual content of the request */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		//The only thing this requests really needs to give, is the GameSessionId
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString());//to check compatibility with a server

		/* the object that will format the json request */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* adding our content and sending the request to the server */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		JoinSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
	}

	if (Return != ONLINE_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ONLINE_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}
```

The method is quite long but it actually only does thing we've seen before.

It creates a session on the client's side (for the OSS) ...

```cpp
        // Create a named session from the search result data, all info should be already in it, as it is the reponsability of find sessions.
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		//Get a Player Id for AWS Player Session
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// Create Internet or LAN match, the port and address willl be recovered by the htt prequest that we will send below
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// turn off advertising on Join, to avoid clients advertising it over LAN
		Session->SessionSettings.bShouldAdvertise = false;

		/* this is where this is very different between LAN and AWS, HTTP requests comes in place */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;
```

... then makes the http requests.

```cpp
		/* The actual request object, on client it is equivalent to starting the session, but on server it is a create request. */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* the actual content of the request */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		//The only thing this requests really needs to give, is the GameSessionId
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString());//to check compatibility with a server

		/* the object that will format the json request */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* adding our content and sending the request to the server */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		JoinSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
```

So it actually combines [CreateSession](CreateSession.md#create-session-method) and [Start Session](CreateSession.md#start-session-request-method) in one method.

Compared to Start Session, we need less thing to create a PlayerSessionId, so we will only send relevent data for it, which is:

- GameSessionId
- UUId of the User (not required)

Let's see how to use those in the lambdas

## Join Session Lambda

The only difference between [Anywhere SDK](../../Plugins/AWSOSS/SAM/join_session/app.py) and [Legacy SDK](../../Plugins/AWSOSS/SAM/join_session_local/app.py) this time is whether gamelift has the url_endpoint defined or not.

Basically it is between this ...
```py
#the gamelift interface (you can define your own endpoint here, a normal endpoint would be: https://gamelift.ap-northeast-1.amazonaws.com)
game_lift = boto3.client("gamelift")
```
and this.
```py
#the gamelift interface (you can define your own endpoint here, a normal endpoint would be: https://gamelift.ap-northeast-1.amazonaws.com, but we're using a local address for our use case to use Gamelift Local)
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

For the actual lambda, it looks like this

```py
# this is the entry point of the join_session lambda.this one is made to work with local Gamelift (SDK4.0)
def lambda_handler(event, context):
	
	print("in join_session local")

	# getting back the post request's body
	body = json.loads(event["body"])
		
	print("sending back player session for", body["GameSessionId"], ".\n")
		
	# we succeeded at creating our game_session, making our player session and sending it as response
	return get_player_session_from_game_session(body["GameSessionId"],body)
```

we have already seen the get_player_session_from_game_session function in the [lambda chapter to create a game session](CreateSession.md#create-session-lambda), so I will not develop it further.
It does exaclty the same, with the only difference being the GameSessionId comes from the event body.

Now for the response on client side.

## Join Session Response method

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L705) is the method.

```cpp

void FOnlineSessionAWS::OnJoinSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();

	//getting back exactly the same data as create game session, as we get back the same thing, connection point to the player session
	FString IpAdress	= responseObject->GetStringField(TEXT("IpAddress"));
	FString Port		= responseObject->GetStringField(TEXT("Port"));
	FString SessionId	= responseObject->GetStringField(TEXT("PlayerSessionId"));

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
	gameSession->SessionState = EOnlineSessionState::InProgress;


	TriggerOnJoinSessionCompleteDelegates(NAME_GameSession, (Result == ONLINE_SUCCESS) ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
}
```

It does almost exactly the same as the [StartSession response method](CreateSession.md#start-session-response-method).

The only difference is the callback.

The same as before, now it is the responsability of user to add the necessarry code in the callback to actually be able to connect to the server, using ClientTravel.

Once again, there is an [example in the GameInstance](../../Source/Private/TestAWSGameInstance.cpp#L288) for such an implementation.

```cpp
void UTestAWSGameInstance::TravelToJoinSession(FName sessionName, EOnJoinSessionCompleteResult::Type joinResult)
{
	if (joinResult == EOnJoinSessionCompleteResult::Success)
	{
		const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();

		FString travelURL;
		FNamedOnlineSession* namedSession = sessions->GetNamedSession(sessionName);
		if (GetPrimaryPlayerController() && !namedSession->SessionSettings.bIsLANMatch && sessions->GetResolvedConnectString(sessionName, travelURL))
		{
			GetPrimaryPlayerController()->ClientTravel(travelURL, ETravelType::TRAVEL_Absolute);
		}
	}
}
```


## Conclusion

This concludes our walkthrough !

Using these methods, you should be able to

- Create a Session from a client
- Find said Session from another client
- Join the found Session

Similarly from what you can do with a the Online Subsystem Null.

The implementation is not completely tested on every platform, and still lacks a lot features that could be interesting to implement (going from basic security features to entire matchmaking).

But this was out of scope for this as this is already hefty, and I still wanted this to be somewhat understandable and readable.

If you want to go further, don't hesitate to use my [Reference Note](../References.md), as there is a lot of great resources to learn about this, and these was only my meager contribution to it.

As for further implementation, I will escape by leaving it as "exercise for the reader".

Hoping it will help in your project, or learning.

Have fun !
