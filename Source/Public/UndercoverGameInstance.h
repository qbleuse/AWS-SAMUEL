// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "OnlineSessionSettings.h"
#include <Interfaces/OnlineSessionInterface.h>
#include "TestAWSGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FPlayerPersistentInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerPseudo;

	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Skin;
};

/**
 * 
 */
UCLASS()
class UNDERCOVER_API UTestAWSGameInstance : public UGameInstance
{
	GENERATED_BODY()

	public:
		virtual void OnStart() override { Super::OnStart(); }

		virtual void Init() override;
		virtual void Shutdown() override { Super::Shutdown(); }
		virtual void StartGameInstance() override { Super::StartGameInstance();	}


		UUndercoverGameInstance();

		/* turns our game instance into a listen server on the level given in parameter. */
		UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void ToListenServer(const FString& levelName);
		void ToListenServer_Implementation(const FString& levelName);

		UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
		void ServerTravel(const FString& levelName, const FString& path = "");
		void ServerTravel_Implementation(const FString& levelName, const FString& path = "");

		UFUNCTION(BlueprintImplementableEvent)
		void OnGameLosesFocus(bool focus);

		/* creates a LAN session */
		UFUNCTION(BlueprintNativeEvent)
		void CreateSession(bool isSessionLan);
		void CreateSession_Implementation(bool isSessionLan);

		UFUNCTION(BlueprintNativeEvent)
		void EndSession();
		void EndSession_Implementation();

		FORCEINLINE void AddPlayersInfo(const FPlayerPersistentInfo& inPlayerInfo) { PlayersInfo.Add(inPlayerInfo); }
		FORCEINLINE void ClearPlayersInfo() { PlayersInfo.Empty(); }

		/* find all sessions */
		FDelegateHandle FindSession(FOnFindSessionsCompleteDelegate& onFoundSessionDelegate, TSharedPtr<FOnlineSessionSearch>& searchResult);
		/* join a single session */
		bool JoinSessionAndTravel(const FOnlineSessionSearchResult& sessionToJoin);

		void TravelToJoinSession(FName sessionName, EOnJoinSessionCompleteResult::Type joinResult);

		UFUNCTION()
		void JoinAfterStart(FName sessionName, bool startSuceeded);

};
