#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FAWSOSSModule : public IModuleInterface
{
	public:	
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:

		/** Class responsible for creating instance(s) of the subsystem */
		class FOnlineFactoryAWS* AWSOnlineFactory {nullptr};
};