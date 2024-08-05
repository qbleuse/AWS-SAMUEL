
#include "OnlineSubsystemAWS.h"
#include "OnlineSessionInterfaceAWS.h"
#include "HttpModule.h"

bool FOnlineSubsystemAWS::Init()
{
	bool toReturn = FOnlineSubsystemNull::Init();

	HTTPModule = &FHttpModule::Get();
	GConfig->GetString(TEXT("OnlineSubsystemAWS"), TEXT("APIGatewayURL"), APIGatewayURL, GEngineIni);
	DistantSessionInterface = MakeShareable(new FOnlineSessionAWS(this));

	return toReturn;
}

bool FOnlineSubsystemAWS::Tick(float DeltaTime)
{
	if (!FOnlineSubsystemNull::Tick(DeltaTime))
	{
		return false;
	}

	if (DistantSessionInterface.IsValid())
	{
		DistantSessionInterface->Tick(DeltaTime);
	}

	return true;
}

bool FOnlineSubsystemAWS::Shutdown()
{
	bool toReturn = FOnlineSubsystemNull::Shutdown();

#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
		ensure(Interface.IsUnique()); \
		Interface = nullptr; \
	}

	DESTRUCT_INTERFACE(DistantSessionInterface);

#undef DESTRUCT_INTERFACE

	return toReturn;
}

FText FOnlineSubsystemAWS::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemAWS", "OnlineServiceName", "AWS");
}


IOnlineSessionPtr FOnlineSubsystemAWS::GetSessionInterface() const
{
	return DistantSessionInterface; 
}

IAWSHttpRequest FOnlineSubsystemAWS::MakeRequest(const FString& RequestURI, const FString& Verb)
{
	IAWSHttpRequest newRequest = HTTPModule->CreateRequest();

	/* empty URI would not make any sense in this context */
	if (!ensureAlwaysMsgf(!RequestURI.IsEmpty(), TEXT("FOnlineSubsystemAWS::MakeRequest was called with an invalid (empty) RequestURI. please make sure your request URIs are set in Engine.ini")))
		return newRequest;

	newRequest->SetURL(APIGatewayURL + RequestURI);
	newRequest->SetHeader("Content-Type", "application/json");

	if (!Verb.IsEmpty())
		newRequest->SetVerb(Verb);

	return newRequest;
}
