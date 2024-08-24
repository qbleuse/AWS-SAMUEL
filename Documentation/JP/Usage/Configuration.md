# AWS OSSプラグインの設定

AWS OSSプラグインを使用するには、プロジェクトと互換性を持たせるためにいくつかの設定を行う必要があります。

AWSの概念や内部の仕組みについて理解していることが前提となります。

もし理解が不十分な場合は、[こちらの章](../Walkthrough/Design.md#amazon-web-services)をご覧ください。

すでにプラグインでプロジェクトを設定している場合は、[こちらの使用例](Usage.md)を参照してください。

## プロジェクトへのプラグインの統合

プラグインは他のUnreal Engineプラグインと同様に統合します。

- AWSOSSフォルダーをPluginsフォルダーに配置して。
- .uprojectファイルの「Plugins」セクションに以下を追加する：

```json
    {
      "Name": "AWSOSS",
      "Enabled": true
    },
```

- プラグインのメソッドを呼び出せるように、.Build.csファイルに以下を追加します（オプション）：

```cs
  PublicDependencyModuleNames.Add("AWSOSS");
```

サーバーターゲットの場合は、.Build.csファイルに以下の行を追加する：

```cs
  PublicDependencyModuleNames.Add("GameLiftServerSDK");
```

これでプラグインが有効になりました。次に設定。

## 最新のSDK用のAWS OSSの設定

統合は、レガシーGamelift SDKと最新のGamelift SDKの両方に対応しています。設定は若干異なります。

どちらの場合も、プラグインが使用するAPI GatewayのURIとLambdaの名前を設定する必要があります。

> [!NOTE]
> 自分のリモートURIやLambdaを使用することもできますが、API Gatewayを使用し、レスポンスが同じである限り、問題ありません。

AWS OSSの設定例は、リポジトリの[DefaultEngine.ini](../../../Config/DefaultEngine.ini)ファイルで確認できます。

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURLは数字が含まれている場合、引用符で囲む必要があります（URLであるため）
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

ここでは、APIGatewayのURIがlocalhostのポート3000を指しています。これはAWS SAMが自分のAPI Gatewayを起動する際に使用するポートです。

> [!NOTE]
> 必要に応じて、設定ファイルとAWS SAMでポートを変更できます。

最新のSDKでは、ローカルでのテストにGamelift Anywhereを使用するため、インターネットにアクセスでき、テスト用に設定した地域でGameliftと通信できる必要があります。

AWS CLIで地域を設定しないと動作しません。設定するには、[configureコマンド](https://docs.aws.amazon.com/cli/latest/reference/configure/)を使用するか、[~/.aws/config](~/.aws/config)ファイルを直接編集してください（このリンクが機能しない場合があります）。

Gameliftにアクセスするには、AWS CLIで有効な認証情報も必要です。これも[同じコマンド](https://docs.aws.amazon.com/cli/latest/reference/configure/)で設定するか、[~/.aws/credentials](~/.aws/credentials)ファイルを編集してください（このリンクも機能しない場合があります）。

最新のSDKでのローカルテストについての詳細は、[AWSのドキュメント](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing.html)を参照してください。

## レガシーGamelift用のAWS OSSの設定

レガシーGamelift用の設定は次のようになります：

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURLは数字が含まれている場合、引用符で囲む必要があります（URLであるため）
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session_local
FindSessionURI=/find_session_local
JoinSessionURI=/join_session_local
```

レガシーGamelift用のLambdaを変更したファイルになります。レガシーSDKではGameliftのすべての機能が利用できないので。

また、各LambdaでGameliftのローカルエンドポイントを設定する必要があります（AWS SAMのシステム変数として設定することもできますが、私はLambda内に設定することをお勧めします）：

```py
# Gameliftインターフェース（ここで独自のエンドポイントを設定できます。通常のエンドポイントは： https://gamelift.ap-northeast-1.amazonaws.com ですが、ローカルGameliftを使用するためにローカルアドレスを使用しています）
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

LambdaとローカルGameliftを起動する際に使用するコマンドでポートが一致していることを確認してください。

"http://host.docker.internal"は、Windows上のDocker内からlocalhostにリダイレクトされ、ローカルで実行中のGameliftに接続できます。これが他のオペレーティングシステムで異なるかもしれません。

レガシーGameliftの起動とテスト方法については、[AWSのドキュメンテーション](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing-local.html)を参照してください。

## 環境に合わせてSAMを設定する

自分はPython 3.9を使用してLambdaを実行していますが、自分のPythonバージョンや他のプログラミング言語に変更することもできます。これは[template.yaml](../../../Plugins/AWSOSS/SAM/template.yaml)ファイルを変更することで行えます。

```yaml
# Globalsについての詳細は： https://github.com/awslabs/serverless-application-model/blob/master/docs/globals.rst
Globals:
  Function:
    Runtime: python3.9 # ここをお好みのPythonバージョンに変更してください
    Handler: app.lambda_handler
    Architectures:
      - x86_64 # Apple SiliconのMacを使用している場合は、これも変更する必要があるかもしれません。
    # Globalsのコメントについては不明
    Timeout: 15 # 長く見えるかもしれませんが、Lambdaが3秒以上かかることがあるため、確実に動作させるための設定です
```

また、各Lambdaのrequirements.txtファイルを調整して、環境に合わせてください。

これで、[プロジェクトへのプラグイン統合](Usage.md)の方法を見ていきましょう。
