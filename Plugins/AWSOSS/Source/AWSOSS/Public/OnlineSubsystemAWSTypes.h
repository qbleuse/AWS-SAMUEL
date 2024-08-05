// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"
#include "Logging/LogMacros.h"

class FOnlineSubsystemAWS;

// from OnlineSubsystemTypes.h
TEMP_UNIQUENETIDSTRING_SUBCLASS(FUniqueNetIdAWS, AWS_SUBSYSTEM);

// define log category and log method
#define UE_LOG_ONLINE_UC(Verbosity, Format, ...) \
{ \
	UE_LOG(LogOnline, Verbosity, TEXT("%s%s"), ONLINE_LOG_PREFIX, *FString::Printf(Format, ##__VA_ARGS__)); \
}


/*
* Implementation of Internet Address. heavily based on the FInternetAddrDemo, as this will do minimum work.
*/
class FInternetAddrAWS : public FInternetAddr
{
public:

	FInternetAddrAWS()
	{
	}

	virtual TArray<uint8> GetRawIp() const override
	{
		UE_LOG_ONLINE_UC(Error, TEXT("FInternetAddrAWS::GetRawIp() is not supported, not implemented, and should not be used"));
		unimplemented()
		return TArray<uint8>();
	}

	virtual void SetRawIp(const TArray<uint8>& RawAddr) override
	{
		UE_LOG_ONLINE_UC(Error, TEXT("FInternetAddrAWS::SetRawIp(const TArray<uint8>&) is not supported, not implemented, and should not be used"));
		unimplemented()
	}

	void SetIp(uint32 InAddr) override
	{
		UE_LOG_ONLINE_UC(Error, TEXT("FInternetAddrAWS::SetIp(uint32&) is not supported, not implemented, and should not be used"));
		unimplemented()
	}


	void SetIp(const TCHAR* InAddr, bool& bIsValid) override
	{
		IpAddr = InAddr;
		bIsValid = true;
	}

	void GetIp(uint32& OutAddr) const override
	{
		UE_LOG_ONLINE_UC(Error, TEXT("FInternetAddrAWS::GetIp(uint32&) is not supported, not implemented, and should not be used"));
		unimplemented()
	}

	void SetPort(int32 InPort) override
	{
		Port = InPort;
	}


	void GetPort(int32& OutPort) const override
	{
		OutPort = Port;
	}


	int32 GetPort() const override
	{
		return Port;
	}

	void SetAnyAddress() override
	{
	}

	void SetBroadcastAddress() override
	{
	}

	void SetLoopbackAddress() override
	{
	}

	FString ToString(bool bAppendPort) const override
	{
		if (bAppendPort)
			return IpAddr + FString(TEXT(":")) + FString::Printf(TEXT("%d"), Port);
		else
			return IpAddr;
	}

	virtual bool operator==(const FInternetAddr& Other) const override
	{
		return Other.ToString(true) == ToString(true);
	}

	bool operator!=(const FInternetAddrAWS& Other) const
	{
		return !(FInternetAddrAWS::operator==(Other));
	}

	virtual uint32 GetTypeHash() const override
	{
		return GetConstTypeHash();
	}

	uint32 GetConstTypeHash() const
	{
		return ::GetTypeHash(ToString(true));
	}

	friend uint32 GetTypeHash(const FInternetAddrAWS& A)
	{
		return A.GetConstTypeHash();
	}

	virtual bool IsValid() const override
	{
		return !IpAddr.IsEmpty() && Port > 0;
	}

	virtual TSharedRef<FInternetAddr> Clone() const override
	{
		return MakeShared<FInternetAddrAWS>(*this);
	}

	FString IpAddr;
	int32 Port = 0;
};

/**
 * Implementation of session information.
 * Implemented for agnostic handling of network connection.
 */
class FOnlineSessionInfoAWS : public FOnlineSessionInfo
{
protected:

	/** Hidden on purpose */
	FOnlineSessionInfoAWS(const FOnlineSessionInfoAWS& Src)
	{
	}

	/** Hidden on purpose */
	FOnlineSessionInfoAWS& operator=(const FOnlineSessionInfoAWS& Src)
	{
		return *this;
	}

public:

	/** Constructor */
	FOnlineSessionInfoAWS();

	/**
	 * Initialize an AWS session info with the address and port of the created session from gamelift
	 * and an id for the session (here we choose to use the PlayerSessionID rather than the Game Session, as this is closer to the connection info,
	 * debatable wether we should take this one or another.)
	 */
	void Init(const FOnlineSubsystemAWS& Subsystem, const FString& IpAddress, const FString& Port, const FString& PlayerSessionId);

	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FUniqueNetIdAWS SessionId;

public:

	virtual ~FOnlineSessionInfoAWS() {}

	bool operator==(const FOnlineSessionInfoAWS& Other) const
	{
		return false;
	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"),
			HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"),
			*SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};
