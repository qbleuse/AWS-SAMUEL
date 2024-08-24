# Unreal Engine用Gamelift SDKプラグインのインストール

再度ですが、インストールプロセスはAmazon自身によって複数の言語でよく文書化されています。詳細は[こちらのリンク](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-engines-setup-unreal.html#integration-engines-setup-unreal-setup)をご覧ください。

ただし、何らかの理由で私自身がこれに苦労したため、ステップバイステップで進めます。

これらのステップは主にWindowsユーザー向けです。

## Unreal Engine Gameliftプラグインのダウンロード

手順は簡単で直接的です： [GitHubのリリースページ](https://github.com/aws/amazon-gamelift-plugin-unreal/releases/) からリリース版をダウンロードしてください。

## Gamelift Server SDKのコンパイル

ここが少し複雑です。Unreal Engineプラグインを動作させるためには、Gamelift Server SDKのcppライブラリをコンパイルする必要があります。

ステップは[こちら](https://github.com/aws/amazon-gamelift-plugin-unreal?tab=readme-ov-file#build-the-amazon-gamelift-c-server-sdk)に詳しく説明されていますが、いくつかの落とし穴があります。

まず、OpenSSLの必要性についての説明が不足している点です。私たちはすでにこれを行ったので問題ありませんが、明らかに欠けているポイントです。

次に、コンパイルはCMakeのコマンドラインで行われるため、以下の問題が発生する可能性があります：

- CMakeがインストールされていない場合： [こちら](https://cmake.org/download/) からダウンロードしてください。これは大きな問題ではありません。
- CMakeがインストールされているがコマンドラインで動作しない場合： システム変数に追加するか、%PATH%に追加する必要があるかもしれません。

さらに、使用されるジェネレーターはVisual Studioであり、ビルドはコマンドラインで行われるため、ツール（VS[140/150/160]CMNTOOLS）をシステム変数として追加し、msvcまたは類似のものを%PATH%に追加する必要があります。
また、VSのバージョンとソースのバージョンの違いにより、コンパイルエラーが発生する可能性があります。

最後に、CMakeはboostなどのリポジトリをダウンロードする必要があるため、gitとインターネット接続が必要です。

他にも多くの方法で失敗した経験がありますが、すべてを記憶しているわけではありません。
時間が経つにつれて成熟し、より堅牢になるでしょうが、これらの二つのライブラリを取得するのは本当に苦労するかもしれません。

## プロジェクトへのプラグインのインストール

これらの二つのファイルを取得してプラグインに配置した後は、他のUnrealプラグインと同様にPluginsフォルダーにインストールし、.uprojectおよび.Build.csファイルに追加して使用します。

AWSOSSプラグインも追加できます。これは、インストール後の設定以外で実際に必要な唯一のものです。

そのためには、[プラグインの使い方](../Usage/Configuration.md)を参照してください。

これでインストール部分は終了です。お疲れ様でした！
