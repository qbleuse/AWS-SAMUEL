
#include "OnlineSessionInterfaceAWS.h"
#include "Misc/Guid.h"

/* Kismet */
#include "Kismet/GameplayStatics.h"

/* Online */
#include "OnlineSubsystem.h"
#include "OnlineSubsystemAWS.h"
#include "OnlineSubsystemAWSTypes.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineAsyncTaskManager.h"

/* SubSystem */
#include "SocketSubsystem.h"

/* Http */
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"

/* Json */
#include "Json.h"

/*========== HttpRequest define ==========*/

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


/*========== FOnlineSessionInfoAWS ==========*/

FOnlineSessionInfoAWS::FOnlineSessionInfoAWS() :
	HostAddr(nullptr),
	SessionId(TEXT("INVALID"))
{
}

void FOnlineSessionInfoAWS::Init(const FOnlineSubsystemAWS& Subsystem, const FString& IpAddress, const FString& Port, const FString& PlayerSessionId)
{
	bool dummy;

	/* create an net address if it is not already initialized */
	if (!HostAddr.IsValid())
		HostAddr = MakeShared<FInternetAddrAWS>();

	HostAddr->SetIp(*IpAddress, dummy);
	HostAddr->SetPort(FCString::Atoi(*Port));
	
	SessionId = FUniqueNetIdAWS(PlayerSessionId);
}

/*========== END FOnlineSessionInfoAWS ==========*/

/*========== FOnlineSessionAWS ==========*/

FOnlineSessionAWS::FOnlineSessionAWS(FOnlineSubsystemAWS* InSubsystem) :
	AWSSubsystem(InSubsystem),
	CurrentSessionSearch(nullptr),
	SessionSearchStartInSeconds(0)
{
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("StartSessionURI"), StartSessionURI, GEngineIni);
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("FindSessionURI"), FindSessionURI, GEngineIni);
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("JoinSessionURI"), JoinSessionURI, GEngineIni);


	IOnlineSessionPtr nullSession = InSubsystem->GetLANSessionInterface();

	nullSession->OnCancelFindSessionsCompleteDelegates.AddLambda([this](bool succeeded) { TriggerOnCancelFindSessionsCompleteDelegates(succeeded); });
	nullSession->OnCancelMatchmakingCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnCancelMatchmakingCompleteDelegates(name, succeeded); });
	nullSession->OnCreateSessionCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnCreateSessionCompleteDelegates(name, succeeded); });
	nullSession->OnDestroySessionCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnDestroySessionCompleteDelegates(name, succeeded); });
	nullSession->OnEndSessionCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnEndSessionCompleteDelegates(name, succeeded); });
	if (nullSession->OnFindFriendSessionCompleteDelegates)
		nullSession->OnFindFriendSessionCompleteDelegates->AddLambda([this](int32 localUserNb, bool succeeded, const TArray<FOnlineSessionSearchResult>& result) { TriggerOnFindFriendSessionCompleteDelegates(localUserNb, succeeded, result); });
	nullSession->OnFindSessionsCompleteDelegates.AddLambda([this](bool succeeded) { TriggerOnFindSessionsCompleteDelegates(succeeded); });
	nullSession->OnJoinSessionCompleteDelegates.AddLambda([this](FName name, EOnJoinSessionCompleteResult::Type result) { TriggerOnJoinSessionCompleteDelegates(name, result); });
	nullSession->OnMatchmakingCompleteDelegates.AddLambda([this](FName name, bool success) { TriggerOnMatchmakingCompleteDelegates(name, success); });
	nullSession->OnPingSearchResultsCompleteDelegates.AddLambda([this](bool success) { TriggerOnPingSearchResultsCompleteDelegates(success); });
	nullSession->OnQosDataRequestedDelegates.AddLambda([this](FName name) { TriggerOnQosDataRequestedDelegates(name); });
	nullSession->OnRegisterPlayersCompleteDelegates.AddLambda([this](FName name, const TArray< FUniqueNetIdRef >& PlayersArray, bool succeeded) { TriggerOnRegisterPlayersCompleteDelegates(name, PlayersArray, succeeded); });
	nullSession->OnSessionCustomDataChangedDelegates.AddLambda([this](FName name, const FOnlineSessionSettings& SessionSettings) { TriggerOnSessionCustomDataChangedDelegates(name, SessionSettings); });
	nullSession->OnSessionFailureDelegates.AddLambda([this](const FUniqueNetId& netId, ESessionFailure::Type type) { TriggerOnSessionFailureDelegates(netId, type); });
	nullSession->OnSessionInviteReceivedDelegates.AddLambda([this](const FUniqueNetId& UserId, const FUniqueNetId& FromId, const FString& AppId, const FOnlineSessionSearchResult& InviteResult) { TriggerOnSessionInviteReceivedDelegates(UserId,FromId,AppId,InviteResult); });
	nullSession->OnSessionParticipantRemovedDelegates.AddLambda([this](FName name, const FUniqueNetId& netId) { TriggerOnSessionParticipantRemovedDelegates(name, netId); });
	nullSession->OnSessionParticipantsChangeDelegates.AddLambda([this](FName name, const FUniqueNetId& netId, bool succeeds) { TriggerOnSessionParticipantsChangeDelegates(name, netId, succeeds); });
	nullSession->OnSessionParticipantSettingsUpdatedDelegates.AddLambda([this](FName name, const FUniqueNetId& netId, const FOnlineSessionSettings& SessionSettings) { TriggerOnSessionParticipantSettingsUpdatedDelegates(name, netId, SessionSettings); });
	nullSession->OnSessionSettingsUpdatedDelegates.AddLambda([this](FName name, const FOnlineSessionSettings& SessionSettings) { TriggerOnSessionSettingsUpdatedDelegates(name, SessionSettings); });
	nullSession->OnSessionUserInviteAcceptedDelegates.AddLambda([this](const bool bWasSuccessful, const int32 ControllerId, FUniqueNetIdPtr UserId, const FOnlineSessionSearchResult& InviteResult) { TriggerOnSessionUserInviteAcceptedDelegates(bWasSuccessful, ControllerId, UserId, InviteResult); });
	nullSession->OnStartSessionCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnStartSessionCompleteDelegates(name, succeeded); });
	nullSession->OnUnregisterPlayersCompleteDelegates.AddLambda([this](FName name, const TArray< FUniqueNetIdRef >& netId, bool succeeded) { TriggerOnUnregisterPlayersCompleteDelegates(name, netId, succeeded); });
	nullSession->OnUpdateSessionCompleteDelegates.AddLambda([this](FName name, bool succeeded) { TriggerOnUpdateSessionCompleteDelegates(name, succeeded); });

}

void FOnlineSessionAWS::Tick(float DeltaTime)
{
	/* are we requesting session */
	if (CurrentSessionSearch.IsValid() && !CurrentSessionSearch->bIsLanQuery && CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		FindSessionRequestTimer += DeltaTime;

		/* if we timeout of the request, we just set the request as failing, even if the response comes after, so that the user can send another request. */
		if (FindSessionRequestTimer > FindSessionRequestTimeout)
		{
			// compared to the LAN session when it will get back all sessions on local network for its given time (5s),
			// and timeout is just stop searching, in our case, it may work like that on the server, nut not locally so we still need a response
			// so this timeout is indeed a request fail as we never got back our response.
			CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
			TriggerOnFindSessionsCompleteDelegates(false);

			//reset to enable new search.
			CurrentSessionSearch = nullptr;
		}
	}
}

FNamedOnlineSession* FOnlineSessionAWS::GetNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			return &Sessions[SearchIndex];
		}
	}
	return AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
}

void FOnlineSessionAWS::RemoveNamedSession(FName SessionName)
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			Sessions.RemoveAtSwap(SearchIndex);
			return;
		}
	}

	/* if did not return it should be here */
	AWSSubsystem->GetLANSessionInterface()->RemoveNamedSession(SessionName);
}

EOnlineSessionState::Type FOnlineSessionAWS::GetSessionState(FName SessionName) const
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionName == SessionName)
		{
			return Sessions[SearchIndex].SessionState;
		}
	}

	return AWSSubsystem->GetLANSessionInterface()->GetSessionState(SessionName);
}

bool FOnlineSessionAWS::HasPresenceSession()
{
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SearchIndex = 0; SearchIndex < Sessions.Num(); SearchIndex++)
	{
		if (Sessions[SearchIndex].SessionSettings.bUsesPresence)
		{
			return true;
		}
	}

	return AWSSubsystem->GetLANSessionInterface()->HasPresenceSession();
}

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

bool FOnlineSessionAWS::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	/* this is expected to be called only when not already connected, so this works */
	return CreateSession(0, SessionName, NewSessionSettings);
}

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

void FOnlineSessionAWS::OnStartSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();
	
	/* verify that response is valid */
	//if (!responseObject->HasField(TEXT("PlayerSessions")))
	//{
	//	UE_LOG_ONLINE(Warning, TEXT("HTTP request %s responded with unvalid data (no available). Dump of response:\n%s\n\nElapsed Time: %f"), *Request->GetURL(), *Response->GetContentAsString(), //Request->GetElapsedTime());
	//
	//	return;
	//}

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


bool FOnlineSessionAWS::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	bool bWasSuccessful = false;

	// Grab the session information by name
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
	{
		bWasSuccessful = AWSSubsystem->GetLANSessionInterface()->UpdateSession(SessionName, UpdatedSessionSettings, bShouldRefreshOnlineData);
	}
	else
	{
		/** as the session is something created and handled locally on each client to represent AWS and not something that is actually used, 
		 * update does not make sense*/
		UE_LOG_ONLINE_UC(Error, TEXT("Update of distant online game is not supported. called for session (%s)"), *SessionName.ToString());
	}

	return bWasSuccessful;
}

bool FOnlineSessionAWS::EndSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
	{
		return AWSSubsystem->GetLANSessionInterface()->EndSession(SessionName);
	}
	else if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EOnlineSessionState::InProgress)
		{
			/** this is kind of irrelevant in the idea of AWS, 
			 * we're doing this purely for if session are used out of this context */
			Session->SessionState = EOnlineSessionState::Ended;
			Result = ONLINE_SUCCESS;

			//TODO: REALLY END SESSION
		}
		else
		{
			UE_LOG_ONLINE_UC(Warning, TEXT("Can't end distant session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Can't end a distant online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}

		TriggerOnEndSessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}

bool FOnlineSessionAWS::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	uint32 Result = ONLINE_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->DestroySession(SessionName, CompletionDelegate);
	else if (Session)
	{
		// The session info is no longer needed
		RemoveNamedSession(Session->SessionName);

		/* it is instantaneous as no call is needed for gamelift to destroy session */
		Result = ONLINE_SUCCESS;

		//TODO: REALLY DESTROY SESSION ON AWS
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Can't destroy a null online session (%s)"), *SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		CompletionDelegate.ExecuteIfBound(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
		TriggerOnDestroySessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}

bool FOnlineSessionAWS::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionAWS::StartMatchmaking(const TArray< FUniqueNetIdRef >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE_UC(Warning, TEXT("StartMatchmaking is not supported on this platform. Use FindSessions or FindSessionById."));
	TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionAWS::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	UE_LOG_ONLINE_UC(Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionAWS::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG_ONLINE_UC(Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionAWS::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	uint32 Return = ONLINE_FAIL;

	if (SearchSettings->bIsLanQuery)
	{
		Return = AWSSubsystem->GetLANSessionInterface()->FindSessions(SearchingPlayerNum, SearchSettings) ? ONLINE_SUCCESS : Return;
	}// Don't start another search while one is in progress
	else if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// Free up previous results
		SearchSettings->SearchResults.Empty();

		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;

		// remember the time at which we started search, as this will be used for a "good enough" ping estimation
		SessionSearchStartInSeconds = FPlatformTime::Seconds();

		// reseting the timeout timer
		FindSessionRequestTimer = 0.0f;

		Return = ONLINE_IO_PENDING;

		if (Return == ONLINE_IO_PENDING)
		{
			SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;

			IAWSHttpRequest FindSessionRequest = AWSSubsystem->MakeRequest(FindSessionURI, TEXT("GET"));
			FindSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnFindSessionsResponseReceived);
			FindSessionRequest->ProcessRequest();

		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Ignoring game search request while one is pending"));
		Return = ONLINE_IO_PENDING;
	}


	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}

void FOnlineSessionAWS::OnFindSessionsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();
	
	/** this can happen in two case: 
	* - a response of a previous request came after being timed out, we still ignore it as we considered it failed
	* - a response of this request came after timeing out, same case we ignore it. */
	if (CurrentSessionSearch->SearchState != EOnlineAsyncTaskState::InProgress)
		return;

	//We got back sessions without errors so we consider we're ok with the result
	//if there is no game session, it is actually a plausible conclusion, there is just no one with a game session right now
	Result = ONLINE_SUCCESS;

	//compute our ping approximate, at least better than nothing
	int32 VeryBadPingApproximate = FPlatformTime::Seconds() - SessionSearchStartInSeconds;

	TArray<TSharedPtr<FJsonValue>> GameSessions = response->AsArray();
	for (int32 i = 0; i < GameSessions.Num(); i++)
	{
		//Getting the game sessions as object for easier time getting the data we want
		TSharedPtr<FJsonObject> GameSession = GameSessions[i]->AsObject();

		//The struct used to fill our AWS game session data
		FOnlineSessionSearchResult SearchResult;

		SearchResult.PingInMs = VeryBadPingApproximate;

		// we did not implement private or public game sessions for the moment, so we'll just fill both for both to work the same
		{
			// max player sessions fill
			SearchResult.Session.SessionSettings.NumPublicConnections = GameSession->GetNumberField(TEXT("MaximumPlayerSessionCount"));
			SearchResult.Session.SessionSettings.NumPrivateConnections = SearchResult.Session.SessionSettings.NumPublicConnections;

			//currently available player sessions
			SearchResult.Session.NumOpenPublicConnections = SearchResult.Session.SessionSettings.NumPublicConnections - GameSession->GetNumberField(TEXT("CurrentPlayerSessionCount"));
			SearchResult.Session.NumOpenPrivateConnections = SearchResult.Session.NumOpenPublicConnections;
		}

		//Set the user friendly name, chosen by the game session's owner
		SearchResult.Session.SessionSettings.Set(TEXT("Name"), GameSession->GetStringField("Name"));
		//Set the Game Session Id, in case user will want to join this session (this should bne opaque to user and nerver be seen, but we still need to save it somewhere)
		SearchResult.Session.SessionSettings.Set(TEXT("GameSessionId"), GameSession->GetStringField("GameSessionId"));

		CurrentSessionSearch->SearchResults.Add(SearchResult);
	}
	
	CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;
	TriggerOnFindSessionsCompleteDelegates((Result == ONLINE_SUCCESS) ? true : false);
}

bool FOnlineSessionAWS::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	/* this is expected to be called only when not already connected, so this works */
	// This function doesn't use the SearchingPlayerNum parameter, so passing in anything is fine.
	return FindSessions(0, SearchSettings);
}

bool FOnlineSessionAWS::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

bool FOnlineSessionAWS::CancelFindSessions()
{
	uint32 Return = ONLINE_FAIL;
	if (CurrentSessionSearch.IsValid() && CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		Return = ONLINE_SUCCESS;

		//FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		CurrentSessionSearch = NULL;
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return != ONLINE_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}

	Return = AWSSubsystem->GetLANSessionInterface()->CancelFindSessions() ? ONLINE_SUCCESS : Return;

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}

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

bool FOnlineSessionAWS::JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	// Assuming player 0 should be OK here
	return JoinSession(0, SessionName, DesiredSession);
}

bool FOnlineSessionAWS::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResult);
	UE_LOG_ONLINE_UC(Warning, TEXT("FindFriendSession is not supported on this platform. Use FindSessions or FindSessionById."));
	return false;
};

bool FOnlineSessionAWS::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	UE_LOG_ONLINE_UC(Warning, TEXT("FindFriendSession is not supported on this platform. Use FindSessions or FindSessionById."));
	return false;
}

bool FOnlineSessionAWS::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<FUniqueNetIdRef>& FriendList)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	UE_LOG_ONLINE_UC(Warning, TEXT("FindFriendSession is not supported on this platform. Use FindSessions or FindSessionById."));
	return false;
}

bool FOnlineSessionAWS::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	UE_LOG_ONLINE_UC(Warning, TEXT("SendSessionInviteToFriend is not supported on this platform. Implement Friend interface if you wish to use it."));
	return false;
};

bool FOnlineSessionAWS::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	UE_LOG_ONLINE_UC(Warning, TEXT("SendSessionInviteToFriend is not supported on this platform. Implement Friend interface if you wish to use it."));

	return false;
}

bool FOnlineSessionAWS::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< FUniqueNetIdRef >& Friends)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	UE_LOG_ONLINE_UC(Warning, TEXT("SendSessionInviteToFriend is not supported on this platform. Implement Friend interface if you wish to use it."));
	return false;
};

bool FOnlineSessionAWS::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< FUniqueNetIdRef >& Friends)
{
	// this function has to exist due to interface definition, but we did not implement friend interface so we cannot change it
	UE_LOG_ONLINE_UC(Warning, TEXT("SendSessionInviteToFriend is not supported on this platform. Implement Friend interface if you wish to use it."));
	return false;
}

bool FOnlineSessionAWS::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG_ONLINE_UC(Warning, TEXT("PingSearchResults is not implemented."));
	return false;
}

/** Get a resolved connection string from a session info */
static bool GetConnectStringFromSessionInfo(TSharedPtr<FOnlineSessionInfoAWS>& SessionInfo, FString& ConnectInfo, int32 PortOverride = 0)
{
	bool bSuccess = false;
	if (SessionInfo.IsValid())
	{
		if (SessionInfo->HostAddr.IsValid())
		{
			if (PortOverride != 0)
			{
				ConnectInfo = FString::Printf(TEXT("%s:%d"), *SessionInfo->HostAddr->ToString(false), PortOverride);
			}
			else
			{
				ConnectInfo = FString::Printf(TEXT("%s"), *SessionInfo->HostAddr->ToString(true));
			}

			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FOnlineSessionAWS::GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType)
{
	bool bSuccess = false;
	// Find the session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->GetResolvedConnectString(SessionName, ConnectInfo, PortType);
	else if (Session != NULL)
	{
		TSharedPtr<FOnlineSessionInfoAWS> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoAWS>(Session->SessionInfo);
		if (PortType == NAME_BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(Session->SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);
		}
		else if (PortType == NAME_GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}

		if (!bSuccess)
		{
			UE_LOG_ONLINE_UC(Warning, TEXT("Invalid distant session info for session %s in GetResolvedConnectString()"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning,
			TEXT("Unknown distant session name (%s) specified to GetResolvedConnectString()"),
			*SessionName.ToString());
	}

	return bSuccess;
}

bool FOnlineSessionAWS::GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	bool bSuccess = false;
	if (SearchResult.Session.SessionInfo.IsValid())
	{
		TSharedPtr<FOnlineSessionInfoAWS> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoAWS>(SearchResult.Session.SessionInfo);

		if (PortType == NAME_BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(SearchResult.Session.SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);

		}
		else if (PortType == NAME_GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}
	}

	if (!bSuccess || ConnectInfo.IsEmpty())
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("Invalid session info in search result to GetResolvedConnectString()"));
	}

	return bSuccess;
}

FOnlineSessionSettings* FOnlineSessionAWS::GetSessionSettings(FName SessionName)
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return &LANSession->SessionSettings;
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}

bool FOnlineSessionAWS::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	TArray< FUniqueNetIdRef > Players;
	Players.Add(PlayerId.AsShared());
	return RegisterPlayers(SessionName, Players, bWasInvited);
}

bool FOnlineSessionAWS::RegisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players, bool bWasInvited)
{
	bool bSuccess = false;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->RegisterPlayers(SessionName, Players, bWasInvited);
	if (Session)
	{
		bSuccess = true;

		for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			const FUniqueNetIdRef& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
			{
				Session->RegisteredPlayers.Add(PlayerId);
				//RegisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections > 0)
				{
					Session->NumOpenPublicConnections--;
				}
				else if (Session->NumOpenPrivateConnections > 0)
				{
					Session->NumOpenPrivateConnections--;
				}
			}
			else
			{
				//RegisterVoice(*PlayerId);
				UE_LOG_ONLINE_UC(Log, TEXT("Player %s already registered in session %s"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

bool FOnlineSessionAWS::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	TArray< FUniqueNetIdRef > Players;
	Players.Add(PlayerId.AsShared());
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionAWS::UnregisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players)
{
	bool bSuccess = true;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		AWSSubsystem->GetLANSessionInterface()->UnregisterPlayers(SessionName, Players);
	if (Session)
	{
		for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			const FUniqueNetIdRef& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
			if (RegistrantIndex != INDEX_NONE)
			{
				Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
				//UnregisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections < Session->SessionSettings.NumPublicConnections)
				{
					Session->NumOpenPublicConnections++;
				}
				else if (Session->NumOpenPrivateConnections < Session->SessionSettings.NumPrivateConnections)
				{
					Session->NumOpenPrivateConnections++;
				}
			}
			else
			{
				UE_LOG_ONLINE_UC(Warning, TEXT("Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("No game present to leave for session (%s)"), *SessionName.ToString());
		bSuccess = false;
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}


int32 FOnlineSessionAWS::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num() + AWSSubsystem->GetLANSessionInterface()->GetNumSessions();
}

void FOnlineSessionAWS::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (int32 SessionIdx = 0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		DumpNamedSession(&Sessions[SessionIdx]);
	}

	AWSSubsystem->GetLANSessionInterface()->DumpSessionState();
}

void FOnlineSessionAWS::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionAWS::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, true);
}

FUniqueNetIdPtr FOnlineSessionAWS::CreateSessionIdFromString(const FString& SessionIdStr)
{
	FUniqueNetIdPtr SessionId;
	if (!SessionIdStr.IsEmpty())
	{
		SessionId = FUniqueNetIdAWS::Create(SessionIdStr);
	}
	return SessionId;
}