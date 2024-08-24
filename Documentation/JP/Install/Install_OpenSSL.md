# OpenSSLのインストール

OpenSSLが何かご存じない場合は、[Wikipediaの記事](https://ja.wikipedia.org/wiki/OpenSSL)をご覧ください。簡単に言うと、OpenSSLはネットワークセキュリティのための人気のある暗号ライブラリです。

ここでの主な問題は、OpenSSLのインストールと選択が簡単ではないことです。主に互換性の問題が関係しています。

GameliftとUnrealはどちらもOpenSSLを使用していますが、使用しているエンジンのバージョンによって、使用するOpenSSLのバージョンが異なる場合があります。
セキュリティの観点からは最新バージョンのOpenSSLを使用することが推奨されますが、互換性の問題でそれができない場合もあります。
他のバージョンを使用することは可能ですが、コンパイルが成功しても、エンジンのパッケージがそれにより動作しない可能性があります。
または、ライブラリを自分で追加する必要があるかもしれません。

Unreal EngineがどのバージョンのOpenSSLを使用しているかを確認するには、[Engine/Source/ThirdParty/OpenSSL](Engine/Source/ThirdParty/OpenSSL)を参照してください。

インストールに関しては、いくつかのオプションがあります：

- [ダウンロード可能な事前コンパイル版（バージョン3.0以上）](https://openssl-library.org/source/index.html)
- [ソースからコンパイルする](https://github.com/openssl/openssl/tree/master?tab=readme-ov-file#build-and-install)
- [ダウンロード可能な事前コンパイル版（レガシーバージョン + Windows専用）](https://wiki.openssl.org/index.php/Binaries)
- [vcpkgを使ってバージョンをダウンロードし、コンパイルする（Windows専用）](https://vcpkgx.com/details.html?package=openssl)

バイナリをダウンロードしてコンパイルした後は、それを環境変数に追加する必要があります。

Windowsでは、システムのプロパティ > 環境変数 > でOPENSSL_ROOT_DIRをシステム変数に追加する必要があります。

他のオペレーティングシステムでどうなるかはわかりませんが、おそらく同様の操作が必要です。

次に、[Gameliftプラグインのダウンロードとコンパイル](Install_GameliftSDK_UE_Plugin.md)を行ってください。
