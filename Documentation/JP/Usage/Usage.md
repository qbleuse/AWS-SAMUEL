# AWS OSSの使用方法

この章では、UnrealのOnline Subsystem（OSS）とAWS Gameliftについて知っていることを前提としています。

このプラグインはOnline Subsystemのプラグインであり、その機能は他のOSSと同じです。

このOSSプラグインで実際に実装されている唯一の機能は、Gameliftへの接続です。つまり、シンプルなインターフェースからの変更はSessionInterfaceにあります。

実際に実装されているメソッドは次の通りです：

- [セッションの作成 / セッションの開始](#セッションの作成--セッションの開始)
- [セッションの検索](#セッションの検索)
- [セッションへの参加](#セッションへの参加)

その他のオンラインセッションインターフェースの機能は、実装されていないか部分的に実装されています（たとえば、マッチメイキングはサポートされていません）。

Online Subsystemのその他の機能はOnlineSubSystemNullを使用します。

これらのメソッドがどのように機能するか、またその使用例を見てみましょう。

## セッションの作成 / セッションの開始

これらのメソッドは、OSS NullのLANセッション作成と同様のアーキテクチャで実装されています。つまり、セッションはユーザーからのリクエストによってGamelift上で作成され、このクライアントが所有者として関連付けられます。

[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L198)が、GameInstanceでCreateSessionとStartSessionを呼び出す例です：

```cpp
void UTestAWSGameInstance::CreateSession_Implementation(bool isSessionLan)
{
	/* 高度なセッションプラグインから来ていますが、
	 * ブループリントではなくcppで行っています */

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

CreateSessionの目的は、ゲームセッションのパラメータをローカルでOSSに保存することだけです。その後、StartSessionを呼び出すと、プラグインはGamelift上でクライアントが所有する等価なゲームセッションを作成し、クライアントに関連付けられたプレイヤーセッションを作成して、その接続文字列をクライアントに返します。

この接続文字列（すなわち、GameliftサーバーのURLとポート）を使用して、クライアントが所有する新しいゲームセッションに接続できます。

[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L309)が、StartSessionのコールバック内での接続方法の例です：

```cpp
void UTestAWSGameInstance::JoinAfterStart(FName sessionName, bool startSuceeded)
{
	if (startSuceeded && bShouldJoin)
	{
		const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
		FNamedOnlineSession* namedSession  = sessions->GetNamedSession(sessionName);

		/* LANで作成されたセッション、リスニングサーバーにする */
		if (namedSession->bHosting && namedSession->SessionSettings.bIsLANMatch)
		{
			ToListenServer(GetWorld()->GetMapName());
		}
		else
		{
			/* AWS経由で作成されたセッション、作成された専用サーバーに参加する必要がある */
			FString travelURL;
			if (GetPrimaryPlayerController() && sessions->GetResolvedConnectString(sessionName, travelURL))
			{
				GetPrimaryPlayerController()->ClientTravel(travelURL, ETravelType::TRAVEL_Absolute);
			}
		}

		bShouldJoin = false;
	}
}
```

所有者は、リモートサーバーに最初に接続し、所有者として認識されます。

## セッションの検索

FindSessionメソッドは、すべてのOSSで同じインターフェースを使用し、FOnlineSessionSearchを利用してFOnlineSessionSearchResultsの配列を返します。

これにより、関連するGameliftインスタンス上で開いているすべてのゲームセッションの情報を要求します。

[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L253)が、Findメソッドがどのようなものかの例です：

```cpp
FDelegateHandle UTestAWSGameInstance::FindSession(FOnFindSessionsCompleteDelegate& onFoundSessionDelegate, TSharedPtr<FOnlineSessionSearch>& searchResult)
{
	/* 高度なセッションプラグインから来ていますが、
	 * ブループリントではなくcppで行っています */

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

これを使用して、JoinSessionを利用して選択したゲームセッションに接続できます。

## セッションへの参加

JoinSessionは、選択したGameSessionに関連付けられたPlayerSessionを作成することで接続を要求します。

PlayerSessionは、コールバックによって提供されるGameSessionによって作成され、IPアドレスとポートを使用してGameSessionに接続できます。

[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L273)が、JoinSessionの呼び出しの例です：

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

次に、[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L288)が、JoinSession後のサーバー接続のコールバックの例です：

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

IPアドレスとポートがオープンしていれば、Unrealはサーバーへの接続を自動で行います。

これで、使用方法が理解できたと思います。次に、[動作させる方法](Run.md)を見てみましょう。