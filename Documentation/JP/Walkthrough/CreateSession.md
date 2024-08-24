# セッションの作成

HTTPリクエストを送信できるオンラインサブシステムを用いて、リモートゲームセッションを作成し、接続するシステムを設定する方法を見ていきましょう。

## 目次

- [セッションの作成](#セッションの作成)
  - [目次](#目次)
  - [ソリューションのアーキテクチャの理解](#ソリューションのアーキテクチャの理解)
    - [Gamelift Anywhereのアーキテクチャ](#gamelift-anywhereのアーキテクチャ)
    - [レガシーGameliftのアーキテクチャ](#レガシー-gameliftのアーキテクチャ)
  - [Create Sessionメソッド](#create-sessionメソッド)
  - [Start Session Requestメソッド](#start-session-requestメソッド)
  - [セッション作成Lambda](#セッション作成lambda)
    - [Anywhere SDKによるセッション作成](#anywhere-sdkによるセッション作成)
    - [レガシーSDKによるセッション作成](#レガシー-sdkによるセッション作成)
  - [Start Session Responseメソッド](#start-session-responseメソッド)

## ソリューションのアーキテクチャの理解

まず、Gamelift AnywhereとレガシーローカルGamelift（以下、レガシー）のソリューションのアーキテクチャを見ていきましょう。

### Gamelift Anywhereのアーキテクチャ

![Gamelift Anywhereのアーキテクチャ](../../Media/gamelift_anywhere_architecture.png)

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../../Media/Light/Res_User_48_Light.svg" width="24"> </picture> クライアントと <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway は、OSSとの間でHTTPリクエストを交換します（SAMのAPI Gatewayを指すURLを使用）。

2. <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway SAMは、Dockerを使用して <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdaを初期化し、実行します。

3. <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdaは、AWS CLI内の認証情報を使用して <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift にアクセスします。

4. <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> Gamelift は、[GameInstanceでゲームサーバーの "Gamelift->InitSDK()" を成功させることで、ローカルサーバーに直接リダイレクトします](../Usage/Run.md#build-le-serveur-de-votre-jeu)。

5. <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> サーバーは、その後、同じマシンのクライアントと異なるポートで接続します <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../../Media/Light/Res_User_48_Light.svg" width="24"> </picture>。

アーキテクチャは比較的複雑に見えますが、一度設定が完了すれば、あまり難しくはありません。

以下を確認します：

- [クライアント側のHTTPリクエスト](#start-session-requestメソッド)
- [Lambdaの実行](#セッション作成lambda)
- [クライアントとサーバー間の接続](#start-session-responseメソッド)

残りの部分は設定により自動的に行われます。

### レガシーGameliftのアーキテクチャ

![レガシーGameliftのアーキテクチャ](../../Media/legacy_gamelift_architecture.png)

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../../Media/Light/Res_User_48_Light.svg" width="24"> </picture> クライアントと <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway は、OSSとの間でHTTPリクエストを交換します（SAMのAPI Gatewayを指すURLを使用）。

2. <img src="../../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway SAMは、Dockerを使用して <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdaを初期化し、実行します。

3. <img src="../../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdaは、レガシーGameliftが実行されているマシン上のポートに直接ポイントして <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> AWS Gamelift にアクセスします。

4. <img src="../../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> Gamelift は、[GameInstanceでゲームサーバーの "Gamelift->InitSDK()" を成功させることで、ローカルサーバーに直接リダイレクトします](../Usage/Run.md#build-your-games-server-build)。

5. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_Client_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_Client_48_Light.svg"> <img alt="" src="../Media/Light/Res_Client_48_Light.svg" width="24"> </picture> サーバーは、その後、同じマシンのクライアントと異なるポートで接続します <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture>。

Anywhereと比較して、3と4のステップのみが変更されます。レガシー実装の利点の1つは、ローカルマシンから出ることがないことです。ただし、呼び出せないメソッドがある可能性があります。

設定もより迅速ですが、実際の本番環境に近い実装とは言えません。

Gamelift Anywhereと同様に、以下の項目を確認します：

- [クライアント側のHTTPリクエスト](#start-session-requestメソッド)
- [Lambdaの実行](#セッション作成lambda)
- [クライアントとサーバー間の接続](#start-session-responseメソッド)

レガシーとAnywhereのソリューション間で変わるのはLambdaのみです。

## セッション作成メソッド

HTTPリクエストを送信する前に、ユーザーはセッションに関連するデータ（例えば、最大プレイヤー数）を埋めるために、セッションインターフェースにNamedSessionを作成する必要があります。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L)がCreate Game Sessionメソッドです。

```cpp
bool FOnlineSessionAWS::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	uint32 Result = ONLINE_FAIL;

	/* すでにLANセッションやリモートセッションが存在するか確認 */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (Session != NULL) /* リモート確認 */
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("セッション '%s' を作成できません: リモートセッションがすでに存在します。"), *SessionName.ToString());
	}
	else if (LANSession != NULL) /* LAN確認 */
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("セッション '%s' を作成できません: ローカルセッションがすでに存在します。"), *SessionName.ToString());
	}
	else
	{
		if (NewSessionSettings.bIsLANMatch) /* LANでのフォールバック */
		{
			return AWSSubsystem->GetLANSessionInterface()->CreateSession(HostingPlayerNum, SessionName, NewSessionSettings);
		}
		else /* リモートセッションの作成、LAN作成のコピーに近い */
		{
			// 新しいセッションを作成し、ゲームパラメータを深くコピー
			Session = AddNamedSession(SessionName, NewSessionSettings);
			check(Session);
			Session->SessionState = EOnlineSessionState::Creating;

			/* この状況では、受信時に変更されることを意図しています */
			Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
			Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections; // 常にすべての公開接続で開始し、後でローカルプレイヤーが登録します

			Session->HostingPlayerNum = HostingPlayerNum;

			check(AWSSubsystem);
			IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
			if (Identity.IsValid())
			{
				Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
				Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
			}

			// 有効なIDが取得できなかった場合、何かを使用
			if (!Session->OwningUserId.IsValid())
			{
				Session->OwningUserId = FUniqueNetIdAWS::Create(FString::Printf(TEXT("%d"), HostingPlayerNum));
				if (!NewSessionSettings.Get(TEXT("Name"), Session->OwningUserName))
					Session->OwningUserName = FString(TEXT("NullUser"));
			}

			// このビルドの一意なIDを設定して互換性を持たせる
			Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

			// ホストのセッション情報を設定
			FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
			//NewSessionInfo->Init(*NullSubsystem);
			Session->SessionInfo = MakeShareable(NewSessionInfo);

			/* 技術的にはAWSの場合、セッションを作成することは、Unreal側のAWSとクライアント間のインターフェースを表す擬似構造を作成することに相当しますので、実際には作成時に何も行われません */
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
```

Named Sessionを作成するときは、何も行いません。Startが実行されるまでは、セッションは常に「書き込みモード」にあり、ユーザーは内部のデータを変更できます。

そのため、サーバー側でセッションを実際に作成する前に、StartSessionのリクエストを受信するのを待っています。

実際には、この実装でサポートされるデータは非常に少ないです：

- 最大プレイヤー数
- セッションの所有者の名前
- セッションの所有者のID
- クライアントのビルドID

ほぼこれだけです。しかし、必要に応じて拡張できます。重要なのは、これらをStart SessionでHTTPリクエストに送信することで、すべてのプレイヤーがそれらを取得できるようにすることです。

## セッション開始リクエストメソッド

セッションが設定されたら、ユーザーはセッションをリモートで作成し接続するための開始メソッドを呼び出します。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L265)がHTTPリクエストを開始する方法です：

```cpp
bool FOnlineSessionAWS::StartSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;
	/* セッション情報を名前で取得 */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->StartSession(SessionName);
	else if (Session)
	{
		// マッチを複数回開始することはできない
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			/* ここがLANとAWSの違いが非常に際立っている部分で、HTTPリクエストが関与します */
			Session->SessionState = EOnlineSessionState::Starting;
			Result = ONLINE_IO_PENDING;

			/* 実際のリクエストオブジェクト、クライアント側ではセッションの開始に相当しますが、サーバー側では作成リクエストです。 */
			IAWSHttpRequest StartSessionRequest = AWSSubsystem->MakeRequest(StartSessionURI, TEXT("POST"));

			/* リクエストの実際の内容 */
			FString JsonRequest;
			TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();
			
			FString SessionName;
			if (Session->SessionSettings.Get<FString>(TEXT("Name"), SessionName))
				JsonRequestObject->SetStringField(TEXT("session_name"), SessionName); // サーバー上で作成されるセッション名
			else
				JsonRequestObject->SetStringField(TEXT("session_name"), FString::Printf(TEXT("%s's Session"), *UKismetSystemLibrary::GetPlatformUserName())); // 名前が取得できない場合、ジェネリックな名前
			JsonRequestObject->SetNumberField(TEXT("build_id"), Session->SessionSettings.BuildUniqueId); // サーバーとの互換性を確認するため
			JsonRequestObject->SetStringField(TEXT("uuid"), Session->OwningUserId->ToString()); // サーバーとの互換性を確認するため
			/* AWSではプライベート接続を管理しないため、プライベートとパブリックの接続が同時に存在しないように実装されており、
			 * そのため追加することで必要な接続数を常に取得できます */
			JsonRequestObject->SetNumberField(TEXT("num_connections"), Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections);
			
			/* JSONリクエストをフォーマットするオブジェクト */
			TSharedRef<TJsonWriter<>> StartSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
			FJsonSerializer::Serialize(JsonRequestObject, StartSessionWriter);

			/* コンテンツを追加してリクエストをサーバーに送信 */
			StartSessionRequest->SetContentAsString(JsonRequest);
			StartSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnStartSessionResponseReceived);
			StartSessionRequest->ProcessRequest();
		}
		else
		{
			UE_LOG_ONLINE_UC(Warning, TEXT("セッション '%s' のリモートゲームを開始できません。現在の状態: %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("作成されていないセッション (%s) のリモートゲームを開始できません"), *SessionName.ToString());
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}
```

各ステップを見ていきましょう：

```cpp
bool FOnlineSessionAWS::StartSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;
	/* セッション情報を名前で取得 */
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);
	if (LANSession)
		return AWSSubsystem->GetLANSessionInterface()->StartSession(SessionName);
	else if (Session)
	{
		// マッチを複数回開始することはできない
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			/* ここがLANとAWSの違いが非常に際立っている部分で、HTTPリクエストが関与します */
			Session->SessionState = EOnlineSessionState::Starting;
			Result = ONLINE_IO_PENDING;

			/* 実際のリクエストオブジェクト、クライアント側ではセッションの開始に相当しますが、サーバー側では作成リクエストです。 */
			IAWSHttpRequest StartSessionRequest = AWSSubsystem->MakeRequest(StartSessionURI, TEXT("POST"));
```

まず、セッションがLANかどうかを確認し、LANの場合はフォールバックします。次に、セッションを実際に開始できるかどうかを確認します。

開始できる場合、HTTPリクエストのボディを作成します。リクエストはPOSTで、セッションデータをGameliftに送信し、適切なゲームセッションを作成させます。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L61)がHTTPリクエストのボディを作成する方法です：

```cpp
IAWSHttpRequest FOnlineSubsystemAWS::MakeRequest(const FString& RequestURI, const FString& Verb)
{
	IAWSHttpRequest newRequest = HTTPModule->CreateRequest();

	/* この文脈では空のURIは意味がありません */
	if (!ensureAlwaysMsgf(!RequestURI.IsEmpty(), TEXT("FOnlineSubsystemAWS::MakeRequest が無効な RequestURI (空) で呼び出されました。リクエストURIが Engine.ini に定義されていることを確認してください")))
		return newRequest;

	newRequest->SetURL(APIGatewayURL + RequestURI);
	newRequest->SetHeader("Content-Type", "application/json");

	if (!Verb.IsEmpty())
		newRequest->SetVerb(Verb);

	return newRequest;
}
```

API GatewayはJSONなどの複数のコンテンツタイプをサポートしており、UnrealにはJSONフォーマットライブラリがあるので、それを使用します。

[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L22)がHTTPリクエストの定義です：

```cpp
/** AWS Online Subsystem全体で使用されるHTTPリクエストのタイプを定義します */
typedef TSharedRef<class IHttpRequest, ESPMode::ThreadSafe> IAWSHttpRequest;
```

その後、JSONの内容を埋めるだけで、Lambdaで処理されることになります（送信と受信は私たちが担当するので、命名のルールはありません）。

```cpp
    /* リクエストの実際の内容 */
    FString JsonRequest;
    TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

    FString SessionName;
    if (Session->SessionSettings.Get<FString>(TEXT("Name"), SessionName))
        JsonRequestObject->SetStringField(TEXT("session_name"), SessionName); // サーバー上で作成されるセッション名
    else
        JsonRequestObject->SetStringField(TEXT("session_name"), FString::Printf(TEXT("%s's Session"), *UKismetSystemLibrary::GetPlatformUserName())); // 名前が取得できない場合、ジェネリックな名前

    JsonRequestObject->SetNumberField(TEXT("build_id"), Session->SessionSettings.BuildUniqueId); // サーバーとの互換性を確認するため
    JsonRequestObject->SetStringField(TEXT("uuid"), Session->OwningUserId->ToString()); // サーバーとの互換性を確認するため

    /* AWSではプライベート接続を管理しないため、プライベートとパブリックの接続が同時に存在しないように実装されており、
    * そのため追加することで必要な接続数を常に取得できます */
    JsonRequestObject->SetNumberField(TEXT("num_connections"), Session->NumOpenPrivateConnections + Session->NumOpenPublicConnections);

    /* JSONリクエストをフォーマットするオブジェクト */
    TSharedRef<TJsonWriter<>> StartSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
    FJsonSerializer::Serialize(JsonRequestObject, StartSessionWriter);

    /* コンテンツを追加してリクエストをサーバーに送信 */
    StartSessionRequest->SetContentAsString(JsonRequest);
    StartSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnStartSessionResponseReceived);
    StartSessionRequest->ProcessRequest();
```

JSONオブジェクトに内容を入力した後、フォーマットしてHTTPリクエストで送信します。

また、API Gatewayからの応答を受け取るためのコールバックも関連付けています。

## セッション作成のLambda

Lambdaには2つのバージョンがあります: [Anywhere SDK](#create-session-anywhere-sdk) バージョンと [Legacy SDK](#create-session-legacy-sdk) バージョンです。

この順序で両方を見ていきましょう。

### セッション作成 Anywhere SDK

Lambdaは複数の言語をサポートしていますが、私は主にPythonを選びました。理由は、私が唯一熟知している言語だからです。

また、SAM API Gatewayに関連付けられたYAMLファイルでエントリポイントの名前を設定できます。

[こちら](../../../Plugins/AWSOSS/SAM/create_session/app.py#L23)がその関数の例です：

```py
# httpsリクエスト用
import json
# 環境変数用
import os
# AWSのPythonツールキット
import boto3
# AWS呼び出しのエラーおよび例外処理用
import botocore
# Lambdaのスレッドでの待機用
import time

# 環境変数AWS_REGIONを取得
# 実行するサーバーに応じて変更されるべきです。
# （例: アイルランドと大阪にサーバーがあります。
# ゲームサーバーを要求するユーザーは、最も近いサーバーにリダイレクトされ、
# ヨーロッパにいるとeu-west-1、アジアにいるとap-northeast-3が得られます）
my_region = os.environ['AWS_REGION']

# Gameliftインターフェース（ここで独自のエンドポイントを設定できます。通常のエンドポイントは: https://gamelift.ap-northeast-1.amazonaws.com）
game_lift = boto3.client("gamelift")

# Lambdaのエントリポイントであるcreate_session
# Gamelift Anywhereまたは通常のGameliftで動作するように設計されています。
def lambda_handler(event, context):
    
    print("create_session内")

    # POSTリクエストのボディを取得
    body = json.loads(event["body"])

    # このリクエストの最終応答
    response = {}
    
    # ゲームセッションを作成するフリートIDを取得
    my_fleet_id = get_my_fleet_id(body, response)
    
    # フリートIDの取得に失敗した場合、エラーレスポンスを送信
    if response:
        return response
        
    print("フリートIDが取得されました: ", my_fleet_id, " 既存のゲームセッションを再利用しようとしています。\n")

    location = game_lift.list_compute(FleetId = my_fleet_id)["ComputeList"][0]["Location"]

    print("ロケーションの結果: ", location)
    
    # 再利用するゲームセッションが見つからない場合は、新しいセッションを作成します。
    my_game_session = create_game_session(my_fleet_id, location, body, response)
    
    # 新しいゲームセッションの作成に失敗した場合、エラーレスポンスを送信
    if response:
        return response
        
    print("ゲームセッションが正常に作成されました。プレイヤーセッションを送信します。\n")
        
    # ゲームセッションの作成に成功したため、プレイヤーセッションを作成し、それを応答として返します
    return get_player_session_from_game_session(my_game_session["GameSessionId"], body)
```

見るべきポイントがたくさんあります！

まず、`boto3`と`botocore`のインポートが新しく見えるかもしれません。これらはPythonでAWSクラウドシステムとやり取りするためのパッケージです。

ご覧のように、`boto3`のインターフェースを使用しています。これが唯一必要なインターフェースで、Gameliftのインターフェースです。

このLambdaでは、以下の2つのことを行う必要があります：

- クライアントから受け取ったデータでGameSessionを作成する
- クライアントがサーバーに接続できるようにPlayerSessionを作成する

PlayerSessionは、クライアントがAWS Gameliftサーバーに接続するための唯一の方法であり、基本的にはクライアントのAWSへの接続を表しています。

PlayerSessionはGameSessionから作成されるので、まずGameSessionを作成します。

そのためには、GameSessionを作成するためのフリートが必要です

```py
# 現在のリージョンの最初のフリートIDを返す、エラーの場合は0を返し、エラーレスポンスを設定
def get_my_fleet_id(body, response):
    # この例では、単一のフリートのみを使用しているため、そのフリートを単純に取得する
    # (事前に設定しておく必要があります。または、直接フリートIDをコードに埋め込むこともできます。
    # 詳しくは https://github.com/aws-samples/amazon-gamelift-unreal-engine/blob/main/lambda/GameLiftUnreal-StartGameLiftSession.py を参照)
    try:
        # 本番環境では「ビルドID検索」を実装するのが良いかもしれませんが、ローカルテストでは問題があります。
        # list_fleet_response = game_lift.list_fleets(BuildId=body["build_id"], Limit=1)
        list_fleet_response = game_lift.list_fleets(Limit=1)
        
        # ニーズに応じて最初のフリートを取得します。
        return list_fleet_response["FleetIds"][0]
        
    except botocore.exceptions.ClientError as error:
        # AWSに問題の説明を任せる
        print(error.response["Error"]["Message"])
        # サーバーエラーを示す500コードを設定し、何が問題だったのかを説明
        # （正式版では公開しない方が良いかもしれませんが、開発中は貴重です）
        response = {
            "statusCode": 500,
            "body": json.dumps(error.response["Error"]["Message"], default=raw_converter).encode("UTF-8")
        }
        raise error
```

AWS Gameliftを[ガイド](../Usage/Run.md#set-up-new-sdk-local-gamelift)のように設定している場合、フリートは1つしかないはずですので、最初のフリートを取得するだけで問題ありません。

Anywhere SDKでGameSessionを作成するには、フリートIDとロケーションの両方が必要です。これらの情報を単に最初のフリートを検索することで取得した後、[GameSessionを作成](../../../Plugins/AWSOSS/SAM/create_session/app.py#L156)することができます。

```py
def create_game_session(fleet_id, location, body, response):
    # 戻り値となるゲームセッション
    game_session = {}

    try:
        # 見つかったフリートでゲームセッションを作成しようとする
        game_session = game_lift.create_game_session(FleetId=fleet_id, Location=location, MaximumPlayerSessionCount=body["num_connections"], Name=body["session_name"])['GameSession']
        
        # ゲームセッションが作成されてから利用可能になるまでには時間がかかるため、使用可能になるまで待機する
        game_session_activating = True
        while game_session_activating:
        
            # ゲームのアクティベーション状態をチェックする
            game_session_activating = is_game_session_activating(game_session, response)
            
            print("ゲームセッションがアクティベーション中です。\n")

            # エラーが発生した場合は終了
            if response:
                break
                
            # 説明呼び出し間の待機時間（10ms）
            time.sleep(0.01)
        
        # アクティベーションが完了したので、ゲームセッションを返す
        print("ゲームセッションがアクティブになりました。\n")
  
    except botocore.exceptions.ClientError as error:
        print("エラー\n")
        # AWSに問題の説明を任せる
        print(error.response["Error"]["Message"])
        # サーバーエラーを示す500コードを設定し、何が問題だったのかを説明
        # （正式版では公開しない方が良いかもしれませんが、開発中は貴重です）
        response = {
            "statusCode": 500,
            "body": json.dumps(error.response["Error"]["Message"], default=raw_converter).encode("UTF-8")
        }
        raise error

    return game_session
```

これで、JSONボディ、フリートID、そして以前に取得したロケーションを使用してGameSessionを作成できます。

作成プロセスは直接的ですが、サーバーがアクティブである必要があるため、プレイヤーセッションを作成するためには準備が整うまで待機する必要があります。

以下は`is_game_session_activating`の定義です。

```py
def is_game_session_activating(game_session, response):
    game_session_status = "ACTIVATING"
    try:
        # ゲームセッションの情報を取得
        session_details = game_lift.describe_game_sessions(GameSessionId=game_session['GameSessionId'])
        game_session_status = session_details['GameSessions'][0]['Status']
    except botocore.exceptions.ClientError as error:
        # AWSに問題の説明を任せる
        print(error.response["Error"]["Message"])
        # サーバーエラーを示す500コードを設定し、何が問題だったのかを説明
        # （正式版では公開しない方が良いかもしれませんが、開発中は貴重です）
        response = {
            "statusCode": 500,
            "body": json.dumps(error.response["Error"]["Message"], default=raw_converter).encode("UTF-8")
        }
        raise error
    return game_session_status == "ACTIVATING"
```

GameSessionには現在の状態を示すステータスがあります。これが私たちが探しているものです。

一般的には、ゲームサーバー側で状態を変更するには、次のような呼び出しを行います：[こちら](../../../Source/Private/TestAWSGameInstance.cpp#L113)を参照してください。

```cpp
gameLiftSdkModule->ActivateGameSession();
```

この関数が呼び出されると、Gamelift側で状態が変更され、処理を続行できるようになります。

問題は、GameliftがGameSessionの作成メソッドをサーバーに送信し、AWSクラウド外のサーバーがそれに応答するのに数秒かかることです。

もしこれを正しく行うなら、異なるLambda関数でこれらのインタラクションを管理するように分けるのが理想ですが、実装の簡略化のためにすべてを一つのLambda関数にまとめているため、デフォルトの三秒よりも長い待機時間が必要です。

アクティブなゲームセッションを取得した後は、単にプレイヤーセッションを作成してクライアントに返すだけです。

[こちら](../../../Plugins/AWSOSS/SAM/create_session/app.py#L198)が`get_player_session_from_game_session`のコードです：

```py
def get_player_session_from_game_session(game_session_id, body):
    try:
        # プレイヤーセッションを作成し、プレイヤーとサーバー間のインターフェースを確立
        player_session = game_lift.create_player_sessions(GameSessionId=game_session_id, PlayerIds=[body["uuid"]])
        
        # 接続情報を取得し、JSON形式でエンコード
        connection_info = json.dumps(player_session["PlayerSessions"][0], default=raw_converter).encode("UTF-8")

        print(connection_info)

        # 成功した場合は接続情報で応答
        return {
            "statusCode": 200,
            "body": connection_info
        }
        
    except botocore.exceptions.ClientError as error:
        # AWSに問題の説明を任せる
        print(error.response["Error"]["Message"])
        # サーバーエラーを示す500コードを設定し、何が問題だったのかを説明
        # （正式版では公開しない方が良いかもしれませんが、開発中は貴重です）
        return {
            "statusCode": 500,
            "body": json.dumps(error.response["Error"]["Message"], default=raw_converter).encode("UTF-8")
        }
```

このコードはかなりシンプルです：プレイヤーセッションを作成し、それをJSON形式でエンコードして応答として返します。

これで完了です！

このLambda関数はかなり長くなりますが、GameSessionとPlayerSessionをクライアントのために成功裏に作成します。

### Legacy SDKを使ったセッションの作成

ロジックはAnywhere SDKとかなり似ていますが、Gameliftがリモートではなくローカルであるため、いくつかの違いがあります。

[こちら](../../../Plugins/AWSOSS/SAM/create_session_local/app.py#L19)がローカルのLambda関数です：

```py
# httpsリクエスト用
import json
# 環境変数用
import os
# 基本的なAWSツール
import boto3
# AWS呼び出しのエラーと例外処理
import botocore
# Lambdaスレッドの待機用
import time

# ローカルテスト用のフリートID（実際のフリートIDが設定されている場合、最終版でも使用できます）
global_fleet_id = "fleet-123"

# Gameliftインターフェース（カスタムエンドポイントを設定できます。通常のエンドポイントは：https://gamelift.ap-northeast-1.amazonaws.comですが、Gamelift Local用にローカルアドレスを使用しています）
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')

# これはローカルのcreate_session Lambdaのエントリーポイントです。Gamelift Local（SDK4.0）用に設計されています
def lambda_handler(event, context):
    
    print("create_session_local内")

    # POSTリクエストのボディを取得
    body = json.loads(event["body"])

    # このリクエストの最終レスポンス
    response = {}
    
    # Gamelift Localでは、フリートIDは重要ではなく、任意の「fleet-」で始まる文字列を使用できます
    # 一貫している限り、問題はありません。
    my_fleet_id = global_fleet_id
        
    print("フリートIDの取得成功：", my_fleet_id, "新しいゲームセッションの作成開始。\n")
    
    # Gamelift Localでは、ゲームセッションの再利用はサポートされていないため、毎回新しいセッションを作成します。
    # ゲーム実装では、プレイヤーがいなくなった場合はゲームセッションを破棄します。
    my_game_session = create_game_session(my_fleet_id, body, response)
    
    # 新しいゲームセッションの作成中にエラーが発生した場合、エラーレスポンスを返します
    if response:
        return response
        
    print("ゲームセッションの作成成功、プレイヤーセッションの送信開始。\n")
        
    # ゲームセッションの作成に成功したので、プレイヤーセッションを作成して応答として返します
    return get_player_session_from_game_session(my_game_session["GameSessionId"], body)
```

関数はAnywhere SDKと同じですが、`game_lift`インターフェースとフリートIDが異なります。

Legacy SDKのGameliftでは、Gameliftがローカルマシン上でJavaスクリプトによってシミュレートされるため、フリートは実際には存在しませんが、メソッドは依然としてフリートIDを必要とします。
したがって、フリートIDが要件に合致している限り（「fleet-[数字]」と呼ばれる）、ほぼ何でも構いません。

Gameliftインターフェースについては、Lambda内でシミュレートされたGameliftを実際に使用できますが、エンドポイントURLを変更してLambdaがマシン上のGameliftを指すようにする必要があります。
この場合、IPアドレスはWindows上でDockerからプログラムを実行する際の`localhost`の代わりに使用されます。
ポートはスクリプトの実行時にユーザーが指定できます。

これで、Unrealでの応答の使用方法を見ていきましょう。

## セッション開始応答メソッド

応答メソッドは[こちら](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L325)で見つけることができます。

```cpp
void FOnlineSessionAWS::OnStartSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();

	FString IpAdress  = responseObject->GetStringField(TEXT("IpAddress"));
	FString Port	  = responseObject->GetStringField(TEXT("Port"));
	FString SessionId = responseObject->GetStringField(TEXT("PlayerSessionId"));

	if (IpAdress.IsEmpty() || Port.IsEmpty() || SessionId.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("HTTPリクエスト %s の応答が無効なデータで返されました。応答のダンプ：\n%s\n\n経過時間： %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());

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
```

ほとんどのメソッドはHTTPリクエストの送受信に関係しているので、エラーハンドリングのための小さなマクロを作成しました。

[ProcessHttpRequest()](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L27)が何をしているかは以下の通りです。

```cpp
#define ProcessHttpRequest() \
	uint32 Result = ONLINE_FAIL;\
	\
	if (!bConnectedSuccessfully)\
		UE_LOG_ONLINE_UC(Warning, TEXT("HTTPリクエスト %s の接続に失敗しました。経過時間： %f"), *Request->GetURL(), Request->GetElapsedTime());\
	\
	TSharedPtr<FJsonValue> response;\
	TSharedRef<TJsonReader<>> responseReader = TJsonReaderFactory<>::Create(Response->GetContentAsString());\
	if (!FJsonSerializer::Deserialize(responseReader, response))\
		UE_LOG_ONLINE_UC(Warning, TEXT("HTTP応答 %s が期待したJSONオブジェクトではありませんでした。応答のダンプ：\n%s\n\n経過時間： %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());\
	TSharedPtr<FJsonObject> responseObject = response.IsValid() ? response->AsObject() : nullptr;\
	if (!responseObject.IsValid() || responseObject->HasField(TEXT("error")))\
		UE_LOG_ONLINE_UC(Warning, TEXT("HTTP応答 %s が期待したJSONオブジェクトではありませんでした。応答のダンプ：\n%s\n\n経過時間： %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());\
\

```

その後、応答からデータを取得し（接続情報を取得していることがわかります）、SessionInfoを作成します。

これで完了です。

ユーザーは次にSessionInfoを使用して接続を要求し、クライアントがサーバーに接続できるようにする必要があります。これは[Game Instanceでのセッション開始のコールバックメソッド](../../../Source/Private/TestAWSGameInstance.cpp#L303)で示されています。

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

`GetResolvedConnectString`を使用することで、ユーザーにセッション情報のIPアドレスとポートが提供され、ユーザーはそれを`ClientTravel`に直接使用してサーバーに接続しようとします。

つまり、Unrealがすべての作業を行います。

この実装の利点は、リモート接続にも対応していることです。

これにより、AWSクラウド上でリモートのGameSessionとPlayerSessionを作成し、クライアントをサーバーに接続することに成功しました。

しかし、マルチプレイヤーセッションであるため、他のプレイヤーも参加できるようにしたいので、ゲームセッションを見つける必要があります。[その方法について見てみましょう](FindSession.md)。
