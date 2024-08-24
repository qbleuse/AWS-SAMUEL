# セッションの検索

セッションを作成した後、他のユーザーがそのゲームセッションに参加できるようにする必要があります。ユーザーが選択して参加できるように、作成されたゲームセッションのリストを表示する方法を見てみましょう。

## 目次

- [セッションの検索](#セッションの検索)
  - [目次](#目次)
  - [ソリューションのアーキテクチャ](#ソリューションのアーキテクチャ)
    - [Gamelift Anywhereのアーキテクチャ](#gamelift-anywhereのアーキテクチャ)
    - [Gamelift Legacyのアーキテクチャ](#gamelift-legacyのアーキテクチャ)
  - [セッション検索リクエスト](#セッション検索リクエスト)
  - [FindSession用のLambda](#findsession用のlambda)
    - [SDK Anywhereによるセッション検索](#sdk-anywhereによるセッション検索)
    - [SDK Legacyによるセッション検索](#sdk-legacyによるセッション検索)
  - [セッション検索応答メソッド](#セッション検索応答メソッド)

## ソリューションのアーキテクチャ

このソリューションは、OSS NullのLAN実装を再現しようとしています。現在のソリューションは、OSS内で作成されたGameSessionの情報を収集し、実際の実装はユーザーに任せるというものです。ゲームセッションのデータを取得するだけで済みます。

つまり、アーキテクチャはGameSessionを作成するためのものとかなり似ていますが、唯一の違いは接続がないことです。

### Gamelift Anywhereのアーキテクチャ

![Gamelift Anywhereのアーキテクチャ](../../Media/gamelift_anywhere_architecture_find.png)

やり取りは、[セッション作成セクション](CreateSession.md#architecture-gamelift-anywhere)で説明された方法と同じです。つまり：

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../../Media/Light/Res_User_48_Light.svg" width="24"> </picture> クライアントと <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway は、OSSからHTTPリクエストを通じて通信します（SAM API Gatewayを指すURLを使用）。

2. <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> SAM API Gateway はDockerを使用して、その <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambda を初期化し、実行します。

3. <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambda は、AWS CLIの認証情報を使用して <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift にアクセスします。

### Gamelift Legacyのアーキテクチャ

![Gamelift Legacyのアーキテクチャ](../../Media/legacy_gamelift_architecture_find.png)

再び、[セッション作成セクション](CreateSession.md#architecture-legacy-gamelift)で説明された方法と同じです。つまり：

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../../Media/Light/Res_User_48_Light.svg" width="24"> </picture> クライアントと <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway は、OSSからHTTPリクエストを通じて通信します（SAM API Gatewayを指すURLを使用）。

2. <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> SAM API Gateway はDockerを使用して、その <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambda を初期化し、実行します。

3. <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambda は、Gamelift Legacy が実行されているマシンのポートを直接指すことで <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift にアクセスします。

## セッション検索リクエスト

Gameliftでゲームセッションの情報にアクセスするためには、HTTPリクエストを送信してLambdaを呼び出す必要があります。

リクエストメソッドは、StartSessionで見たものと非常に似ています。というのも、ロジックがかなり似ているからです。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L487)がそのメソッドです。

```cpp
bool FOnlineSessionAWS::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	uint32 Return = ONLINE_FAIL;

	if (SearchSettings->bIsLanQuery)
	{
		Return = AWSSubsystem->GetLANSessionInterface()->FindSessions(SearchingPlayerNum, SearchSettings) ? ONLINE_SUCCESS : Return;
	}
	// 他の検索が進行中の場合、別の検索を開始しない
	else if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// 前の結果をクリア
		SearchSettings->SearchResults.Empty();

		// 検索ポインタをコピーして保持
		CurrentSessionSearch = SearchSettings;

		// 検索開始時の時間を記録（適切なPingの推定に使用）
		SessionSearchStartInSeconds = FPlatformTime::Seconds();

		// タイマのリセット
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
		UE_LOG_ONLINE_UC(Warning, TEXT("検索待機中のため、ゲーム検索リクエストを無視します"));
		Return = ONLINE_IO_PENDING;
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}
```

[StartSessionメソッド](CreateSession.md#セッション開始リクエストメソッド)との違いは、POSTではなくGETリクエストであることです。サーバーを変更するのではなく、データを要求するためです。また、`FindSessionURI`を使用しています。

それ以外はかなり似ています。

## FindSession用のLambda

セッション作成用の[lambda](CreateSession.md#セッションの作成)と同様に、FindSession用のlambdaには2つのバージョンがあります。1つは[SDK Anywhere](#sdk-anywhereによるセッションの検索)版、もう1つは[SDK Legacy](#sdk-legacyによるセッションの検索)版です。

### SDK Anywhereによるセッションの検索

セッション作成用のlambdaと比較すると、セッション検索用のlambdaは実際にはかなり短いです。なぜなら、Gameliftにデータを要求して、それを含むHTTPレスポンスを返すだけで済むからです。

[こちら](../../../Plugins/AWSOSS/SAM/find_session/app.py)が関数です：

```py
def lambda_handler(event, context):
	
	print("find_session内")

	# このリクエストの最終的なレスポンス
	response = {}
	
	# ゲームセッションを作成するフリートのID
	my_fleet_id = get_my_fleet_id(response)
	
	# フリートIDの取得に失敗した場合、エラーレスポンスを返す
	if response:
		return response
		
	print("フリートIDの取得に成功しました：", my_fleet_id, "ゲームセッションの検索を開始します")

	location = game_lift.list_compute(FleetId = my_fleet_id)["ComputeList"][0]["Location"]

	print("ロケーションの取得結果：", location)

	# 以下で試みているのはリソースを保護するために行うべきことですが、Gameliftのローカル（SDK 4またはそれ以前）では、game_sessionの検索が機能しません。
	
	# フリート内の既存のゲームセッションを検索する
	game_sessions = game_lift.search_game_sessions(FleetId = my_fleet_id, Location = location, FilterExpression = "hasAvailablePlayerSessions=true")

	print("ゲームセッションが見つかりました：", game_sessions.__str__(), "そのまま送信します")
		
	# これは単純なGETリクエストで、クライアントに現在の状態を示す必要があります。
	# したがって、この生のレスポンスをそのまま送信します
	return {
		"statusCode": 200,
		"body" : json.dumps(game_sessions["GameSessions"], default = raw_converter).encode("UTF-8")
	}
```

多くの関数は[セッション作成用のlambda](CreateSession.md#セッション作成-anywhere-sdk)で既に見たもので、何か不明な点があればそちらを参照できます。

現在のゲームセッションを取得するために使用される唯一のメソッドは[search_game_sessions](https://boto3.amazonaws.com/v1/documentation/api/latest/reference/services/gamelift/client/search_game_sessions.html)です。

GameSessionのデータを取得するためにいくつかの関数がありますが、`search_game_sessions`の利点は、パラメータに基づいてGameSessionをフィルタリングできることです。

この範囲を超えますが、リクエストをPOSTリクエストに置き換え、アクティブなGameSessionの中から必要なものをフィルタリングするためのデータを提供することができます。ここで示している唯一のフィルタは、まだ完了していないGameSession（つまり、他のPlayerSessionを作成できるもの）に対してのみ応答するという点です。

これがSDK Legacyとの唯一の違いです。

### SDK Legacyによるセッションの検索

[こちら](../../../Plugins/AWSOSS/SAM/find_session_local/app.py)がSDK Legacyの関数です：

```py
def lambda_handler(event, context):
	
	print("find_session内")

	# このリクエストの最終的なレスポンス
	response = {}
	
	# ゲームセッションを作成するフリートのID。
	# Gameliftローカル（SDK 4.0）では、同じものであれば実際には重要ではありません。
	my_fleet_id = global_fleet_id
	
	# フリートIDの取得に失敗した場合、エラーレスポンスを返す
	if response:
		return response
		
	print("フリートIDの取得に成功しました：", my_fleet_id, "ゲームセッションの検索を開始します")
	
	# フリート内の既存のゲームセッションを検索する
	# Gameliftローカルではセッション検索が存在しないため、describe_game_sessionsを使用します
	# フィルタがないことに注意してください。これはこの実装の問題ですが、この環境は非常に小さいため、実際には大きな問題ではありません。
	game_sessions = game_lift.describe_game_sessions(FleetId = my_fleet_id)

	print("ゲームセッションが見つかりました：", game_sessions.__str__(), "そのまま送信します")
		
	# これは単純なGETリクエストで、クライアントに現在の状態を示す必要があります。
	# したがって、この生のレスポンスをそのまま送信します
	return {
		"statusCode": 200,
		"body" : json.dumps(game_sessions["GameSessions"], default = raw_converter).encode("UTF-8")
	}
```

再度、[SDK Legacyのセッション作成用lambda](CreateSession.md#legacy-sdkを使ったセッションの作成)の実装と非常に似ていますが、1つの関数が異なります。

なぜ同じ「search_game_session」関数を使ってフィルタをかけてGameSessionのデータを取得しないのかというと、単純に言うと、使用できないからです。

[AWSのドキュメント](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing-local.html)には、GameliftローカルのSDK Legacyは以下のAPIのみをサポートしていると書かれています：

- CreateGameSession
- CreatePlayerSession
- CreatePlayerSessions
- DescribeGameSessions
- DescribePlayerSessions

したがって、利用可能でアクティブなGameSessionsの情報を取得するために使用する関数は選択の余地がありません。

リクエストがGameSessionsのデータを提供するようになったので、レスポンスを処理しましょう。

## Find Session Responseメソッド

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L531)がメソッドです。

```cpp
void FOnlineSessionAWS::OnFindSessionsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();
	
	/** 次の2つのケースで発生する可能性があります：
	* - 前のリクエストのレスポンスがタイムアウト後に到着した場合、失敗したと見なして無視します
	* - このリクエストのレスポンスがタイムアウト後に到着した場合も、同様に無視します。 */
	if (CurrentSessionSearch->SearchState != EOnlineAsyncTaskState::InProgress)
		return;

	// エラーなしでセッションを受け取ったので、結果が満足いくものであると考えます
	// GameSessionがない場合は、実際には妥当な結論であり、単に現在GameSessionがないことを意味します
	Result = ONLINE_SUCCESS;

	// 少なくとも何もないよりはマシなpingの概算を計算します
	int32 VeryBadPingApproximate = FPlatformTime::Seconds() - SessionSearchStartInSeconds;

	TArray<TSharedPtr<FJsonValue>> GameSessions = response->AsArray();
	for (int32 i = 0; i < GameSessions.Num(); i++)
	{
		// データの抽出を容易にするためにGameSessionsをオブジェクトとして取得します
		TSharedPtr<FJsonObject> GameSession = GameSessions[i]->AsObject();

		// GameSession AWSデータを埋めるために使用する構造体
		FOnlineSessionSearchResult SearchResult;

		SearchResult.PingInMs = VeryBadPingApproximate;

		// 現在、プライベートまたはパブリックなGameSessionsを実装していないので、両方を埋めて同様に機能させます
		{
			// 最大PlayerSessions数の設定
			SearchResult.Session.SessionSettings.NumPublicConnections = GameSession->GetNumberField(TEXT("MaximumPlayerSessionCount"));
			SearchResult.Session.SessionSettings.NumPrivateConnections = SearchResult.Session.SessionSettings.NumPublicConnections;

			// 現在利用可能なPlayerSessions数
			SearchResult.Session.NumOpenPublicConnections = SearchResult.Session.SessionSettings.NumPublicConnections - GameSession->GetNumberField(TEXT("CurrentPlayerSessionCount"));
			SearchResult.Session.NumOpenPrivateConnections = SearchResult.Session.NumOpenPublicConnections;
		}

		// ゲームセッションの所有者が選んだフレンドリーネームを設定します
		SearchResult.Session.SessionSettings.Set(TEXT("Name"), GameSession->GetStringField("Name"));
		// GameSessionのIDを設定します。ユーザーがこのセッションに参加したい場合のために（ユーザーには見えないはずですが、それでもどこかに保存する必要があります）
		SearchResult.Session.SessionSettings.Set(TEXT("GameSessionId"), GameSession->GetStringField("GameSessionId"));

		CurrentSessionSearch->SearchResults.Add(SearchResult);
	}
	
	CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;
	TriggerOnFindSessionsCompleteDelegates((Result == ONLINE_SUCCESS) ? true : false);
}
```

詳細を見てみましょう。

まず、レスポンスが有効であり、遅すぎて到着していないかを確認します。

```cpp
void FOnlineSessionAWS::OnFindSessionsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();
	
	/** 次の2つのケースで発生する可能性があります：
	* - 前のリクエストのレスポンスがタイムアウト後に到着した場合、失敗したと見なして無視します
	* - このリクエストのレスポンスがタイムアウト後に到着した場合も、同様に無視します。 */
	if (CurrentSessionSearch->SearchState != EOnlineAsyncTaskState::InProgress)
		return;
```

次に、レスポンスが提供するGameSessionsの数をループして、各GameSessionのために`SessionSearchResult`オブジェクト（OSSオブジェクト）を作成します。

```cpp
// 少なくとも何もないよりはマシなpingの概算を計算します
	int32 VeryBadPingApproximate = FPlatformTime::Seconds() - SessionSearchStartInSeconds;

	TArray<TSharedPtr<FJsonValue>> GameSessions = response->AsArray();
	for (int32 i = 0; i < GameSessions.Num(); i++)
	{
		// データの抽出を容易にするためにGameSessionsをオブジェクトとして取得します
		TSharedPtr<FJsonObject> GameSession = GameSessions[i]->AsObject();

		// GameSession AWSデータを埋めるために使用する構造体
		FOnlineSessionSearchResult SearchResult;

		SearchResult.PingInMs = VeryBadPingApproximate;
```

次に、適切なデータを検索結果に追加します。それには以下が含まれます：

- 最大プレイヤー数
- 現在のプレイヤー数
- セッション名
- GameSessionのID

```cpp
		// 現在、プライベートまたはパブリックなGameSessionsを実装していないので、両方を埋めて同様に機能させます
		{
			// 最大PlayerSessions数の設定
			SearchResult.Session.SessionSettings.NumPublicConnections = GameSession->GetNumberField(TEXT("MaximumPlayerSessionCount"));
			SearchResult.Session.SessionSettings.NumPrivateConnections = SearchResult.Session.SessionSettings.NumPublicConnections;

			// 現在利用可能なPlayerSessions数
			SearchResult.Session.NumOpenPublicConnections = SearchResult.Session.SessionSettings.NumPublicConnections - GameSession->GetNumberField(TEXT("CurrentPlayerSessionCount"));
			SearchResult.Session.NumOpenPrivateConnections = SearchResult.Session.NumOpenPublicConnections;
		}

		// ゲームセッションの所有者が選んだフレンドリーネームを設定します
		SearchResult.Session.SessionSettings.Set(TEXT("Name"), GameSession->GetStringField("Name"));
		// GameSessionのIDを設定します。ユーザーがこのセッションに参加したい場合のために（ユーザーには見えないはずですが、それでもどこかに保存する必要があります）
		SearchResult.Session.SessionSettings.Set(TEXT("GameSessionId"), GameSession->GetStringField("GameSessionId"));

		CurrentSessionSearch->SearchResults.Add(SearchResult);
	}
	
	CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;
	TriggerOnFindSessionsCompleteDelegates((Result == ONLINE_SUCCESS) ? true : false);
```

`SearchResults`に何を入れるかはあなたの自由であり、これは基本的な実装に過ぎません。ユーザーのためにGameSessionsのリストをソートするなど、さらに機能を追加したいかもしれません。

この関数は、検索結果が他の場所で使用できるように、コールバックを最後に呼び出します。

期待される使い方は、ユーザーが自分のセッションを選べるように、GameSessionsをリストアップしてセッションメニューを作成することです。

> [!NOTE]
> これは大きく範囲を超えますが、クライアントにGameSessionsに関する*すべてのデータ*を送信しない方が良い場合があります。これは悪意のある使用に使われる可能性があるからです。
> 必要な情報だけを送信するようにしてください。

> [!WARNING]
> 直感に反するかもしれませんが、このソリューションをテストし、動作したことは確認しましたが、複数のセッションでのセッション検索が機能するかどうかはテストできなかったため、問題があるかもしれません。

ユーザーが利用可能なGameSessionの中から選べるようになったので、次に[GameSessionに参加する方法](JoinSession.md)を見てみましょう。
