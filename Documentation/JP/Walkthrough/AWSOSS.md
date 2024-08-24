# AWS OSS と HTTP リクエスト

ここでは、AWS OSS がどのように構築され、AWS とどのように通信するかについて検討します。

## 目次

- [AWS OSS と HTTP リクエスト](#aws-oss-と-http-リクエスト)
  - [目次](#目次)
  - [OSS プラグインの作成](#oss-プラグインの作成)
  - [オンラインサブシステムの初期化](#オンラインサブシステムの初期化)
  - [AWS セッションインターフェースの初期化](#aws-セッションインターフェースの初期化)

## OSS プラグインの作成

OSS プラグインを作成するには、主にエンジンのプラグインにあるソースコードをコピーしました。手順を見てみましょう。

まず、Online SubSystem はサブシステムであり、モジュールの定義でそれとして登録する必要があります。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.h#L6)がモジュールの宣言です。

```cpp
class FAWSOSSModule : public IModuleInterface
{
	public:	
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:

		/** サブシステムのインスタンス作成を担当するクラス */
		class FOnlineFactoryAWS* AWSOnlineFactory {nullptr};
};
```

モジュールはかなりシンプルですが、宣言内で OSS を利用可能なプラットフォームのリストに登録するためにファクトリーが必要です。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.cpp#L13)がその見た目です。

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
				UE_LOG_ONLINE(Warning, TEXT("Null API の初期化に失敗しました！"));
				AWSOSS->Shutdown();
				AWSOSS = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Null API が無効です！"));
			AWSOSS->Shutdown();
			AWSOSS = NULL;
		}

		return AWSOSS;
	}
};

void FAWSOSSModule::StartupModule()
{
	AWSOnlineFactory = new FOnlineFactoryAWS();
	
	/* 基本的に、AWSOSS をエンジンで利用できるようにファクトリーを設定します */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(AWS_SUBSYSTEM, AWSOnlineFactory);
}

void FAWSOSSModule::ShutdownModule()
{
	/* ロードされたオンラインサブシステムから解除します */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(AWS_SUBSYSTEM);
	
	/* そして削除します */
	delete AWSOnlineFactory;
	AWSOnlineFactory = NULL;
}
```

> [!NOTE]
> ファクトリーは new と delete で作成されるため、ガーベジコレクターには含まれません。

それ以外はかなりシンプルです：OSS を作成し、初期化し、成功した場合に登録します。

## オンラインサブシステムの初期化

次に、[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L28)がオンラインサブシステムの実装です。

```cpp
/**
 *	OnlineSubsystemAWS - AWS 用のカスタムオンラインサブシステム実装
 */
class AWSOSS_API FOnlineSubsystemAWS :
	public FOnlineSubsystemNull
{

	public:
		virtual ~FOnlineSubsystemAWS() = default;
	
	public:
		/** ファクトリーのみがインスタンスを作成します */
		FOnlineSubsystemAWS() = delete;
		explicit FOnlineSubsystemAWS(FName InInstanceName) :
			FOnlineSubsystemNull(InInstanceName)
		{}

		virtual		IOnlineSessionPtr	GetSessionInterface() const override;
		FORCEINLINE IOnlineSessionPtr	GetLANSessionInterface() const { return FOnlineSubsystemNull::GetSessionInterface(); }
		FORCEINLINE class FHttpModule*	GetHTTP()const { return HTTPModule; }
		FORCEINLINE FString				GetAPIGateway()const { return APIGatewayURL; }
		
		/** 指定された HTTP リクエスト URI に対して HTTP リクエストを作成します
		 * RequestURI: AWS API ゲートウェイ インターフェースの HTTP リクエスト URI (通常は "/login" など)
		 * Verbe: 指定された URI のリクエストタイプ (GET や POST など)。空の場合は指定しません。
		 */
		IAWSHttpRequest MakeRequest(const FString& RequestURI, const FString& Verb = TEXT(""));

		virtual bool Init() override;
		virtual bool Tick(float DeltaTime) override;
		virtual bool Shutdown() override;
		virtual FText GetOnlineServiceName() const override;

	protected:
		/* AWS サーバーに HTTP リクエストを送信するための HTTPModule への参照 */
		class FHttpModule* HTTPModule;

		/** AWS API ゲートウェイは常に https URL で、Amazon のサービスに対して直接クライアントアクセスを提供します。
		 * 主に AWS との通信手段として利用します。 */
		UPROPERTY(EditAnywhere)
		FString APIGatewayURL;

		/** AWS セッションインターフェースは、リモートセッション (AWS 経由) と LAN セッションの両方を管理します。
		 * 名前が示唆するように、LAN も含めてローカルおよびリモート接続を管理します。
		 * LAN のリグレッションを実装しています。 */
		FOnlineSessionAWSPtr DistantSessionInterface;
};
```

実装を簡単にするために、`OnlineSubsystemNull` を継承しています。これにより、Online Subsystem 内の他のほとんどのインターフェースは Null インターフェースに基づいています。

唯一変わるのは `SessionInterface` で、これを `OnlineSessionAWSInterface` に置き換え、Null インターフェースの代わりに使用します。
LAN 接続には Null インターフェースを保持します。

このクラスの他のメンバーは HTTP リクエストに関連しています。

これらの初期化を [Init メソッド](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L6)で見てみましょう。

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

クラスのメンバーは比較的簡単に初期化されています。

幸いなことに、HTTP リクエストを簡単に送信するために、Unreal にはそのためのエンジンプラグインがあります。それを [.Build.cs](../../../Plugins/AWSOSS/Source/AWSOSS.Build.cs) の DependencyModuleNames に追加するだけです。

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
		"HTTP",// <-- これが必要な行です
		"Json",
		"Sockets"
      });
```

これで、HTTP リクエストを簡単に作成、フォーマット、送信することができます。
モジュールにアクセスするだけで使用できるので、OSS のメンバーとして保存します。

HTTP リクエストを送信するには URI (または URL) が必要です。
そのため、他の OSS が OSS 内のアイテムを設定する方法を模倣し、[DefaultEngine.ini](../../../Config/DefaultEngine.ini) の "OnlineSubsystemAWS" セクションに設定しました。

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
; APIGatewayURL が数字を含む場合は引用符で囲む必要があります (URL だからです)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

Voici la traduction en japonais du texte fourni :

---

これにより、さまざまなシナリオで他のURLを設定することができ、また、OSSをどのように設定する必要があるかにも似ています。

URLに関しては、実装の唯一の問題は、すべてのAWS呼び出しに対して単一のURLを使用している点です。実際の実装では、最初のURLにGETリクエストを送信し、自分のリージョンのURLを取得して、より良い接続性を実現することを望むかもしれません。

この方法では、セッションインターフェースを作成し、初期化します。

## AWSセッションインターフェースの初期化

サブシステムはOnlineSubsystemNullを継承しており、したがって、2つのセッションインターフェース：AWSSessionInterfaceとSessionInterfaceNullを持っています。

AWSSessionInterfaceには2つの役割があります。1つはリモート接続時にオンラインサブシステムのリソースでHTTPリクエストを送信すること、もう1つはLAN接続時にSessionInterfaceNullにフォールバックすることです。

[クラスの定義](../../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSessionInterfaceAWS.h#L33)を見てみましょう。

```cpp
class FOnlineSessionAWS : public IOnlineSession
{
private:

	/** 主なOSSカスタムの参照 */
	class FOnlineSubsystemAWS* AWSSubsystem;

	/** セッション作成を担当するラムダを呼び出すAPIポータルに従うURIです。
	 * AWSでの実装については、このフォルダのルートにあるAWSImplementationNoticeを参照してください。
	 * この値は、FConfigCacheIniを使用してOnlineSessionAWS->StartSessionURIフィールドで変更できます。 */
	FString StartSessionURI;

	/** すべてのセッションのリストを取得するラムダを呼び出すAPIポータルに従うURIです。
	 * AWSでの実装については、このフォルダのルートにあるAWSImplementationNoticeを参照してください。
	 * この値は、FConfigCacheIniを使用してOnlineSessionAWS->FindSessionURIフィールドで変更できます。 */
	FString FindSessionURI;

	/** すべてのセッションのリストを取得するラムダを呼び出すAPIポータルに従うURIです。
	 * AWSでの実装については、このフォルダのルートにあるAWSImplementationNoticeを参照してください。
	 * この値は、FConfigCacheIniを使用してOnlineSessionAWS->JoinSessionURIフィールドで変更できます。 */
	FString JoinSessionURI;

PACKAGE_SCOPE:

	/** セッションリストの安全な操作に関するクリティカルセクション */
	mutable FCriticalSection SessionLock;

	/** 現在のセッション設定 */
	TArray<FNamedOnlineSession> Sessions;

	/** 現在の検索オブジェクト */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;

	/** 現在のセッション検索の開始時間 */
	double SessionSearchStartInSeconds;

	/** 検索要求のタイムアウト時間 */
	float FindSessionRequestTimeout = 15.0f;

	/** 要求が発生したときのタイマー変数 */
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

	/* タイムアウトの実装用 */
	void Tick(float DeltaTime);

	// セッションのゲッター/セッター
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

以下は、日本語への翻訳です：

---

これは主に `SessionInterfaceNull` の定義のコピー＆ペーストです。すべてのメソッドがオーバーライドされており、セッションが正しく動作するように調整されています。ほとんどのメソッドは `Null` インターフェースにフォールバックするか、`AWS` インターフェース内で `Null` インターフェースと同じロジックを実装しています。

メンバー URI を確認できます。これらは、API Gateway のグローバル URL に追加する必要があり、呼び出されるメソッドに対して正しいラムダ関数を呼び出すために使います。

[初期化方法](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L72)については、`OnlineSubsystem` と同様の方法で行います。

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

また、Null インターフェースの各コールバックが私たちのインターフェースも呼び出すようにしています。これにより、ユーザーは LAN 接続とリモート接続の両方にこのインターフェースを使用できるようになります。

私たちの OnlineSubsystem プラグインは現在 HTTP リクエストを送信できるようになりました。次に、[リモートゲームセッションを作成する方法](CreateSession.md)を見ていきましょう。
