// Fill out your copyright notice in the Description page of Project SessionSettings.


#include "TestAWSGameInstance.h"

/* Kismet */
#include "Kismet/KismetSystemLibrary.h"

/* Framework */
#include "Framework/Application/SlateApplication.h"

/* Online */
#include "OnlineSubsystemUtils/Public/OnlineSubsystemUtils.h"
#include "GameLiftServerSDK.h"

/* Player/Game State */
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"
#include "Utilities/Macro.h"

DEFINE_LOG_CATEGORY_STATIC(GameServerLog, Log, All);

UTestAWSGameInstance::UTestAWSGameInstance()
{
	/* put in constructor default session settings */

	SessionSettings.NumPublicConnections	= UUndercoverSettings::GetNbOfPlayerInGame();
	SessionSettings.NumPrivateConnections	= 0;
	SessionSettings.bShouldAdvertise		= true;
	SessionSettings.bAllowJoinInProgress	= true;
	SessionSettings.bAllowJoinViaPresence	= true;
	SessionSettings.bIsLANMatch				= true;
	SessionSettings.bIsDedicated			= false;
	SessionSettings.bUsesPresence			= true;
	SessionSettings.bUseLobbiesIfAvailable	= true;
	SessionSettings.bUseLobbiesVoiceChatIfAvailable = false;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = false;
	SessionSettings.bAntiCheatProtected		= false;
	SessionSettings.bUsesStats				= false;

	// These are about the only changes over the standard Create sessions Node
	SessionSettings.bAllowInvites = true;
}

void UTestAWSGameInstance::Init()
{
	Super::Init();

	UE_LOG(GameServerLog, Log, TEXT("Initializing the GameLift Server"));
	//Let's run this code only if GAMELIFT is enabled. Only with Server targets!
#if WITH_GAMELIFT

	UE_LOG(GameServerLog, Log, TEXT("Initializing the GameLift Server"));

	//Getting the module first.
	FGameLiftServerSDKModule* gameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

	//Define the server parameters for a GameLift Anywhere fleet. These are not needed for a GameLift managed EC2 fleet.
	FServerParameters serverParameters;

	//AuthToken returned from the "aws gamelift get-compute-auth-token" API. Note this will expire and require a new call to the API after 15 minutes.
	if (FParse::Value(FCommandLine::Get(), TEXT("-authtoken="), serverParameters.m_authToken))
	{
		UE_LOG(GameServerLog, Log, TEXT("AUTH_TOKEN: %s"), *serverParameters.m_authToken)
	}

	//The Host/compute-name of the GameLift Anywhere instance.
	if (FParse::Value(FCommandLine::Get(), TEXT("-hostid="), serverParameters.m_hostId))
	{
		UE_LOG(GameServerLog, Log, TEXT("HOST_ID: %s"), *serverParameters.m_hostId)
	}

	//The Anywhere Fleet ID.
	if (FParse::Value(FCommandLine::Get(), TEXT("-fleetid="), serverParameters.m_fleetId))
	{
		UE_LOG(GameServerLog, Log, TEXT("FLEET_ID: %s"), *serverParameters.m_fleetId)
	}

	//The WebSocket URL (GameLiftServiceSdkEndpoint).
	if (FParse::Value(FCommandLine::Get(), TEXT("-websocketurl="), serverParameters.m_webSocketUrl))
	{
		UE_LOG(GameServerLog, Log, TEXT("WEBSOCKET_URL: %s"), *serverParameters.m_webSocketUrl)
	}

	//The PID of the running process
	serverParameters.m_processId = FString::Printf(TEXT("%d"), GetCurrentProcessId());
	UE_LOG(GameServerLog, Log, TEXT("PID: %s"), *serverParameters.m_processId);

	//InitSDK establishes a local connection with GameLift's agent to enable communication.
	UE_LOG(GameServerLog, Log, TEXT("Call InitSDK"))

	FGameLiftGenericOutcome outcome = gameLiftSdkModule->InitSDK(serverParameters);

	if (outcome.IsSuccess())
	{
		UE_LOG(GameServerLog, Log, TEXT("GameLift Server Initialization Successful"));
	}
	else
	{
		FGameLiftError error = outcome.GetError();
		UE_LOG(GameServerLog, Log, TEXT("GameLift Server Initialization Failed: Error %s\n%s"), *error.m_errorName, *error.m_errorMessage);
	}


	//Respond to new game session activation request. GameLift sends activation request 
	//to the game server along with a game session object containing game properties 
	//and other settings. Once the game server is ready to receive player connections, 
	//invoke GameLiftServerAPI.ActivateGameSession()
	auto onGameSession = [=](Aws::GameLift::Server::Model::GameSession gameSession)
	{
		FString gameSessionId = FString(gameSession.GetGameSessionId());
		UE_LOG(GameServerLog, Log, TEXT("GameSession Initializing: %s"), *gameSessionId);
		gameLiftSdkModule->ActivateGameSession();
	};

	FProcessParameters* params = new FProcessParameters();
	params->OnStartGameSession.BindLambda(onGameSession);

	//OnProcessTerminate callback. GameLift invokes this before shutting down the instance 
	//that is hosting this game server to give it time to gracefully shut down on its own. 
	//In this example, we simply tell GameLift we are indeed going to shut down.
	params->OnTerminate.BindLambda([=]() 
	{
		UE_LOG(GameServerLog, Log, TEXT("Game Server Process is terminating"));
		gameLiftSdkModule->ProcessEnding(); 
	});

	//HealthCheck callback. GameLift invokes this callback about every 60 seconds. By default, 
	//GameLift API automatically responds 'true'. A game can optionally perform checks on 
	//dependencies and such and report status based on this info. If no response is received  
	//within 60 seconds, health status is recorded as 'false'. 
	//In this example, we're always healthy!
	params->OnHealthCheck.BindLambda([]() {UE_LOG(GameServerLog, Log, TEXT("Performing Health Check")); return true; });

	//Here, the game server tells GameLift what port it is listening on for incoming player 
	//connections. In this example, the port is hardcoded for simplicity. Since active game
	//that are on the same instance must have unique ports, you may want to assign port values
	//from a range, such as:
	const int32 port = FURL::UrlConfig.DefaultPort;
	params->port = port;

	//Here, the game server tells GameLift what set of files to upload when the game session 
	//ends. GameLift uploads everything specified here for the developers to fetch later.
	TArray<FString> logfiles;
	logfiles.Add(TEXT("aLogFile.txt"));
	params->logParameters = logfiles;

	//Call ProcessReady to tell GameLift this game server is ready to receive game sessions!
	outcome = gameLiftSdkModule->ProcessReady(*params);
	if (outcome.IsSuccess())
	{
		UE_LOG(GameServerLog, Log, TEXT("GameLift Server Initialization Successful"));
	}
	else
	{
		FGameLiftError error = outcome.GetError();
		UE_LOG(GameServerLog, Log, TEXT("GameLift Server Initialization Failed: Error %s\n%s"), *error.m_errorName, *error.m_errorMessage);
	}

#else

	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
	sessions->OnJoinSessionCompleteDelegates.AddUObject(this, &UTestAWSGameInstance::TravelToJoinSession);
	sessions->OnStartSessionCompleteDelegates.AddUObject(this, &UTestAWSGameInstance::JoinAfterStart);

	FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this, &UTestAWSGameInstance::OnGameLosesFocus);
#endif
}


void UTestAWSGameInstance::ServerTravel_Implementation(const FString& levelName, const FString& path)
{
	AGameModeBase* gameMode = GetWorld()->GetAuthGameMode();

	if (gameMode->IsValidLowLevel())
	{	
		FString travelURL = path + levelName;
		
		if (gameMode->GetNetMode() & (ENetMode::NM_DedicatedServer | ENetMode::NM_ListenServer))
			travelURL += TEXT("?listen");

		if (gameMode->CanServerTravel(travelURL, false))
		{
			gameMode->ProcessServerTravel(travelURL);
		}
	}
}

void UTestAWSGameInstance::ToListenServer_Implementation(const FString& levelName)
{
	UWorld* currentWorld = GetWorld();
	const FString& string = levelName;
	FURL newURL = FURL(*string);

	currentWorld->Listen(newURL);
}

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

void UTestAWSGameInstance::EndSession_Implementation()
{
	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();

	if (sessions->EndSession(NAME_GameSession))
	{
		sessions->DestroySession(NAME_GameSession);
	}

	bShouldJoin = true;
}


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
