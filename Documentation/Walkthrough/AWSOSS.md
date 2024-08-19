# AWS OSS and Http Request

Here, we will dive in how the AWS OSS is built and how it communicates with AWS.

## Table Of Contents

- [AWS OSS and Http Request](#aws-oss-and-http-request)
	- [Table Of Contents](#table-of-contents)
	- [Create an OSS Plugin](#create-an-oss-plugin)
	- [Initializing the Online Subsystem](#initializing-the-online-subsystem)
	- [Initialize the AWS Session Interface](#initialize-the-aws-session-interface)

## Create an OSS Plugin

To create an OSS Plugin, I mostly copied the source code available in the Engine's Plugins, let's look at it step by step.

Firstly, the Online SubSystem is a SubSystem, meaning in the Module definition it should be registered as one.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.h#L6) is the declaration of the Module.

```cpp
class FAWSOSSModule : public IModuleInterface
{
	public:	
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:

		/** Class responsible for creating instance(s) of the subsystem */
		class FOnlineFactoryAWS* AWSOnlineFactory {nullptr};
};
```

The module is a simple one , but in the declaration, it needs the factory to register our OSS in the array of platform available.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.cpp#L13) is what is looks like.

```cpp
class FOnlineFactoryAWS : public IOnlineFactory
{
public:

	FOnlineFactoryAWS() {}
	virtual ~FOnlineFactoryAWS() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemAWSPtr AWSOSS = MakeShared<FOnlineSubsystemAWS, ESPMode::ThreadSafe>(InstanceName);
		if (AWSOSS->IsEnabled())
		{
			if(!AWSOSS->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("Null API failed to initialize!"));
				AWSOSS->Shutdown();
				AWSOSS = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Null API disabled!"));
			AWSOSS->Shutdown();
			AWSOSS = NULL;
		}

		return AWSOSS;
	}
};

void FAWSOSSModule::StartupModule()
{
	AWSOnlineFactory = new FOnlineFactoryAWS();
	
	/* basically, setup a factory for our AWSOSS to be accessible by the Engine */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(AWS_SUBSYSTEM, AWSOnlineFactory);
}

void FAWSOSSModule::ShutdownModule()
{
	/* remove from loaded online subsystem */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(AWS_SUBSYSTEM);
	
	/* and delete */
	delete AWSOnlineFactory;
	AWSOnlineFactory = NULL;
}
```

Note that the factory is newed and deleted, meaning it is not part of the Garbage Collection.

Otherwise, I think it is pretty straightforward ; creating the OSS, initializing it, and registering it if it succeeded.

## Initializing the Online Subsystem

Now, [here](../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L28) is what the Online Subsystem looks like.

```cpp
/**
 *	OnlineSubsystemAWS - Custom Implementation of the online subsystem for AWS
 */
class AWSOSS_API FOnlineSubsystemAWS :
	public FOnlineSubsystemNull
{

	public:
		virtual ~FOnlineSubsystemAWS() = default;
	
	public:
		/** Only the factory makes instances */
		FOnlineSubsystemAWS() = delete;
		explicit FOnlineSubsystemAWS(FName InInstanceName) :
			FOnlineSubsystemNull(InInstanceName)
		{}

		virtual		IOnlineSessionPtr	GetSessionInterface() const override;
		FORCEINLINE IOnlineSessionPtr	GetLANSessionInterface() const { return FOnlineSubsystemNull::GetSessionInterface(); }
		FORCEINLINE class FHttpModule*	GetHTTP()const { return HTTPModule; }
		FORCEINLINE FString				GetAPIGateway()const { return APIGatewayURL; }
		
		/** Creates the body of an http request to the AWS API Gateway request URI with specified verb
		 * RequestURI: the URI of the http request in the AWS API Gateway interface (typically, something similar to "/login")
		 * Verb: the type of request for the specified uri (can be GET or POST, etc...). Won't be specified if left empty.
		 */
		IAWSHttpRequest MakeRequest(const FString& RequestURI, const FString& Verb = TEXT(""));

		virtual bool Init() override;
		virtual bool Tick(float DeltaTime) override;
		virtual bool Shutdown() override;
		virtual FText GetOnlineServiceName() const override;

	protected:
		/* reference to the HTTPModule to send HTTP request to AWS server */
		class FHttpModule* HTTPModule;

		/** AWS's API Gateway is always a https url, it allows for direct access of client to Amazon's services through lambda
		 * we will mainly use this as our way of communicating with AWS as it will be more straightforward and closer in implementation */
		UPROPERTY(EditAnywhere)
		FString APIGatewayURL;

		/** AWS Session Interface that will handle both distant (through AWS) and LAN sessions.
		 * As opposed to what the name would suggest, it does handle both Local and Distant connection 
		 * by having a rollback on LAN implemented. */
		FOnlineSessionAWSPtr DistantSessionInterface;
};
```

For ease of implementation, it inherits from OnlineSubsystemNull so that most other interface inside the OnlineSubsystem just falls back on the Null Interfaces.

The only one that changes is the Session Interface, which we change so that our OnlineSessionAWSInterface is used instead of the Null one.
We still keep the Null one for fallback on it in case the session is for LAN.

The only other members of this class are about the http requests.

Let us see their initialization in the class' [Init method](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L6)

```cpp
bool FOnlineSubsystemAWS::Init()
{
	bool toReturn = FOnlineSubsystemNull::Init();

	HTTPModule = &FHttpModule::Get();
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("APIGatewayURL"), APIGatewayURL, GEngineIni);
	DistantSessionInterface = MakeShareable(new FOnlineSessionAWS(this));

	return toReturn;
}
```

Quite straightforward, the members of the class are being initialized.

Fortunately, to easily send Http Requests, Unreal has an Engine Plugin for that and you just need to add it  to your [.Build.cs](../../Plugins/AWSOSS/Source/AWSOSS.Build.cs) in the DependencyModuleNames.

```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
        "Core",
        "CoreUObject",
        "Engine",
        "CoreUObject",
        "OnlineSubsystem",
        "OnlineSubsystemUtils",
		"OnlineSubsystemNull",
		"GameLiftServerSDK",
		"HTTP",// <-- the line you should look at.
		"Json",
		"Sockets"
      });
```

With this, we'll be able to create, format, and send http requests easily.
And we just need to access to the Module to use it, so we save it as a member of the OSS.

But to send http requets, we need a URI (or URL).
For this, I decided to imitate what the other OSS do when you need to configure stuff inside the OSS : I used to make it configurable inside the [DefaultEngine.ini](../../Config/DefaultEngine.ini) file, inside an "OnlineSubsystemAWS" section.

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL needs to be between quotation marks if it has numbers in it (as it is a URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

That allows for configuration of other URLs in different scenarios, and also is similar to the way you need to confgigure OSS anyway.

As for the URL, the one problem of the implementation is that it uses a single URL for all calls to AWS, whereas in a real implementation, you may want to send a GET request to this first URL to get your own regions's URL, to get better connectivity.

And in this method we also create, and thus, initialize our Session Interface.

## Initialize the AWS Session Interface

The Subsystem inherits from OnlineSubsystemNull, thus has two session interface : AWSSessionInterface and SessionInterfaceNull.

The AWSSessionInterface has then two roles : being able to send http requests with the Online Subsysytem resource if the connection is distant, or to fallback on Session Interface Null.

Let us see the [definition](../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSessionInterfaceAWS.h#L33) for the class.

```cpp
class FOnlineSessionAWS : public IOnlineSession
{
private:

	/** Reference to the main Custom OSS */
	class FOnlineSubsystemAWS* AWSSubsystem;

	/** basically the URI following the API gateway allowing to call the lambda responsible for creating session.
	 * for how you shall implement it in AWS, refer to the AWSImplementationNotice available in the root of this folder.
	 * you can modify this value using FConfigCacheIni in the field OnlineSessionAWS->StartSessionURI */
	FString StartSessionURI;

	/** basically the URI following the API gateway allowing to call the lambda responsible for listing all session.
	 * for how you shall implement it in AWS, refer to the AWSImplementationNotice available in the root of this folder.
	 * you can modify this value using FConfigCacheIni in the field OnlineSessionAWS->FindSessionURI */
	FString FindSessionURI;

	/** basically the URI following the API gateway allowing to call the lambda responsible for listing all session.
	 * for how you shall implement it in AWS, refer to the AWSImplementationNotice available in the root of this folder.
	 * you can modify this value using FConfigCacheIni in the field OnlineSessionAWS->JoinSessionURI */
	FString JoinSessionURI;

PACKAGE_SCOPE:

	/** Critical sections for thread safe operation of session lists */
	mutable FCriticalSection SessionLock;

	/** Current session settings */
	TArray<FNamedOnlineSession> Sessions;

	/** Current search object */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;

	/** Current search start time. */
	double SessionSearchStartInSeconds;

	/** How much time should we wait for a search call before timing out */
	float FindSessionRequestTimeout = 15.0f;

	/** timer variable for the timeout when request is launched */
	float FindSessionRequestTimer = 15.0f;

	FOnlineSessionAWS(class FOnlineSubsystemAWS* InSubsystem);

	// IOnlineSession
	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, SessionSettings);
	}

	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, Session);
	}

	void OnStartSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnFindSessionsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnJoinSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

public:

	virtual ~FOnlineSessionAWS() {}

	virtual FUniqueNetIdPtr CreateSessionIdFromString(const FString& SessionIdStr) override;

	/* for timeout Implementation */
	void Tick(float DeltaTime);

	// Session Getter/Setter
			FNamedOnlineSession*		GetNamedSession(FName SessionName) override;
	virtual void						RemoveNamedSession(FName SessionName) override;
	virtual EOnlineSessionState::Type	GetSessionState(FName SessionName) const override;
	virtual bool						HasPresenceSession() override;

	// IOnlineSession
	virtual bool CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool StartSession(FName SessionName) override;
	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = true) override;
	virtual bool EndSession(FName SessionName) override;
	virtual bool DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate()) override;
	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) override;
	virtual bool StartMatchmaking(const TArray< FUniqueNetIdRef >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName) override;
	virtual bool CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName) override;
	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	virtual bool CancelFindSessions() override;
	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) override;
	virtual bool JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<FUniqueNetIdRef>& FriendList) override;
	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< FUniqueNetIdRef >& Friends) override;
	virtual bool SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< FUniqueNetIdRef >& Friends) override;
	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType) override;
	virtual bool GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo) override;
	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) override;
	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited) override;
	virtual bool RegisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players, bool bWasInvited = false) override;
	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId) override;
	virtual bool UnregisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players) override;
	virtual void RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual int32 GetNumSessions() override;
	virtual void DumpSessionState() override;
};
```

It is mainly a copy paste of the SessionInterfaceNull definition.
All the methods are overriden for the session to work properly, and most fallback on the Null interface, or implements the same logic than the Null interface instide the AWS interface.

We can see there are the member URIs.
These are needed to append to the global API gateway url to call the right lambda for the called method.

To [initialize](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L72) them we do the same than in the Online Subsystem

```cpp
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
```

We also make so that every callback from the Null interface calls back our interface too.
This allows for the user to always use this interface for both LAN connection and Distant connection.

Our Subsystem plugin can now send http request, let's see how it can [create a distant game session](CreateSession.md)