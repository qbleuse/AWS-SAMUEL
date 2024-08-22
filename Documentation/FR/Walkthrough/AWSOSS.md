# AWS OSS et Requêtes HTTP

Ici, nous allons examiner la façon dont l’AWS OSS est construit et comment il communique avec AWS.

## Table des Matières

- [AWS OSS et Requêtes HTTP](#aws-oss-et-requêtes-http)
  - [Table des Matières](#table-des-matières)
  - [Créer un Plugin OSS](#créer-un-plugin-oss)
  - [Initialisation de l'Online Subsystem](#initialisation-de-lonline-subsystem)
  - [Initialiser l'Interface de Session AWS](#initialiser-linterface-de-session-aws)

## Créer un Plugin OSS

Pour créer un Plugin OSS, j'ai principalement copié le code source disponible dans les Plugins de l'Engine, voyons cela étape par étape.

Tout d'abord, l'Online SubSystem est un SubSystem, ce qui signifie que dans la définition du Module, il doit être enregistré comme tel.

[Voici](../../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.h#L6) la déclaration du Module.

```cpp
class FAWSOSSModule : public IModuleInterface
{
	public:	
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:

		/** Classe responsable de la création d'instance(s) du sous-système */
		class FOnlineFactoryAWS* AWSOnlineFactory {nullptr};
};
```

Le module est assez simple, mais dans la déclaration, il a besoin de la factory pour enregistrer notre OSS dans le tableau des plateformes disponibles.

[Voici](../../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.cpp#L13) à quoi cela ressemble.

```cpp
class FOnlineFactoryAWS : public IOnlineFactory
{
public:

	FOnlineFactoryAWS() {}
	virtual ~FOnlineFactoryAWS() {}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemAWSPtr AWSOSS = MakeShared<FOnlineSubsystemAWS, ESPMode::ThreadSafe>(InstanceName);
		if (AWSOSS->IsEnabled())
		{
			if(!AWSOSS->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("L'API Null a échoué à s'initialiser !"));
				AWSOSS->Shutdown();
				AWSOSS = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("L'API Null est désactivée !"));
			AWSOSS->Shutdown();
			AWSOSS = NULL;
		}

		return AWSOSS;
	}
};

void FAWSOSSModule::StartupModule()
{
	AWSOnlineFactory = new FOnlineFactoryAWS();
	
	/* en gros, configurer une fabrique pour que notre AWSOSS soit accessible par l'Engine */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(AWS_SUBSYSTEM, AWSOnlineFactory);
}

void FAWSOSSModule::ShutdownModule()
{
	/* retirer du sous-système en ligne chargé */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(AWS_SUBSYSTEM);
	
	/* et supprimer */
	delete AWSOnlineFactory;
	AWSOnlineFactory = NULL;
}
```

> [!NOTE]
> La factory est new et delete, ce qui signifie qu'elle ne fait pas partie du Garbage Collector.

Sinon, je pense que c'est assez simple : créer l'OSS, l'initialiser, et l'enregistrer si cela réussit.

## Initialisation de l'Online Subsystem

Maintenant, [voici](../../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L28) à quoi ressemble l'OnlineSubsystem.

```cpp
/**
 *	OnlineSubsystemAWS - Implémentation personnalisée du sous-système en ligne pour AWS
 */
class AWSOSS_API FOnlineSubsystemAWS :
	public FOnlineSubsystemNull
{

	public:
		virtual ~FOnlineSubsystemAWS() = default;
	
	public:
		/** Seule la fabrique crée des instances */
		FOnlineSubsystemAWS() = delete;
		explicit FOnlineSubsystemAWS(FName InInstanceName) :
			FOnlineSubsystemNull(InInstanceName)
		{}

		virtual		IOnlineSessionPtr	GetSessionInterface() const override;
		FORCEINLINE IOnlineSessionPtr	GetLANSessionInterface() const { return FOnlineSubsystemNull::GetSessionInterface(); }
		FORCEINLINE class FHttpModule*	GetHTTP()const { return HTTPModule; }
		FORCEINLINE FString				GetAPIGateway()const { return APIGatewayURL; }
		
		/** Crée le corps d'une requête HTTP vers l'URI de la passerelle API AWS avec le verbe spécifié
		 * RequestURI: l'URI de la requête HTTP dans l'interface de la passerelle API AWS (généralement quelque chose comme "/login")
		 * Verbe: le type de requête pour l'URI spécifié (peut être GET ou POST, etc.). Ne sera pas spécifié s'il est laissé vide.
		 */
		IAWSHttpRequest MakeRequest(const FString& RequestURI, const FString& Verb = TEXT(""));

		virtual bool Init() override;
		virtual bool Tick(float DeltaTime) override;
		virtual bool Shutdown() override;
		virtual FText GetOnlineServiceName() const override;

	protected:
		/* référence au HTTPModule pour envoyer des requêtes HTTP au serveur AWS */
		class FHttpModule* HTTPModule;

		/** La passerelle API AWS est toujours une URL https, elle permet un accès direct des clients aux services Amazon via lambda
		 * nous l'utiliserons principalement comme notre moyen de communication avec AWS car elle sera plus simple et plus proche dans l'implémentation */
		UPROPERTY(EditAnywhere)
		FString APIGatewayURL;

		/** Interface de session AWS qui gérera à la fois les sessions distantes (via AWS) et les sessions LAN.
		 * Contrairement à ce que le nom pourrait suggérer, elle gère à la fois les connexions Locales et Distantes
		 * en ayant une régression sur LAN implémentée. */
		FOnlineSessionAWSPtr DistantSessionInterface;
};
```

Pour faciliter l'implémentation, il hérite de OnlineSubsystemNull afin que la plupart des autres interfaces à l'intérieur de l'Online Subsystem se basent sur les Interfaces Null.

La seule qui change est la SessionInterface, que nous remplaçons pour que notre OnlineSessionAWSInterface soit utilisée au lieu de l'interface Null.
Nous conservons toujours l'interface Null pour les connexions LAN.

Les autres membres de cette classe concernent les requêtes HTTP.

Voyons leur initialisation dans la méthode [Init de la classe](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L6)

```cpp
bool FOnlineSubsystemAWS::Init()
{
	bool toReturn = FOnlineSubsystemNull::Init();

	HTTPModule = &FHttpModule::Get();
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("APIGatewayURL"), APIGatewayURL, GEngineIni);
	DistantSessionInterface = MakeShareable(new FOnlineSessionAWS(this));

	return toReturn;
}
```

Assez simple, les membres de la classe sont initialisés.

Heureusement, pour envoyer facilement des requêtes HTTP, Unreal dispose d'un Plugin Engine pour cela, et il suffit de l'ajouter à votre [.Build.cs](../../../Plugins/AWSOSS/Source/AWSOSS.Build.cs) dans les DependencyModuleNames.

```csharp
PublicDependencyModuleNames.AddRange(new string[] { 
        "Core",
        "CoreUObject",
        "Engine",
        "CoreUObject",
        "OnlineSubsystem",
        "OnlineSubsystemUtils",
		"OnlineSubsystemNull",
		"GameLiftServerSDK",
		"HTTP",// <-- c'est cette ligne dont on parle
		"Json",
		"Sockets"
      });
```

Avec cela, nous pourrons créer, formater, et envoyer des requêtes HTTP facilement.
Il suffit d'accéder au Module pour l'utiliser, donc nous le sauvegardons comme membre de l'OSS.

Mais pour envoyer des requêtes HTTP, nous avons besoin d'une URI (ou URL).
Pour cela, j'ai décidé d'imiter ce que font les autres OSS lorsque vous devez configurer des éléments à l'intérieur de l'OSS : j'ai utilisé la configuration dans le fichier [DefaultEngine.ini](../../../Config/DefaultEngine.ini), dans une section "OnlineSubsystemAWS".

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
; APIGatewayURL doit être entre guillemets s'il contient des chiffres (car c'est une URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

Cela permet de configurer d'autres URL dans différents scénarios, et est également similaire à la façon dont vous devez configurer l'OSS de toute façon.

Quant à l'URL, le seul problème de l'implémentation est qu'elle utilise une seule URL pour tous les appels à AWS, alors que dans une implémentation réelle, vous voudriez peut-être envoyer une requête GET à cette première URL pour obtenir l'URL de votre propre région, pour une meilleure connectivité.

Et dans cette méthode, nous créons, et donc, initialisons notre Session Interface.

## Initialiser l'Interface de Session AWS

Le sous-système hérite de OnlineSubsystemNull, et possède donc deux interfaces de session : AWSSessionInterface et SessionInterfaceNull.

L'AWSSessionInterface a alors deux rôles : pouvoir envoyer des requêtes HTTP avec les ressources du sous-système en ligne si la connexion est distante, ou se rabattre sur la SessionInterfaceNull si c'est une connexion LAN.

Regardons la [définition](../../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSessionInterfaceAWS.h#L33) pour la classe.

```cpp
class FOnlineSessionAWS : public IOnlineSession
{
private:

	/** Référence au principal OSS personnalisé */
	class FOnlineSubsystemAWS* AWSSubsystem;

	/** En gros, l'URI suivant le portail API permettant d'appeler le lambda responsable de la création de session.
	 * Pour la mise en œuvre dans AWS, référez-vous à l'AWSImplementationNotice disponible à la racine de ce dossier.
	 * Vous pouvez modifier cette valeur en utilisant FConfigCacheIni dans le champ OnlineSessionAWS->StartSessionURI */
	FString StartSessionURI;

	/** En gros, l'URI suivant le portail API permettant d'appeler le lambda responsable de la liste de toutes les sessions.
	 * Pour la mise en œuvre dans AWS, référez-vous à l'AWSImplementationNotice disponible à la racine de ce dossier.
	 * Vous pouvez modifier cette valeur en utilisant FConfigCacheIni dans le champ OnlineSessionAWS->FindSessionURI */
	FString FindSessionURI;

	/** En gros, l'URI suivant le portail API permettant d'appeler le lambda responsable de la liste de toutes les sessions.
	 * Pour la mise en œuvre dans AWS, référez-vous à l'AWSImplementationNotice disponible à la racine de ce dossier.
	 * Vous pouvez modifier cette valeur en utilisant FConfigCacheIni dans le champ OnlineSessionAWS->JoinSessionURI */
	FString JoinSessionURI;

PACKAGE_SCOPE:

	/** Sections critiques pour le fonctionnement sécurisé des listes de sessions */
	mutable FCriticalSection SessionLock;

	/** Paramètres de session actuels */
	TArray<FNamedOnlineSession> Sessions;

	/** Objet de recherche actuel */
	TSharedPtr<FOnlineSessionSearch> CurrentSessionSearch;

	/** Heure de début de la recherche de session actuelle. */
	double SessionSearchStartInSeconds;

	/** Combien de temps devrions-nous attendre pour un appel de recherche avant de dépasser le délai */
	float FindSessionRequestTimeout = 15.0f;

	/** Variable de minuterie pour le délai lorsque la demande est lancée */
	float FindSessionRequestTimer = 15.0f;

	FOnlineSessionAWS(class FOnlineSubsystemAWS* InSubsystem);

	// IOnlineSession
	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSessionSettings& SessionSettings) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, SessionSettings);
	}

	class FNamedOnlineSession* AddNamedSession(FName SessionName, const FOnlineSession& Session) override
	{
		FScopeLock ScopeLock(&SessionLock);
		return new (Sessions) FNamedOnlineSession(SessionName, Session);
	}

	void OnStartSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnFindSessionsResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);
	void OnJoinSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully);

public:

	virtual ~FOnlineSessionAWS() {}

	virtual FUniqueNetIdPtr CreateSessionIdFromString(const FString& SessionIdStr) override;

	/* pour l'implémentation du délai */
	void Tick(float DeltaTime);

	// Getter/Setter de session
			FNamedOnlineSession*		GetNamedSession(FName SessionName) override;
	virtual void						RemoveNamedSession(FName SessionName) override;
	virtual EOnlineSessionState::Type	GetSessionState(FName SessionName) const override;
	virtual bool						HasPresenceSession() override;

	// IOnlineSession
	virtual bool CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings) override;
	virtual bool StartSession(FName SessionName) override;
	virtual bool UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData = true) override;
	virtual bool EndSession(FName SessionName) override;
	virtual bool DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate = FOnDestroySessionCompleteDelegate()) override;
	virtual bool IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId) override;
	virtual bool StartMatchmaking(const TArray< FUniqueNetIdRef >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName) override;
	virtual bool CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName) override;
	virtual bool FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings) override;
	virtual bool FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegate) override;
	virtual bool CancelFindSessions() override;
	virtual bool PingSearchResults(const FOnlineSessionSearchResult& SearchResult) override;
	virtual bool JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession) override;
	virtual bool FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend) override;
	virtual bool FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<FUniqueNetIdRef>& FriendList) override;
	virtual bool SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend) override;
	virtual bool SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< FUniqueNetIdRef >& Friends) override;
	virtual bool SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< FUniqueNetIdRef >& Friends) override;
	virtual bool GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType) override;
	virtual bool GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo) override;
	virtual FOnlineSessionSettings* GetSessionSettings(FName SessionName) override;
	virtual bool RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited) override;
	virtual bool RegisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players, bool bWasInvited = false) override;
	virtual bool UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId) override;
	virtual bool UnregisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players) override;
	virtual void RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual void UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate) override;
	virtual int32 GetNumSessions() override;
	virtual void DumpSessionState() override;
};
```

C'est principalement un copier-coller de la définition de SessionInterfaceNull.
Toutes les méthodes sont surchargées pour que la session fonctionne correctement, et la plupart se rabattent sur l'interface Null ou implémentent la même logique que l'interface Null à l'intérieur de l'interface AWS.

Nous pouvons voir les URIs membres.
Celles-ci sont nécessaires pour être ajoutées à l'URL globale de l'API Gateway afin d'appeler le bon lambda pour la méthode appelée.

Pour [les initialiser](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L72), nous faisons de même que dans l'OnlineSubsystem.

