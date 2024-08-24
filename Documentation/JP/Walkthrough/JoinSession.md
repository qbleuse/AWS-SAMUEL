# セッションに参加する

ユーザーが `SessionSearchResult` の中からセッションを選択しました。

次に、リモートの GameSession をユーザーのクライアントに参加させる方法を見ていきましょう。

実装の観点から、リモートセッションに接続する方法は、[GameSession 作成の Lambda](CreateSession.md#セッション作成のlambda)および[StartSession Response メソッド](CreateSession.md#セッション開始応答メソッド)の章で見てきましたが、いくつかの細かな違いがあるため、ここでも解説します。

## 目次

- [セッションに参加する](#セッションに参加する)
  - [目次](#目次)
  - [ソリューションのアーキテクチャ](#ソリューションのアーキテクチャ)
  - [セッション要求メソッド](#セッション要求メソッド)
  - [セッション参加用 Lambda](#セッション参加用-lambda)
  - [Join Session Response メソッド](#join-session-response-メソッド)
  - [結論](#結論)

## ソリューションのアーキテクチャ

これは GameSession のアーキテクチャとまったく同じです。以前に読んでいない場合は、[この章](CreateSession.md#ソリューションのアーキテクチャの理解)をご参照ください。

## セッション要求メソッド

ここでの目的は、POST リクエストを作成し、`GameSessionId` を送信して `PlayerSessionId` を作成することです。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L624)がその例です：

```cpp
bool FOnlineSessionAWS::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	uint32 Return = ONLINE_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);

	// すでにセッションに参加しているか、ホストしている場合は参加しない
	if (LANSession != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("ローカルセッション (%s) はすでに存在します。二重参加はできません"), *SessionName.ToString());
	}
	else if (Session != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("リモートセッション (%s) はすでに存在します。二重参加はできません"), *SessionName.ToString());
	}
	else
	{
		// LANセッションの場合、サブシステムを null にする
		if (DesiredSession.IsValid() && DesiredSession.Session.SessionSettings.bIsLANMatch)
		{
			return AWSSubsystem->GetLANSessionInterface()->JoinSession(PlayerNum, SessionName, DesiredSession);
		}

		// 検索データから名前付きセッションを作成します。すべての情報はすでにそこにあるはずです。セッション検索の責任です。
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		// AWS PlayerSession 用のプレイヤーIDを取得します
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// インターネットまたは LAN のマッチを作成します。ポートとアドレスは、以下の HTTP リクエストで取得されます
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// 参加時に広告を無効にして、クライアントが LAN でセッションを公開しないようにします
		Session->SessionSettings.bShouldAdvertise = false;

		/* ここが LAN と AWS の大きな違いで、HTTP リクエストが関与します */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;

		/* 実際のリクエストオブジェクト。クライアントではセッションの開始と同等ですが、サーバー側では作成リクエストです。 */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* リクエストの実際の内容 */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		// このリクエストが実際に提供すべき唯一の情報は GameSessionId です
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString()); // サーバーとの互換性を確認するため

		/* JSON リクエストをフォーマットするオブジェクト */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* コンテンツを追加してリクエストをサーバーに送信 */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		JoinSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
	}

	if (Return != ONLINE_IO_PENDING)
	{
		// 失敗した場合はデリゲートをトリガーします
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ONLINE_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}
```

メソッドはかなり長いですが、実際には私たちがすでに見たことを行っています。

クライアント側でセッションを作成し（OSS 用）...

```cpp
        // 検索データから名前付きセッションを作成します。すべての情報はすでにそこにあるはずです。セッション検索の責任です。
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		// AWS PlayerSession 用のプレイヤーIDを取得します
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// インターネットまたは LAN のマッチを作成します。ポートとアドレスは、以下の HTTP リクエストで取得されます
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// 参加時に広告を無効にして、クライアントが LAN でセッションを公開しないようにします
		Session->SessionSettings.bShouldAdvertise = false;

		/* ここが LAN と AWS の大きな違いで、HTTP リクエストが関与します */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;
```

... その後、HTTP リクエストを実行します。

```cpp
		/* 実際のリクエストオブジェクト。クライアントではセッションの開始と同等ですが、サーバー側では作成リクエストです。 */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* リクエストの実際の内容 */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		// このリクエストが実際に提供すべき唯一の情報は GameSessionId です
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString()); // サーバーとの互換性を確認するため

		/* JSON リクエストをフォーマットするオブジェクト */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* コンテンツを追加してリクエストをサーバーに送信 */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		Join

SessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
```

このように、[CreateSession](CreateSession.md#セッションの作成) と [StartSession](CreateSession.md#セッション開始リクエストメソッド) を 1 つのメソッドに統合しています。

StartSession と比較して、`PlayerSessionId` を作成するために必要な要素が少ないため、関連するデータのみを送信します。これには次のものが含まれます：

- `GameSessionId`
- ユーザーの UUID（必須ではありません）

これらの要素を Lambda でどのように使用するかを見てみましょう。

## セッション参加用 Lambda

[Anywhere SDK](../../../Plugins/AWSOSS/SAM/join_session/app.py) と [Legacy SDK](../../../Plugins/AWSOSS/SAM/join_session_local/app.py) の違いは、`gamelift` の `url_endpoint` が設定されているかどうかだけです。

基本的に、これの違いです...
```py
# gamelift インターフェース（ここで独自のエンドポイントを設定できます。通常のエンドポイントは: https://gamelift.ap-northeast-1.amazonaws.com です）
game_lift = boto3.client("gamelift")
```
とこれ。
```py
# gamelift インターフェース（ここで独自のエンドポイントを設定できます。通常のエンドポイントは: https://gamelift.ap-northeast-1.amazonaws.com ですが、Gamelift Local 用にローカルアドレスを使用しています）
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

Lambda の場合は、次のようになります：

```py
# これは lambda のエントリポイントで、Gamelift Local（SDK4.0）用に設計されています
def lambda_handler(event, context):
	
	print("join_session local 内にいます")

	# POST リクエストのボディを取得
	body = json.loads(event["body"])
		
	print("プレイヤーセッションを返します", body["GameSessionId"], ".\n")
		
	# GameSession を作成し、PlayerSession を作成して返すことに成功しました
	return get_player_session_from_game_session(body["GameSessionId"], body)
```

`get_player_session_from_game_session` 関数については、[GameSession 作成用の Lambda](CreateSession.md#セッション作成のlambda) で既に見たので、これ以上詳しく説明する必要はありません。イベントのボディから `GameSessionId` を取得する以外は、全く同じ動作をします。

次に、クライアント側のレスポンスを見てみましょう。

## Join Session Response メソッド

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L705)がそのメソッドです。

```cpp

void FOnlineSessionAWS::OnJoinSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();

	// ゲームセッション作成と同じデータを取得します。プレイヤーセッションに向けられた接続ポイントを取得します
	FString IpAddress = responseObject->GetStringField(TEXT("IpAddress"));
	FString Port = responseObject->GetStringField(TEXT("Port"));
	FString SessionId = responseObject->GetStringField(TEXT("PlayerSessionId"));

	if (IpAddress.IsEmpty() || Port.IsEmpty() || SessionId.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("HTTP リクエスト %s が無効なデータで応答しました。レスポンスのダンプ:\n%s\n\n経過時間: %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());

		return;
	}

	FNamedOnlineSession* gameSession = GetNamedSession(NAME_GameSession);
	if (FOnlineSessionInfoAWS* SessionInfo = (FOnlineSessionInfoAWS*)(gameSession->SessionInfo.Get()))
	{
		SessionInfo->Init(*AWSSubsystem, IpAddress, Port, SessionId);
		Result = ONLINE_SUCCESS;
	}
	gameSession->SessionState = EOnlineSessionState::InProgress;

	TriggerOnJoinSessionCompleteDelegates(NAME_GameSession, (Result == ONLINE_SUCCESS) ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
}
```

このメソッドは、[StartSession のレスポンスメソッド](CreateSession.md#セッション開始応答メソッド) とほぼ同じことを行います。

唯一の違いはコールバックです。

以前と同様に、実際にサーバーに接続するためのコードをコールバックに追加するのはユーザーの責任です。これには `ClientTravel` を使用します。

再度、[GameInstance の実装例](../../../Source/Private/TestAWSGameInstance.cpp#L288)を参照できます。

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

## 結論

これでガイドは終了です！

このメソッドを使えば、次のことができるようになるはずです：

- クライアントからセッションを作成する
- 他のクライアントからそのセッションを見つける
- 見つけたセッションに参加する

Online Subsystem Null でできることと同じように。

実装はすべてのプラットフォームで完全にテストされておらず、追加すべき多くの機能がまだあります（基本的なセキュリティ機能から完全なマッチメイキングまで）。

しかし、これはこのガイドの範囲を超えており、ガイドが理解しやすく、読みやすく保たれるようにしたいと思いました。

さらに進む場合は、私の[参考ノート](../../References.md)（英語）を確認してください。学ぶための優れたリソースが多数あります。これは私のささやかな貢献に過ぎません。

今後の実装については、「読者の演習」としておきます。

これがプロジェクトや学習に役立つことを願っています。

楽しんでくださいね！
