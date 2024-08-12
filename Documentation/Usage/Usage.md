# AWS OSS Usage

This chapter expects knowledge in both Unreal's Online SubSystem and AWS Gamelift.

The plugin is a Online SubSystems Plugin, and the features are the same as any other OSS.

The only really implemented feature for this OSS Plugin are Gamelift connection.
This means what changes between a simple interface is the Online Session Interface.

The real method implemented are :
	- [Create Session / Start Session](#create-session--start-session)
	- [Find Session](#find-session)
	- [Join Session](#join-session)
 
The other feature of the Online Session interface are not implemented or partially implemented (matchmaking for example is not supported).

Every other feature of the Online Subsystem will fall back on the OnlineSubSystemNull.

Let us review how those method work and an example on how to use them.

## Create Session / Start Session


The method were implemented to be similar in architecture as creating LAN session with the Null OSS.
Therefore, the way session are created on gamelift are through user request by a client, that associates said client as the owner.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L198) is an example of calling CreateSession and StartSession in a GameInstance

```cpp
void UTestAWSGameInstance::CreateSession_Implementation(bool isSessionLan)
{
	/* comes from advanced session plugin,
	 * but we're doing it in cpp rather than blueprint */

	SessionSettings.bIsLANMatch = isSessionLan;
	SessionSettings.bIsDedicated = !isSessionLan;
	
	if (SessionSettings.bIsDedicated)
	{
		SessionSettings.bUsesPresence = false;
		SessionSettings.bUseLobbiesIfAvailable = false;
	}
	else
	{
		SessionSettings.bUsesPresence = true;
		SessionSettings.bUseLobbiesIfAvailable = true;
	}

	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
	const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();

	bool result = false;

	if (!SessionSettings.bIsDedicated)
	{
		if (netId.IsValid())
		{
			result = sessions->CreateSession(*netId, NAME_GameSession, SessionSettings);
		}
	}
	else
		result = sessions->CreateSession(0, NAME_GameSession, SessionSettings);

	if (result)
	{
		FNamedOnlineSession* namedSession = sessions->GetNamedSession(NAME_GameSession);
		namedSession->bHosting = true;
		sessions->StartSession(NAME_GameSession);
	}
}
```

CreateSession has for only purpose to save the setting of a certain game session locally, in the OSS.
Then if you call Start Session on this created session, the Plugin will create an equivalent game session with client as owner, and will create a player session associated with the client, that it gives back as a connect string in the local game session.

With this Connect String (meaning the url and port of your gamelift server), you may connect to the newly created game session that client is the owner.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L309) is an example of how you would do this in the callback of Start Session.

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

The owner will then be the first to connect to the distant server and be regognized as the owner 

## Find Session

The find session method uses the same interface as any OSS, it uses a FOnlineSessionSearch and returns an array of FOnlineSessionSearchResults.

For this, it will request information of all opened Game Session on the associated Gamelift instance.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L253) is an example of what the Find method could look like.

```cpp
FDelegateHandle UTestAWSGameInstance::FindSession(FOnFindSessionsCompleteDelegate& onFoundSessionDelegate, TSharedPtr<FOnlineSessionSearch>& searchResult)
{
	/* comes from advanced session plugin,
	 * but we're doing it in cpp rather than blueprint */

	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
	if (sessions.IsValid())
	{

		FDelegateHandle delegateHandle = sessions->AddOnFindSessionsCompleteDelegate_Handle(onFoundSessionDelegate);

		const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
		sessions->FindSessions(*netId, searchResult.ToSharedRef());

		return delegateHandle;
	}

	return FDelegateHandle();
}
```

Using this you may afterwards use Join Session to connect to said game session.

## Join Session

Join Session request the creation of a player session associated with the game session chosen to connect to it.

The player session is filled up in the game session given by the call back and you may use the ip address and port to connect to the game session.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L273) is an example of calling Join Session:

```cpp
bool UTestAWSGameInstance::JoinSessionAndTravel(const FOnlineSessionSearchResult& sessionToJoin)
{
	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();

	if (sessions.IsValid())
	{
		const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
		if (sessions->JoinSession(*netId, NAME_GameSession, sessionToJoin))
		{
			return sessions->StartSession(NAME_GameSession);
		}
	}

	return false;
}
```


Then, [here](../../Source/Private/TestAWSGameInstance.cpp#L288) is an example of may look like the callback of join session, to afterwards connect to the server.

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

If the ip address and port are opened, Unreal does everything for us to connect to the server.

Now, that you understand how to use it, let us see [how to run it](Run.md).
