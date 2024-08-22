# Configurer le Plugin AWS OSS

Pour utiliser le plugin AWS OSS, vous devez configurer certains éléments pour le rendre compatible avec votre projet.

Vous êtes supposé comprendre les concepts et le fonctionnement interne de AWS.

Dans le cas contraire, je vous invite à lire [ce chapitre](../Walkthrough/Design.md#amazon-web-services) du guide.

Si vous avez déjà configuré votre projet pour fonctionner avec le plugin, consultez [cet exemple d'utilisation](Usage.md).

## Intégrer le Plugin dans votre Projet

Le plugin s'intègre comme n'importe quel autre plugin Unreal Engine.

- Placez le dossier AWSOSS dans votre dossier Plugins.
- Ajoutez cette ligne à votre fichier .uproject sous le tableau "Plugins" :

```json
    {
      "Name": "AWSOSS",
      "Enabled": true
    },
```

- Ajoutez cette ligne à votre fichier .Build.cs pour pouvoir appeler les méthodes du plugin (optionnel) :

```cs
  PublicDependencyModuleNames.Add("AWSOSS");
```

Pour les cibles serveur, je vous conseille d'ajouter ceci à votre fichier .Build.cs :

```cs
  PublicDependencyModuleNames.Add("GameLiftServerSDK");
```

Le plugin sera activé, maintenant passons à la configuration.

## Configurer AWS OSS pour le SDK le plus récent

L'intégration prend en charge à la fois le legacy Gamelift SDK et le plus récent, la configuration varie légèrement entre les deux.

Dans les deux cas, vous devez configurer l'URI de l'API Gateway et le nom de la lambda que le plugin utilisera.

> [!NOTE]
> Vous pouvez utiliser votre propre URI distant et vos lambdas, si vous en avez, tant qu'ils utilisent l'API Gateway et que les réponses sont les mêmes.

Un exemple de lignes de configuration de l'AWS OSS peut être vu dans le fichier [DefaultEngine.ini](../../../Config/DefaultEngine.ini) du dépôt.

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL doit être entre guillemets s'il contient des chiffres (car il s'agit d'une URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

Ici, l'URI de l'APIGateway redirige vers localhost, soit la machine locale au port 3000, qui est le port utilisé par AWS SAM lorsqu'il active son propre API Gateway.

> [!NOTE]
> Vous pouvez changer le port dans le fichier de configuration et dans AWS SAM si nécessaire.

Avec le SDK le plus récent, le test en local utilise Gamelift Anywhere, vous devez donc être capable d'accéder à Internet et de communiquer avec Gamelift dans la région que vous avez configurée pour votre test local.

N'oubliez pas de configurer votre région dans l'AWS CLI sinon, cela ne fonctionnera pas.
Vous pouvez le faire en utilisant la commande [configure](https://docs.aws.amazon.com/cli/latest/reference/configure/) ou en modifiant directement votre fichier [~/.aws/config](~/.aws/config) (ce lien peut ne pas fonctionner).

Pour accéder à Gamelift, vous aurez également besoin de credentials valides dans l'AWS CLI
qui peuvent être configurés avec la [même commande](https://docs.aws.amazon.com/cli/latest/reference/configure/) ou en modifiant le fichier [~/.aws/credentials](~/.aws/credentials) (ce lien peut également ne pas fonctionner).

Pour plus d'informations sur les tests locaux avec le nouveau SDK, référez-vous à la [documentation d'AWS sur le sujet](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing.html).

## Configurer AWS OSS pour legacy Gamelift

La configuration pour legacy Gamelift ressemble à ça :

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL doit être entre guillemets s'il contient des chiffres (car il s'agit d'une URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session_local
FindSessionURI=/find_session_local
JoinSessionURI=/join_session_local
```

Les lambdas pour legacy sont différentes car toutes les fonctionnalités de Gamelift ne sont pas disponibles dans le legacy SDK.

Vous devrez également configurer dans chaque lambda le point de terminaison local de Gamelift (cela peut être fait en tant que variable système AWS SAM, mais je préfère le mettre dans les lambdas) :

```py
# l'interface Gamelift (vous pouvez définir votre propre point de terminaison ici, un point de terminaison normal serait : https://gamelift.ap-northeast-1.amazonaws.com, mais nous utilisons une adresse locale pour notre cas d'utilisation afin d'utiliser Gamelift Local)
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

Assurez-vous de faire correspondre le port dans les lambdas et dans votre commande lors du lancement de votre  legacy Gamelift local.

"http://host.docker.internal" redirige vers localhost de la machine depuis l'intérieur de Docker sur Windows, ce qui nous permet de nous connecter à legacy Gamelift en cours d'exécution sur notre machine. Cela peut être différent sur d'autres systèmes d'exploitation.

Pour savoir comment lancer et tester avec un legacy Gamelift, référez-vous à la [documentation d'AWS sur le sujet](https://docs.aws.amazon.com/gamelift/latest/developerguide/integration-testing-local.html).

## Configurer SAM pour votre environnement

J'utilise Python 3.9 pour exécuter les lambdas, mais vous pouvez le changer pour votre propre version de Python ou pour un autre langage de programmation, en modifiant le fichier [template.yaml](../../../Plugins/AWSOSS/SAM/template.yaml).

```yaml
# Plus d'infos sur Globals : https://github.com/awslabs/serverless-application-model/blob/master/docs/globals.rst
Globals:
  Function:
    Runtime: python3.9 # changez cela pour la version de Python de votre choix
    Handler: app.lambda_handler
    Architectures:
      - x86_64 # vous devrez peut-être également changer cela si vous êtes sur un Mac avec Apple Silicon.
    # pas sûr des Globals commentés ici 
    Timeout: 15 # peut sembler long mais les lambdas prennent parfois plus de 3 secondes, c'est pour être sûr
```

Vous pouvez également ajuster les fichiers requirements.txt de chaque lambda pour correspondre à votre environnement.

Maintenant, voyons comment vous pouvez [intégrer le Plugin dans votre projet](Usage.md).
