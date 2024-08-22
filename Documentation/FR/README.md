# AWS-SAMUEL

Un dépôt pour une intégration rapide et un test des fonctionnalités de connexion d'AWS Gamelift localement, en utilisant SAM dans Unreal Engine :

[A]mazon [W]eb [S]ervices - [S]erverless [A]ccess [M]odel [U]nreal [E]ngine [L]ibrary.

C'est aussi un jeu de mots pour inclure le nom d'un ami comme nom du dépôt.

![Illustration](../Media/stripped-out-multiplayer-session-based-game-hosting-on-aws.png)

> [!CAUTION]
> Bien que je parle français, et que j'ai relu ce qui m'a été généré, j'ai utilisé un LLM pour accélérer la production de la documentation en français.
> Il est donc possible que des choses me soit passé sous les yeux et que certaines phrases soit bancales. Veuillez donc être compréhensif.

## Table des matières

- [AWS-SAMUEL](#aws-samuel)
  - [Table des matières](#table-des-matières)
  - [Qu'est-ce que c'est ?](#quest-ce-que-cest-)
  - [Ce que ce n'est pas](#ce-que-ce-nest-pas)
  - [Pourquoi ?](#pourquoi-)
  - [Ce qui est inclus](#ce-qui-est-inclus)
  - [Comment installer](#comment-installer)
  - [Comment utiliser / Commencer](#comment-utiliser--commencer)
  - [Guide](#guide)
  - [Références et Liens](#références-et-liens)
  - [Licence](#licence)

## Qu'est-ce que c'est ?

Ce dépôt vise une intégration facile et rapide des fonctionnalités de connexion de base avec AWS Gamelift dans Unreal Engine.

Il utilise et adapte la définition de l'Online Subsystem (OSS) d'Unreal, de sorte que les projets existants utilisant déjà un OSS puissent simplement changer le back-end pour voir leur jeu fonctionner.

L'utilisation prévue de ce projet est un jeu Unreal Engine fonctionnant déjà avec un OSS souhaitant tester l'utilisation d'AWS comme back-end.

Étant donné qu'il est prévu pour une utilisation locale, l'AWS API Gateway est simulé localement en utilisant [AWS-SAM CLI](https://github.com/aws/aws-sam-cli).

Donc, ce dépôt est :

- un moyen simple d'intégrer AWS dans une implémentation existante de l'OSS d'UE, et de le tester localement.
- un pseudo-tutoriel, avec le guide associé de l'implémentation.
- une inspiration pour votre propre implémentation.

## Ce que ce n'est pas

- Une solution complète implémentant toutes les fonctionnalités de AWS Gamelift
- Une intégration robuste et sécurisée

Bien que le code ici puisse être utilisé en dehors du cadre du projet pour créer un AWS API Gateway en utilisant la commande de "deploy" de SAM-CLI ou utiliser l'implémentation Gamelift Anywhere disponible pour vous connecter à votre serveur public distant ; ces deux options ne sont pas testées et sont en dehors du cadre de l'utilisation prévue.

De plus, l'intégration a été réalisée avec une mentalité "As long as it works", donc certaines parties de l'implémentation (comme les lambdas SAM) ne sont pas du tout optimisées.

Vous pouvez étendre le champ d'application en faisant un fork ou simplement en prenant et en modifiant le code à votre guise (la licence choisie le permettra), mais sachez que cela nécessite du travail supplémentaire.

## Pourquoi ?

À l'origine, le besoin d'une intégration de AWS Gamelift dans une interface OSS est venu de mon projet de fin d'études, qui nécessitait une intégration AWS et LAN.

Pour avoir une intégration LAN simple dans UE, Autant utiliser l'interface OSS Null, et j'avais la flemme de faire quelque chose de complètement différent pour la connexion distante avec AWS.

Quant à pourquoi cette concentration sur les tests locaux, c'est parce que bien que nous ayons reçu l'obligation explicite d'utiliser AWS comme serveur distant, AWS ne nous a pas accordé de crédits pour nous permettre de tester notre implémentation.

Nous avons donc été contraints de trouver une solution pour tester notre intégration, complètement gratuitement.

Bien que l'intégration n'ait pas été terminée à temps pour ce projet, j'ai travaillé dessus par la suite dans l'espoir d'aider des personnes dans la même situation.

## Ce qui est inclus

- [Un plugin Unreal Engine personnalisé Online SubSystem, intégrant la création, la recherche et la participation aux sessions de jeu AWS Gamelift](Plugins/AWSOSS/)
- [Un SAM API gateway local pour test local](Plugins/AWSOSS/SAM/)
- [Quelques fichiers source en exemple pour utiliser ledit plugin](Source/)
- [Un fichier .ini avec des lignes à copier pour faire fonctionner le plugin](Config/DefaultEngine.ini)
- [Un guide expliquant comment le code fonctionne pour ceux qui veulent aller plus loin](#guide)

## Comment installer

L'installation est assez compliquée, car elle nécessite de compiler le plugin Gamelift et d'installer d'autres éléments, et constitue une partie importante du travail. Voici le [guide pour l'installer](Documentation/Install/Prerequisites.md).

Une chose à noter : ce plugin et les tests pour vérifier son bon fonctionnement ont été réalisés exclusivement sur Windows 11.

Bien que, voyant les outils utilisés, je pense qu'il ne devrait pas être trop difficile de le faire fonctionner sur d'autres systèmes d'exploitation, sachez que cela n'a pas été testé et qu'il peut y avoir des problèmes de compatibilité.

## Comment utiliser / Commencer

Pour savoir comment utiliser et commencer à utiliser le plugin, voir [ici](Documentation/Usage/Configuration.md)

## Guide

Si vous voulez savoir comment l'intégration a été réalisée, peut-être pour développer votre propre version ou par curiosité, c'est [ici](Documentation/Walkthrough/Design.md).

Encore une fois, ce guide sera présenté du point de vue d'un utilisateur Windows.
De plus, cela s'adresse aux débutants en prog réseau (comme c'était mon cas lors de la création), mais j'attends une expérience avec Unreal Engine et en programmation, et je n'expliquerai donc pas des principes de base de programmation.
De plus, je fait part d'un retour d'expérience, il peut donc y avoir des erreurs ou des omissions.

## Références et Liens

Voici une [liste de tous les liens que j'ai utilisés](..//References.md) pour réaliser ce projet, à voir comme ma bibliographie.

## Licence

AWS-SAMUEL est licencié sous la Licence MIT, voir [LICENSE](../../LICENSE) pour plus d'informations.

Donc, faites ce que vous voulez.
