# Utilisation de AWS OSS

Ce chapitre suppose que vous connaissez l'Online Subsystem d'Unreal (OSS) et AWS Gamelift.

Le plugin est un plugin Online Subsystem, et ses fonctionnalités sont les mêmes que celles de tout autre OSS.

La seule fonctionnalité réellement implémentée pour ce plugin OSS est la connexion à Gamelift. Cela signifie que les changements par rapport à une interface simple se trouvent dans la SessionInterface.

Les méthodes réellement implémentées sont :

- [Créer une session / Démarrer une session](#créer-une-session--démarrer-une-session)
- [Trouver une session](#trouver-une-session)
- [Rejoindre une session](#rejoindre-une-session)

Les autres fonctionnalités de l'interface de session en ligne ne sont pas implémentées ou sont partiellement implémentées (le matchmaking par exemple n'est pas supporté).

Toutes les autres fonctionnalités de l'Online Subsystem utiliseront l'OnlineSubSystemNull.

Examinons comment ces méthodes fonctionnent et un exemple de leur utilisation.

## Créer une session / Démarrer une session

Les méthodes ont été implémentées pour être similaires en architecture à la création de sessions en LAN de l'OSS Null. Donc, les sessions se créent sur Gamelift via une demande de l'utilisateur sur un client, qui associe ce client en tant que propriétaire.

[Voici](../../../Source/Private/TestAWSGameInstance.cpp#L198) un exemple d'appel à CreateSession et StartSession dans un GameInstance :

```cpp
void UTestAWSGameInstance::CreateSession_Implementation(bool isSessionLan)
{
	/* vient du plugin de session avancée,
	 * mais nous le faisons en cpp plutôt qu'en blueprint */

	SessionSettings.bIsLANMatch = isSessionLan;
	SessionSettings.bIsDedicated = !isSessionLan;
	
	if (SessionSettings.bIsDedicated)
	{
		SessionSettings.bUsesPresence = false;
		SessionSettings.bUseLobbiesIfAvailable = false;
	}
	else
	{
		SessionSettings.bUsesPresence = true;
		SessionSettings.bUseLobbiesIfAvailable = true;
	}

	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
	const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();

	bool result = false;

	if (!SessionSettings.bIsDedicated)
	{
		if (netId.IsValid())
		{
			result = sessions->CreateSession(*netId, NAME_GameSession, SessionSettings);
		}
	}
	else
		result = sessions->CreateSession(0, NAME_GameSession, SessionSettings);

	if (result)
	{
		FNamedOnlineSession* namedSession = sessions->GetNamedSession(NAME_GameSession);
		namedSession->bHosting = true;
		sessions->StartSession(NAME_GameSession);
	}
}
```

CreateSession a pour seul but de sauvegarder les paramètres d'une certaine session de jeu localement, dans l'OSS. Ensuite, si vous appelez StartSession sur cette session créée, le plugin créera une session de jeu équivalente avec le client comme propriétaire sur Gamelift et créera une session de joueur associée au client, qu'il renverra comme une chaîne de connexion au client.

Avec cette chaîne de connexion (c'est-à-dire l'URL et le port de votre serveur Gamelift), vous pouvez vous connecter à la nouvelle session de jeu que le client possède.

[Voici](../../../Source/Private/TestAWSGameInstance.cpp#L309) un exemple de la façon dont vous feriez cela dans le callback de StartSession :

```cpp
void UTestAWSGameInstance::JoinAfterStart(FName sessionName, bool startSuceeded)
{
	if (startSuceeded && bShouldJoin)
	{
		const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
		FNamedOnlineSession* namedSession  = sessions->GetNamedSession(sessionName);

		/* session créée en LAN, en faire un serveur écoute */
		if (namedSession->bHosting && namedSession->SessionSettings.bIsLANMatch)
		{
			ToListenServer(GetWorld()->GetMapName());
		}
		else
		{
			/* session créée via AWS, besoin de rejoindre le serveur dédié créé */
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

Le propriétaire sera alors le premier à se connecter au serveur distant et sera reconnu comme le propriétaire.

## Trouver une session

La méthode FindSession utilise la même interface que tout OSS, elle utilise un FOnlineSessionSearch et retourne un tableau de FOnlineSessionSearchResults.

Pour cela, elle demandera des informations sur toutes les sessions de jeu ouvertes sur l'instance Gamelift associée.

[Voici](../../../Source/Private/TestAWSGameInstance.cpp#L253) un exemple de ce à quoi la méthode Find pourrait ressembler :

```cpp
FDelegateHandle UTestAWSGameInstance::FindSession(FOnFindSessionsCompleteDelegate& onFoundSessionDelegate, TSharedPtr<FOnlineSessionSearch>& searchResult)
{
	/* vient du plugin de session avancée,
	 * mais nous le faisons en cpp plutôt qu'en blueprint */

	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();
	if (sessions.IsValid())
	{

		FDelegateHandle delegateHandle = sessions->AddOnFindSessionsCompleteDelegate_Handle(onFoundSessionDelegate);

		const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
		sessions->FindSessions(*netId, searchResult.ToSharedRef());

		return delegateHandle;
	}

	return FDelegateHandle();
}
```

En utilisant cela, vous pouvez ensuite utiliser JoinSession pour vous connecter à la session de jeu choisie.

## Rejoindre une session

JoinSession demande la création d'une PlayerSession associée à la GameSession choisie pour se connecter.

La PlayerSession est créée par la GameSession donnée par le callback, et vous pouvez utiliser l'adresse IP et le port pour vous connecter à la GameSession.

[Voici](../../../Source/Private/TestAWSGameInstance.cpp#L273) un exemple d'appel à JoinSession :

```cpp
bool UTestAWSGameInstance::JoinSessionAndTravel(const FOnlineSessionSearchResult& sessionToJoin)
{
	const IOnlineSessionPtr& sessions = Online::GetSubsystem(GetWorld(), TEXT("AWS"))->GetSessionInterface();

	if (sessions.IsValid())
	{
		const FUniqueNetIdPtr& netId = GetPrimaryPlayerController()->PlayerState->GetUniqueId().GetUniqueNetId();
		if (sessions->JoinSession(*netId, NAME_GameSession, sessionToJoin))
		{
			return sessions->StartSession(NAME_GameSession);
		}
	}

	return false;
}
```

Ensuite, [voici](../../../Source/Private/TestAWSGameInstance.cpp#L288) à quoi pourrait ressembler le callback de JoinSession, pour se connecter au serveur par la suite :

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

Si l'adresse IP et le port sont ouverts, Unreal fait tout pour nous connecter au serveur.

Maintenant que vous comprenez comment l'utiliser, voyons [comment le faire fonctionner](Run.md).
