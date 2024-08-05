#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemImpl.h"

#define ONLINESUBSYSTEMNULL_PACKAGE

#include "OnlineSubsystemNullPackage.h"
#include "HAL/ThreadSafeCounter.h"
#include "OnlineSubsystemNull.h"

#ifndef AWS_SUBSYSTEM
#define AWS_SUBSYSTEM FName(TEXT("AWS"))
#endif



/** Forward declarations of Session inteface */
class FOnlineSessionAWS;
typedef TSharedPtr<class FOnlineSessionAWS, ESPMode::ThreadSafe> FOnlineSessionAWSPtr;

/** define Http request type that will be used throughout AWS Online Subsystem pipeline*/
typedef TSharedRef<class IHttpRequest, ESPMode::ThreadSafe> IAWSHttpRequest;

/**
 *	OnlineSubsystemAWS - Custom Implementation of the online subsystem for AWS
 */
class AWSOSS_API FOnlineSubsystemAWS :
	public FOnlineSubsystemNull
{

	public:
		virtual ~FOnlineSubsystemAWS() = default;
	
	public:
		/** Only the factory makes instances */
		FOnlineSubsystemAWS() = delete;
		explicit FOnlineSubsystemAWS(FName InInstanceName) :
			FOnlineSubsystemNull(InInstanceName)
		{}

		virtual		IOnlineSessionPtr	GetSessionInterface() const override;
		FORCEINLINE IOnlineSessionPtr	GetLANSessionInterface() const { return FOnlineSubsystemNull::GetSessionInterface(); }
		FORCEINLINE class FHttpModule*	GetHTTP()const { return HTTPModule; }
		FORCEINLINE FString				GetAPIGateway()const { return APIGatewayURL; }
		
		/** Creates the body of an http request to the AWS API Gateway request URI with specified verb
		 * RequestURI: the URI of the http request in the AWS API Gateway interface (typically, something similar to "/login")
		 * Verb: the type of request for the specified uri (can be GET or POST, etc...). Won't be specified if left empty.
		 */
		IAWSHttpRequest MakeRequest(const FString& RequestURI, const FString& Verb = TEXT(""));

		virtual bool Init() override;
		virtual bool Tick(float DeltaTime) override;
		virtual bool Shutdown() override;
		virtual FText GetOnlineServiceName() const override;

	protected:
		/* reference to the HTTPModule to send HTTP request to AWS server */
		class FHttpModule* HTTPModule;

		/** AWS's API Gateway is always a https url, it allows for direct access of client to Amazon's services through lambda
		 * we will mainly use this as our way of communicating with AWS as it will be more straightforward and closer in implementation */
		UPROPERTY(EditAnywhere)
		FString APIGatewayURL;

		/** AWS Session Interface that will handle both distant (through AWS) and LAN sessions.
		 * As opposed to what the name would suggest, it does handle both Local and Distant connection 
		 * by having a rollback on LAN implemented. */
		FOnlineSessionAWSPtr DistantSessionInterface;
};

typedef TSharedPtr<FOnlineSubsystemAWS, ESPMode::ThreadSafe> FOnlineSubsystemAWSPtr;