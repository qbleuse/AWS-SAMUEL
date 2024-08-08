# AWS OSS Usage

This chapter expects knowledge in both Unreal's Online SubSystem and AWS Gamelift.

The plugin is a Online SubSystems Plugin, and the features are the same as any other OSS.

The only really implemented feature for this OSS Plugin are Gamelift connection.
This means what changes between a simple interface is the Online Session Interface.

The real method implemented are :
 - Create and Start Session
 - Find Session
 - Join Session
 
The other feature of the Online Session interface are not implemented or partially implemented (match making for example is not supported).

Every other feature of the Online Subsystem will fall back on the OnlineSubSystemNull.

Let us review how those method work and an example on how to use them.

## Create Session / Start Session


The method were implemented to be similar in architecture as creating LAN session with the Null OSS.
Therefore, the way session are created on gamelift are through user request by a client, that associates said client as the owner.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L201) is an example of calling CreateSession and StartSession in a GameInstance

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
```

CreateSession has for only purpose to save the setting of a certain game session locally, in the OSS.
Then if you call Start Session on this created session, the Plugin will create an equivalent game session with client as owner, and will create a player session associated with the client, that it gives back as a connect string in the local game session.

With this Connect String (meaning the url and port of your gamelift server), you may connect to the newly created game session that client is the owner.

[Here](../../Source/Private/TestAWSGameInstance.cpp#L309) is an example of how you would do this in the callback of Start Session.
