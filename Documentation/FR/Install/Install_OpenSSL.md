# Installer d'OpenSSL

Dans le cas où vous ne sauriez pas ce qu'est OpenSSL, je vous redirige vers l'[article Wikipedia](https://fr.wikipedia.org/wiki/OpenSSL) qui en parle. En résumé, c'est une bibliothèque cryptographique populaire pour la sécurité réseaux.

Ici, le plus gros problème est que l'installation et le choix d'OpenSSL n'est pas trivial, car le principal problème ici est la compatibilité.

Gamelift et Unreal utilisent tous deux OpenSSL, mais selon la version du moteur que vous utilisez, la version d'OpenSSL employée peut varier.
Il est toujours préférable d'utiliser la dernière version d'OpenSSL pour des raisons de sécurité, mais pour des raisons de compatibilité, cela peut ne pas être possible.
Utiliser une autre version devrait fonctionner, mais le problème est que, bien que la compilation puisse fonctionner, le package du moteur pourrait ne pas fonctionner à cause de cela.
Ou du moins, vous devrez peut-être ajouter les libs vous-même.

Pour vérifier quelle version d'OpenSSL votre version d'UE utilise, vous pouvez consulter [Engine/Source/ThirdParty/OpenSSL](Engine/Source/ThirdParty/OpenSSL).

En ce qui concerne l'installation, il y a plusieurs options :

- [Version précompilée à télécharger (seulement 3.0+)](https://openssl-library.org/source/index.html)
- [Compiler à partir des sources](https://github.com/openssl/openssl/tree/master?tab=readme-ov-file#build-and-install)
- [Version précompilée à télécharger (version héritée + uniquement Windows)](https://wiki.openssl.org/index.php/Binaries)
- [Télécharger la version et compiler avec vcpkg (uniquement Windows)](https://vcpkgx.com/details.html?package=openssl)

Après avoir téléchargé et compilé des binaires fonctionnels, vous devez les ajouter à vos variables d'environnement.

Sur Windows, vous devrez accéder aux propriétés système > variables d'environnement > et ajouter OPENSSL_ROOT_DIR à vos variables système.

Je ne sais pas comment cela fonctionnerait sur d'autres systèmes d'exploitation, mais il est probable que vous deviez faire quelque chose de similaire.

Ensuite, pour [téléchargez et compilez le plugin Gamelift](Install_GameliftSDK_UE_Plugin.md).
