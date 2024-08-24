# Walkthrough : Design

## The Design Philosophy

The idea behind the original plugin was to make something "easy and fast" : as any good programmer, I was lazy.

But I wanted a tool that could be used by anyone that knows Unreal a bit.

So I chose to try to integrate Gamelift connection (that until then, tutorial showed only how to make it with distant gamelift, and doing it directly), by using the familiar Online SubSystem Interface, and make it use local solution (like Gamelift Anywhere, or at the time the legacy SDK, as this was the only one out).

Also, It is written on the README, but for the school project, I had no choice but to use AWS, and we did not have credit, so all server-side development, needed to be done locally, which is what motivated for the development of a locally tested, but easily server reproductible (for when we would have the credits) setup.

Let us go over and present the tools for the solution

## Unreal's Online SubSystem

Unreal's Online SubSystem (OSS), is an attempt by Unreal's team to make a platform agnostic Interface for all communication with server back-ends, that does not revolve around replication and game logic.

It would be for thing like accessing databases, making voicechat, or connecting clients to server's game session, what interests us today.

Let's have a short look a what that looks like

```cpp
/**
 *	OnlineSubsystem - Series of interfaces to support communicating with various web/platform layer services
 */
class ONLINESUBSYSTEM_API IOnlineSubsystem
{
protected:
	/** Hidden on purpose */
	IOnlineSubsystem() {}

	FOnlineNotificationHandlerPtr OnlineNotificationHandler;
	FOnlineNotificationTransportManagerPtr OnlineNotificationTransportManager;

public:
	
	virtual ~IOnlineSubsystem() {}

    ...

	/** 
	 * Determine if the subsystem for a given interface is enabled by config and command line
	 * @param SubsystemName - Name of the requested online service
	 * @return true if the subsystem is enabled by config
	 */
	static bool IsEnabled(const FName& SubsystemName, const FName& InstanceName = NAME_None);

	/**
	 * Return the name of the subsystem @see OnlineSubsystemNames.h
	 *
	 * @return the name of the subsystem, as used in calls to IOnlineSubsystem::Get()
	 */
	virtual FName GetSubsystemName() const = 0;

	/**
	 * Get the instance name, which is typically "default" or "none" but distinguishes
	 * one instance from another in "Play In Editor" mode.  Most platforms can't do this
	 * because of third party requirements that only allow one login per machine instance
	 *
	 * @return the instance name of this subsystem
	 */
	virtual FName GetInstanceName() const = 0;

	/**
	 * Get the local online platform based on compile time determination of hardware.
	 * @see OnlineSubsystemNames.h OSS_PLATFORM_NAME_*
	 * @return string representation of online platform name
	 */
	static FString GetLocalPlatformName();

	/** @return true if the subsystem is enabled, false otherwise */
	virtual bool IsEnabled() const = 0;

	/** 
	 * Get the interface for accessing the session management services
	 * @return Interface pointer for the appropriate session service
	 */
	virtual IOnlineSessionPtr GetSessionInterface() const = 0;
	
	/** 
	 * Get the interface for accessing the player friends services
	 * @return Interface pointer for the appropriate friend service
	 */
	virtual IOnlineFriendsPtr GetFriendsInterface() const = 0;

	...
};
```

The class definition is very long, so I have replaced parts with "...", to make it shorter.
The idea is simple, a long class with methods to override, and use inheritance and polymorphism to use your specific back-end while using the same object.


Here is an example of a child class, the OSS Null that implements LAN features

```cpp
/**
 *	OnlineSubsystemNull - Implementation of the online subsystem for Null services
 */
class ONLINESUBSYSTEMNULL_API FOnlineSubsystemNull : 
	public FOnlineSubsystemImpl
{

public:

	virtual ~FOnlineSubsystemNull() = default;

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;	
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	virtual IOnlineTournamentPtr GetTournamentInterface() const override;
	virtual IMessageSanitizerPtr GetMessageSanitizer(int32 LocalUserNum, FString& OutAuthTypeToExclude) const override;

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;

	// FTickerObjectBase
	
	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemNull

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemNull() = delete;
	explicit FOnlineSubsystemNull(FName InInstanceName) :
		FOnlineSubsystemImpl(NULL_SUBSYSTEM, InInstanceName),
		SessionInterface(nullptr),
		VoiceInterface(nullptr),
		bVoiceInterfaceInitialized(false),
		LeaderboardsInterface(nullptr),
		IdentityInterface(nullptr),
		AchievementsInterface(nullptr),
		StoreV2Interface(nullptr),
		MessageSanitizerInterface(nullptr),
		OnlineAsyncTaskThreadRunnable(nullptr),
		OnlineAsyncTaskThread(nullptr)
	{}

private:

	/** Interface to the session services */
	FOnlineSessionNullPtr SessionInterface;

	/** Interface for voice communication */
	mutable IOnlineVoicePtr VoiceInterface;

	/** Interface for voice communication */
	mutable bool bVoiceInterfaceInitialized;

	/** Interface to the leaderboard services */
	FOnlineLeaderboardsNullPtr LeaderboardsInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityNullPtr IdentityInterface;

	/** Interface for achievements */
	FOnlineAchievementsNullPtr AchievementsInterface;

	/** Interface for store */
	FOnlineStoreV2NullPtr StoreV2Interface;

	/** Interface for purchases */
	FOnlinePurchaseNullPtr PurchaseInterface;

	/** Interface for message sanitizing */
	FMessageSanitizerNullPtr MessageSanitizerInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerNull* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	// task counter, used to generate unique thread names for each task
	static FThreadSafeCounter TaskCounter;
};
```

Simple enough. Everything is overriden and all the interface for the features exists in the class.

A lot of platform have their implementation : Epic Online Services (arguably the better working one), Steam (working for most of what is important, but buggy on some features), IOS, Android...

But then, if all those platforms have implementation, AWS should have one, right ?
The answer is yes ! The Amazon OSS exists, let's take a look at it.

```cpp

/**
 * Amazon subsystem
 */
class ONLINESUBSYSTEMAMAZON_API FOnlineSubsystemAmazon :
	public FOnlineSubsystemImpl
{
	class FOnlineFactoryAmazon* AmazonFactory;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityAmazonPtr IdentityInterface;

	/** Used to toggle between 1 and 0 */
	int TickToggle;

PACKAGE_SCOPE:

	/** Only the factory makes instances */
	FOnlineSubsystemAmazon() = delete;
	explicit FOnlineSubsystemAmazon(FName InInstanceName);

public:
	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override { return nullptr; }
	virtual IOnlineFriendsPtr GetFriendsInterface() const override { return nullptr; }
	virtual IOnlinePartyPtr GetPartyInterface() const override { return nullptr; }
	virtual IOnlineGroupsPtr GetGroupsInterface() const override { return nullptr; }
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override { return nullptr; }
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override { return nullptr; }
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override { return nullptr; }
	virtual IOnlineVoicePtr GetVoiceInterface() const override { return nullptr; }
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override { return nullptr; }
	virtual IOnlineTimePtr GetTimeInterface() const override { return nullptr; }
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override { return nullptr; }
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override { return nullptr; }
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override { return nullptr; }
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override { return nullptr; }
	virtual IOnlineEventsPtr GetEventsInterface() const override { return nullptr; }
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override { return nullptr; }
	virtual IOnlineSharingPtr GetSharingInterface() const override { return nullptr; }
	virtual IOnlineUserPtr GetUserInterface() const override { return nullptr; }
	virtual IOnlineMessagePtr GetMessageInterface() const override { return nullptr; }
	virtual IOnlinePresencePtr GetPresenceInterface() const override { return nullptr; }
	virtual IOnlineChatPtr GetChatInterface() const override { return nullptr; }
	virtual IOnlineStatsPtr GetStatsInterface() const override { return nullptr; }
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override { return nullptr; }
	virtual IOnlineTournamentPtr GetTournamentInterface() const override { return nullptr; }

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;
	virtual bool IsEnabled() const override;

	// FTickerBaseObject

	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemAmazon

	/**
	 * Destructor
	 */
	virtual ~FOnlineSubsystemAmazon();
};
```

It only implements the Indentity Interface.
It may sound harsh, but this is utterly useless.

There was also an attempt to do one in 2016, and is now depreceated.
You may [look at it](https://github.com/gameDNAstudio/AmazonServices) if you are intered.

But why not make it more useful then ? I have a theory.
Just AWS design phylososphy may simply not work withg the limiting integration OSS requires.

For our purpose, I actually have to only override the Session Interface so that is not that far away from only implementing the Identity Interface, but as I just want something that works and that I could use familiarly, this is enough for me.

Now that we understand better the client side system, let's look at what our server looks like.

## Amazon Web Services

Amazon Web Services (AWS for short), is both, a server and a server developement solution, that bases itself on two architecture principles : [Event-Driven Architecture](https://aws.amazon.com/event-driven-architecture/) and [Infrastructure as a Service (Iaas)](https://aws.amazon.com/fr/what-is/iaaS/)/[Cloud Computing](https://aws.amazon.com/what-is-cloud-computing/?nc1=h_ls).

If you want to know more about AWS's Framework, [here is their documentation](https://docs.aws.amazon.com/en_us/wellarchitected/2022-03-31/framework/welcome.html) (though a bit old even at the time of writing).

In short, AWS works by having everything being a very small piece, and to connect these little tools that do one thing well to create your own custom solution.

This is a philosophy akin to the ["Do One Thing And Do It Well" philosophy](https://en.wikipedia.org/wiki/Unix_philosophy) from the UNIX community.

The most stricking example of this is [Lambdas](https://aws.amazon.com/lambda/)

These are functions that can be called from everywhere, and that should not be more than 10 lines of code to execute, the idea being that lambda can call other lambdas and that, because of the event-driven architecture, it is not a server waiting to execute the code, just a function that will be called when asked for, on AWS's own server.

The main benefits resides in its ability to [horizontally scale](https://en.wikipedia.org/wiki/Database_scalability).

Simply put, as everything in AWS is a service (would it be to execute functions with lambda or to host a game server for a period of time with gamelift), and that you do not have to create your own server, if demand suddenly increase or decrease, with a service-based event-driven architecture, usage of resources follows demand, and you only pay what you use.

Now, that is the sales pitch, in practice it is way harder to actually configure something that closely follows demand, and does not lose in computing time.

So in our case, what does our usage of AWS looks like ?

[Here](https://docs.aws.amazon.com/architecture-diagrams/latest/multiplayer-session-based-game-hosting-on-aws/multiplayer-session-based-game-hosting-on-aws.html) is what AWS pictures on its website.

![multiplayer-session-based-game-hosting-on-aws.png](../Media/multiplayer-session-based-game-hosting-on-aws.png)

That is quite a complete picture of the full capabilities of AWS.
Here there is a lot of feature : user account connection, database, matchmaking, game data and analytics, etc...

But this is a bit too much for our simple aplication that just want the same features than a LAN connection but on AWS.

If we strip all of the "unnecessary", this will become this.

![our architecture](../Media/stripped-out-multiplayer-session-based-game-hosting-on-aws.png)

Here, everything but the client is in the "AWS Cloud".
Let's see the components of our solution one by one.

1. <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client and <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway communication.

AWS is quite strict security-wise, so we cannot directly communicate with the block inside our solution (such as gamelift), to get a server.
In the case of communicate with a game session directly, you need to ask gamelift to open a public port in one of this fleet to communicate with said client temporarily.

The only access for public connection with AWS is called [API Gateway](https://aws.amazon.com/api-gateway/), which like its name suggests, is a gateway for accessing everything in AWS Cloud, from outside  of it.

API Gateway can be both a Rest or WebSocket API, but what we'll use it as is just a way to communicate with AWS through http requests.

2. <img src="../Media/Arch_Amazon-API-Gateway_48.svg" alt="drawing" width="24"/> API Gateway to <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambda execution

Unfortunately, we cannot create an event on API Gateway to do all the setup we need for our server then respond the necessary data.
We'll need to make function and to do it programatically.
So in our solution, API Gateway is just here to execute the necessary [Lambdas](https://aws.amazon.com/lambda/), wich are functions that can do almost everything inside the AWS Cloud.

As this is supposed to run locally, there will be no consideration for security, but you sould definitely look into it if you intend to ship your solution.

3. <img src="../Media/Arch_AWS-Lambda_48.svg" alt="drawing" width="24"/> Lambdas interacting <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> with AWS Gamelift
   
In the execution of the lambdas, using the tools given by AWS, we will be able to create games sessions, player sessions, and look up in the resources of [Gamelift](https://aws.amazon.com/gamelift/) programatically.

These resources will be formatted as an http response to make it available to the client.

4. <img src="../Media/Arch_Amazon-GameLift_48.svg" alt="drawing" width="24"/> Gamelift initializing instances on its <img src="../Media/Res_Amazon-EC2_Instances_48.svg" alt="drawing" width="24"/> Fleet

Fleet are resources on gamelift, usually synonymous to a server in a region in the world.

For our use case, we define one as "ourselves" (basically redirecting to "localhost"). if you did not do that see [how to make the project run](../Usage/Run.md).

5. <img src="../Media/Res_Amazon-EC2_Instances_48.svg" alt="drawing" width="24"/> Fleet's game session connecting directly with <picture> <source media="(prefers-color-scheme: dark)" srcset="../Media/Dark/Res_User_48_Dark.svg"> <source media="(prefers-color-scheme: light)" srcset="../Media/Light/Res_User_48_Light.svg"> <img alt="" src="../Media/Light/Res_User_48_Light.svg" width="24"> </picture> client

After having been assigned a game session, client directly connects to the IP address and port of the server, that is opened for a little amount of time.

In our case, the ip address is localhost (or 127.0.0.1) meaning it redirects our network connection to search on the client's device for the server which should already be opened on a certain port.

This is a cumbersome solution to simply redirects client to a server that is on the same machine, but unfortunately, this is how works connection on AWS.
And for good reasons, as this system is easily expandable to work on multiple servers worldwide, even if that is outside our scope.

## In details

Now that we've covered the general design of the solution, let's dive into how it works, and cover the three main methods implemented in the Plugin.

- [Make the AWSOSS launch http requests to API Gateway](AWSOSS.md)
- [Create Session](CreateSession.md)
- [Find Session](FindSession.md)
- [Join Session](JoinSession.md)
