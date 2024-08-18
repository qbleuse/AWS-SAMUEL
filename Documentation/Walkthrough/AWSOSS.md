# AWS OSS and Http Request

Here, we will dive in how the AWS OSS is built and how it communicates with AWS.

## Table Of Contents

- [AWS OSS and Http Request](#aws-oss-and-http-request)
  - [Table Of Contents](#table-of-contents)

## Create an OSS Plugin

To create an OSS Plugin, I mostly copied the source code available in the Engine's Plugins, let's look at it step by step.

Firstly, the Online SubSystem is a SubSystem, meaning in the Module definition it should be registered as one.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.h#L6) is the declaration of the Module.

```cpp
class FAWSOSSModule : public IModuleInterface
{
	public:	
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

	private:

		/** Class responsible for creating instance(s) of the subsystem */
		class FOnlineFactoryAWS* AWSOnlineFactory {nullptr};
};
```

The module is a simple one , but in the declaration, it needs the factory to register our OSS in the array of platform available.

[Here](../../Plugins/AWSOSS/Source/AWSOSS/Private/AWSOSSModule.cpp#L13) is what is looks like.

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
```

Note that the factory is newed and deleted, meaning it is not part of the Garbage Collection.

Otherwise, I think it is pretty straightforward ; creating the OSS, initializing it, and registering it if it succeeded.

## Initializing the Online Subsystem

Now, [here](../../Plugins/AWSOSS/Source/AWSOSS/Public/OnlineSubsystemAWS.h#L28) is what the Online Subsystem looks like.

```cpp
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
```

For ease of implementation, it inherits from OnlineSubsystemNull so that most other interface inside the OnlineSubsystem just falls back on the Null Interfaces.

The only one that changes is the Session Interface, which we change so that our OnlineSessionAWSInterface is used instead of the Null one.
We still keep the Null one for fallback on it in case the session is for LAN.

The only other members of this class are about the http requests.

Let us see their initialization in the class' [Init method](../../Plugins/AWSOSS/Source/AWSOSS/Private/OnlineSubsystemAWS.cpp#L6)

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

Quite straightforward, the members of the class are being initialized.

Fortunately, to easily send Http Requests, Unreal has an Engine Plugin for that and you just need to add it  to your [.Build.cs](../../Plugins/AWSOSS/Source/AWSOSS.Build.cs) in the DependencyModuleNames.

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
		"HTTP",// <-- the line you should look at.
		"Json",
		"Sockets"
      });
```

With this, we'll be able to create, format, and send http requests easily.
And we just need to access to the Module to use it, so we save it as a member of the OSS.

But to send http requets, we need a URI (or URL).
For this, I decided to imitate what the other OSS do when you need to configure stuff inside the OSS : I used to make it configurable inside the [DefaultEngine.ini](../../Config/DefaultEngine.ini) file, inside an "OnlineSubsystemAWS" section.

```ini
[OnlineSubsystem]
DefaultPlatformService=AWS

[OnlineSubsystemAWS]
;APIGatewayURL needs to be between quotation marks if it has numbers in it (as it is a URL)
APIGatewayURL="http://127.0.0.1:3000"
StartSessionURI=/create_session
FindSessionURI=/find_session
JoinSessionURI=/join_session
```

That allows for configuration of other URLs in different scenarios, and also is similar to the way you need to confgigure OSS anyway.

As for the URL, the one problem of the implementation is that it uses a single URL for all calls to AWS, whereas in a real implementation, you may want to send a GET request to this first URL to get your own regions's URL, to get better connectivity.

And in this method we also create, and thus, initialize our Session Interface.

## Initialize the AWS Session Interface

