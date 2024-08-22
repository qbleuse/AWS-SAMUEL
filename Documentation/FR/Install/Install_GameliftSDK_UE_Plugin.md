# Installer le Plugin Gamelift SDK pour Unreal Engine

Encore une fois, le processus d'installation est assez bien documenté par Amazon lui-même en plusieurs langues, via [ce lien](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-engines-setup-unreal.html#integration-engines-setup-unreal-setup).

Nous allons tout de même procéder étape par étape, car pour une raison quelconque, j'ai eu du mal à le faire.

Ces étapes seront principalement destinées aux utilisateurs Windows.

## Télécharger le Plugin Unreal Engine Gamelift

C'est simple et direct : téléchargez la version Release depuis la [page GitHub](https://github.com/aws/amazon-gamelift-plugin-unreal/releases/).

## Compiler le Gamelift Server SDK

C'est là que ça se complique. Vous devez compiler les libs cpp du Gamelift Server SDK pour faire fonctionner le plugin Unreal Engine.

Les étapes sont assez bien expliquées [ici](https://github.com/aws/amazon-gamelift-plugin-unreal?tab=readme-ov-file#build-the-amazon-gamelift-c-server-sdk), mais il y a quelques pièges.

Premièrement, le manque d'explications sur la nécessité d'OpenSSL. Nous l'avons déjà fait, donc pour nous ça va, mais c'est clairement un point manquant ici.

Deuxièmement, la compilation se fait avec CMake en ligne de commande, donc vous pourriez rencontrer deux problèmes :

- Vous n'avez pas CMake (dans ce cas, téléchargez-le [ici](https://cmake.org/download/), ce n'est pas un problème majeur)
- Vous avez CMake mais il ne fonctionne pas en ligne de commande : vous devrez peut-être l'ajouter aux variables système ou dans votre %PATH% système.

Troisièmement, le générateur utilisé est Visual Studio, et le build se fait en ligne de commande, ce qui signifie que vous devez ajouter vos outils (VS[140/150/160]CMNTOOLS) comme variable système, et ajouter msvc ou similaire à votre %PATH%.
De plus, il peut y avoir des erreurs de compilation en raison de versions différentes de VS par rapport aux sources.

Enfin, CMake devra télécharger des dépôts comme boost, donc vous devez avoir git et une connexion Internet pour que cela fonctionne.

Il y a eu de nombreuses autres façons dont cela a échoué, mais je ne me souviens pas de toutes.
Cela mûrira avec le temps et deviendra plus robuste, mais vous pourriez avoir du mal à obtenir ces deux libs qui sont les seules choses qui vous intéressent vraiment.

## Installer le Plugin dans votre projet

Après avoir obtenu ces deux fichiers et les avoir placés dans le plugin, vous pouvez installer le plugin comme tout autre plugin Unreal dans le dossier Plugins, et l'ajouter à vos fichiers .uproject et .Build.cs pour l'utiliser.

Vous pouvez également ajouter le plugin AWSOSS, car c'est la seule chose vraiment nécessaire pour le faire fonctionner, en dehors de la configuration après l'installation.

Pour cela, je vous invite à consulter [comment utiliser le plugin](../Usage/Configuration.md).

C'est la fin de la partie installation, bon travail !
Prenez une petite pause !
