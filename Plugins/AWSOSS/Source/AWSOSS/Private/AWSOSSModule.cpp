#include "AWSOSSModule.h"
#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemAWS.h"

IMPLEMENT_MODULE(FAWSOSSModule, AWSOSS);

/**
 * Class responsible for creating instance(s) of the AWS Online Subsystem (AWSOSS)
 */
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
				UE_LOG_ONLINE(Warning, TEXT("Null API failed to initialize!"));
				AWSOSS->Shutdown();
				AWSOSS = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("Null API disabled!"));
			AWSOSS->Shutdown();
			AWSOSS = NULL;
		}

		return AWSOSS;
	}
};

void FAWSOSSModule::StartupModule()
{
	AWSOnlineFactory = new FOnlineFactoryAWS();
	
	/* basically, setup a factory for our AWSOSS to be accessible by the Engine */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(AWS_SUBSYSTEM, AWSOnlineFactory);
}

void FAWSOSSModule::ShutdownModule()
{
	/* remove from loaded online subsystem */
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(AWS_SUBSYSTEM);
	
	/* and delete */
	delete AWSOnlineFactory;
	AWSOnlineFactory = NULL;
}