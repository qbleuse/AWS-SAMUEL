# Rejoindre une Session

L'utilisateur a choisi une session en sélectionnant parmi les `SessionSearchResult` disponibles.

Voyons comment faire rejoindre la GameSession distante au client de l'utilisateur.

En termes de mise en œuvre, nous avons déjà vu comment se connecter à une session distante dans le chapitre [créer une lambda de création de GameSession](CreateSession.md#lambda-de-création-de-session) et [méthode StartSession Response](CreateSession.md#méthode-start-session-response), mais nous allons passer en revue la solution ici car il y a quelques différences mineures.

## Table des Matières

- [Rejoindre une Session](#rejoindre-une-session)
  - [Table des Matières](#table-des-matières)
  - [L'architecture de la solution](#larchitecture-de-la-solution)
  - [Méthode de demande de session](#méthode-de-demande-de-session)
  - [Lambda pour Rejoindre une Session](#lambda-pour-rejoindre-une-session)
  - [Méthode Join Session Response](#méthode-join-session-response)
  - [Conclusion](#conclusion)

## L'architecture de la solution

C'est exactement la même chose que pour l'architecture de la GameSession, veuillez vous référer à [ce chapitre](CreateSession.md#comprendre-larchitecture-de-la-solution) si vous ne l'avez pas lu auparavant.

## Méthode de demande de session

L'objectif ici est de créer une requête POST et d'envoyer le `GameSessionId` pour créer le `PlayerSessionId`.

[Voici](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L624) à quoi cela ressemble :

```cpp
bool FOnlineSessionAWS::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	uint32 Return = ONLINE_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	FNamedOnlineSession* LANSession = AWSSubsystem->GetLANSessionInterface()->GetNamedSession(SessionName);

	// Ne pas rejoindre une session si on en est déjà dans une ou en hébergeant une
	if (LANSession != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("La session locale (%s) existe déjà, impossible de rejoindre deux fois"), *SessionName.ToString());
	}
	else if (Session != NULL)
	{
		UE_LOG_ONLINE_UC(Warning, TEXT("La session distante (%s) existe déjà, impossible de rejoindre deux fois"), *SessionName.ToString());
	}
	else
	{
		// Si session LAN, utiliser le sous-système null
		if (DesiredSession.IsValid() && DesiredSession.Session.SessionSettings.bIsLANMatch)
		{
			return AWSSubsystem->GetLANSessionInterface()->JoinSession(PlayerNum, SessionName, DesiredSession);
		}

		// Créer une session nommée à partir des données de recherche, toutes les informations devraient déjà y être, car c'est la responsabilité de la recherche de sessions.
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		// Obtenir un ID de joueur pour la PlayerSession AWS
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// Créer un match Internet ou LAN, le port et l'adresse seront récupérés par la requête HTTP que nous allons envoyer ci-dessous
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// Désactiver la publicité lors de la connexion, pour éviter que les clients ne la diffusent sur LAN
		Session->SessionSettings.bShouldAdvertise = false;

		/* C'est ici que la différence est très marquée entre LAN et AWS, les requêtes HTTP entrent en jeu */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;

		/* L'objet de requête réel, sur le client, il est équivalent à démarrer la session, mais sur le serveur, il s'agit d'une requête de création. */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* Le contenu réel de la requête */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		// La seule chose que cette requête doit réellement fournir est le GameSessionId
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString()); // pour vérifier la compatibilité avec un serveur

		/* L'objet qui formatera la requête JSON */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* Ajouter notre contenu et envoyer la requête au serveur */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		JoinSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
	}

	if (Return != ONLINE_IO_PENDING)
	{
		// Déclencher simplement le délégué en cas d'échec
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ONLINE_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}
```

La méthode est assez longue, mais elle fait en fait uniquement ce que nous avons déjà vu.

Elle crée une session côté client (pour l'OSS) ...

```cpp
        // Créer une session nommée à partir des données de recherche, toutes les informations devraient déjà y être, car c'est la responsabilité de la recherche de sessions.
		Session = AddNamedSession(SessionName, DesiredSession.Session);

		// Obtenir un ID de joueur pour la PlayerSession AWS
		check(AWSSubsystem);
		IOnlineIdentityPtr Identity = AWSSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->LocalOwnerId = Identity->GetUniquePlayerId(0);
		}

		// Créer un match Internet ou LAN, le port et l'adresse seront récupérés par la requête HTTP que nous allons envoyer ci-dessous
		FOnlineSessionInfoAWS* NewSessionInfo = new FOnlineSessionInfoAWS();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		// Désactiver la publicité lors de la connexion, pour éviter que les clients ne la diffusent sur LAN
		Session->SessionSettings.bShouldAdvertise = false;

		/* C'est ici que la différence est très marquée entre LAN et AWS, les requêtes HTTP entrent en jeu */
		Session->SessionState = EOnlineSessionState::Starting;
		Return = ONLINE_IO_PENDING;
```

... puis fait les requêtes HTTP.

```cpp
		/* L'objet de requête réel, sur le client, il est équivalent à démarrer la session, mais sur le serveur, il s'agit d'une requête de création. */
		IAWSHttpRequest JoinSessionRequest = AWSSubsystem->MakeRequest(JoinSessionURI, TEXT("POST"));

		/* Le contenu réel de la requête */
		FString JsonRequest;
		TSharedRef<FJsonObject> JsonRequestObject = MakeShared<FJsonObject>();

		FString GameSessionId;
		if (!Session->SessionSettings.Get("GameSessionId", GameSessionId))
		{
			Return = ONLINE_FAIL;
		}

		// La seule chose que cette requête doit réellement fournir est le GameSessionId
		JsonRequestObject->SetStringField(TEXT("GameSessionId"), GameSessionId);
		JsonRequestObject->SetStringField(TEXT("uuid"), Session->LocalOwnerId->ToString()); // pour vérifier la compatibilité avec un serveur

		/* L'objet qui formatera la requête JSON */
		TSharedRef<TJsonWriter<>> JoinSessionWriter = TJsonWriterFactory<>::Create(&JsonRequest);
		FJsonSerializer::Serialize(JsonRequestObject, JoinSessionWriter);

		/* Ajouter notre contenu et envoyer la requête au serveur */
		JoinSessionRequest->SetContentAsString(JsonRequest);
		JoinSessionRequest->OnProcessRequestComplete().BindRaw(this, &FOnlineSessionAWS::OnJoinSessionResponseReceived);
		JoinSessionRequest->ProcessRequest();
```

Donc, cela combine en fait [CreateSession](CreateSession.md#créer-une-session) et [StartSession](CreateSession.md#méthode-start-session-request) en une seule méthode.

Comparé à StartSession, nous avons besoin de moins d'éléments pour créer un `PlayerSessionId`, donc nous n'enverrons que les données pertinentes pour cela, qui sont :

- `GameSessionId`
- UUID de l'utilisateur (non requis)

Voyons comment utiliser ces éléments dans les lambdas.

## Lambda pour Rejoindre une Session

La seule différence entre [Anywhere SDK](../../../Plugins/AWSOSS/SAM/join_session/app.py) et [Legacy SDK](../../../Plugins/AWSOSS/SAM/join_session_local/app.py) cette fois-ci est de savoir si `gamelift` a l'`url_endpoint` défini ou non.

En gros, c'est la différence entre ça...
```py
# l'interface gamelift (vous pouvez définir votre propre point de terminaison ici, un point de terminaison normal serait : https://gamelift.ap-northeast-1.amazonaws.com)
game_lift = boto3.client("gamelift")
```
et ça.
```py
# l'interface gamelift (vous pouvez définir votre propre point de terminaison ici, un point de terminaison normal serait : https://gamelift.ap-northeast-1.amazonaws.com, mais nous utilisons une adresse locale pour notre cas d'utilisation avec Gamelift Local)
game_lift = boto3.client("gamelift", endpoint_url='http://host.docker.internal:9870')
```

Pour la lambda, cela ressemble à ceci :

```py
# ceci est le point d'entrée de la lambda join_session. Celui-ci est conçu pour fonctionner avec Gamelift local (SDK4.0)
def lambda_handler(event, context):
	
	print("dans join_session local")

	# récupération du corps de la requête POST
	body = json.loads(event["body"])
		
	print("renvoi de la session joueur pour", body["GameSessionId"], ".\n")
		
	# nous avons réussi à créer notre GameSession, à créer notre PlayerSession et à l'envoyer en réponse
	return get_player_session_from_game_session(body["GameSessionId"], body)
```

Nous avons déjà vu la fonction `get_player_session_from_game_session` dans le [chapitre lambda pour créer une GameSession](CreateSession.md#lambda-de-création-de-session), donc je ne vais pas la développer davantage. Elle fait exactement la même chose, avec la seule différence que le `GameSessionId` provient du corps de l'événement.

Passons maintenant à la réponse côté client.

## Méthode Join Session Response

[Voici](../../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSessionInterfaceAWS.cpp#L705) la méthode.

```cpp

void FOnlineSessionAWS::OnJoinSessionResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully)
{
	ProcessHttpRequest();

	// Récupération des mêmes données que pour la création de la session de jeu, puisque nous recevons les mêmes informations, le point de connexion pointe vers la PlayerSession
	FString IpAddress = responseObject->GetStringField(TEXT("IpAddress"));
	FString Port = responseObject->GetStringField(TEXT("Port"));
	FString SessionId = responseObject->GetStringField(TEXT("PlayerSessionId"));

	if (IpAddress.IsEmpty() || Port.IsEmpty() || SessionId.IsEmpty())
	{
		UE_LOG_ONLINE(Warning, TEXT("La requête HTTP %s a répondu avec des données invalides. Dump de la réponse:\n%s\n\nTemps écoulé: %f"), *Request->GetURL(), *Response->GetContentAsString(), Request->GetElapsedTime());

		return;
	}

	FNamedOnlineSession* gameSession = GetNamedSession(NAME_GameSession);
	if (FOnlineSessionInfoAWS* SessionInfo = (FOnlineSessionInfoAWS*)(gameSession->SessionInfo.Get()))
	{
		SessionInfo->Init(*AWSSubsystem, IpAddress, Port, SessionId);
		Result = ONLINE_SUCCESS;
	}
	gameSession->SessionState = EOnlineSessionState::InProgress;

	TriggerOnJoinSessionCompleteDelegates(NAME_GameSession, (Result == ONLINE_SUCCESS) ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::CouldNotRetrieveAddress);
}
```

Elle fait presque exactement la même chose que la [méthode de réponse de StartSession](CreateSession.md#méthode-start-session-response).

La seule différence est le callback.

Comme auparavant, il est maintenant de la responsabilité de l'utilisateur d'ajouter le code nécessaire dans le rappel pour pouvoir se connecter réellement au serveur, en utilisant `ClientTravel`.

Encore une fois, il y a un [exemple d'implémentation dans la GameInstance](../../../Source/Private/TestAWSGameInstance.cpp#L288).

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

## Conclusion

Cela conclut notre guide !

Avec ces méthodes, vous devriez être en mesure de :

- Créer une Session depuis un client
- Trouver cette Session depuis un autre client
- Rejoindre la Session trouvée

De la même manière que ce que vous pouvez faire avec l'Online Subsystem Null.

L'implémentation n'est pas complètement testée sur toutes les plateformes et manque encore de nombreuses fonctionnalités qui pourraient être intéressantes à ajouter (allant des fonctionnalités de sécurité de base au matchmaking complet).

Mais c'est encore une fois hors du cadre de ce guide car déjà assez long, et je voulais qu'il reste compréhensible et lisible.

Si vous souhaitez aller plus loin, n'hésitez pas à consulter ma [Note de Référence](../../References.md) (en anglais), car il y a de nombreuses excellentes ressources pour apprendre à ce sujet, et ce n'était que ma modeste contribution.

Quant aux implémentations futures, je laisse cela lachement comme un "exercice pour le lecteur".

En espérant que cela vous aidera dans vos projets ou vos apprentissages.

Amusez-vous bien !
