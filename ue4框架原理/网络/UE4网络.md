# 基本网络类

## FSocket

最基本的类，套接字，与IP和Port联系，提供绑定，监听，连接，关闭等接口。

基本信息:

```c++
	/** Indicates the type of socket this is */
	const ESocketType SocketType;//有none，udp，tcp三种
	/** Debug description of socket usage. */
	FString SocketDescription;
	/** Protocol used in creation of a socket */
	FName SocketProtocol;//所用协议
```

相关接口

```c++
	virtual bool Shutdown(ESocketShutdownMode Mode) = 0;	
	virtual bool Close() = 0;
	virtual bool Bind(const FInternetAddr& Addr) = 0;
	virtual bool Connect(const FInternetAddr& Addr) = 0;
	virtual bool Listen(int32 MaxBacklog) = 0;
	virtual class FSocket* Accept(const FString& InSocketDescription) = 0;
	virtual class FSocket* Accept(FInternetAddr& OutAddr, const FString& InSocketDescription) = 0;
	virtual bool SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination);
	virtual bool Send(const uint8* Data, int32 Count, int32& BytesSent);
	virtual bool RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None);
	virtual bool Recv(uint8* Data, int32 BufferSize, int32& BytesRead, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None);
	virtual bool Wait(ESocketWaitConditions::Type Condition, FTimespan WaitTime) = 0;
```

还有一些没列出来，总之提供一些底层的接口，相应实现在子类中，主要看一下BSDSocket的实现。

## BSDSocket

继承自FSocket。

多出的成员变量：

```c++
	/** Holds the BSD socket object. */
	SOCKET Socket;
	/** Last activity time. */
	double LastActivityTime;
	/** Pointer to the subsystem that created it. *///指向创建它的subsystem
	ISocketSubsystem* SocketSubsystem;
```

重写的操作：

```c++
	virtual bool Shutdown(ESocketShutdownMode Mode) override;
	virtual bool Close() override;
	virtual bool Bind(const FInternetAddr& Addr) override;
	virtual bool Connect(const FInternetAddr& Addr) override;
	virtual bool Listen(int32 MaxBacklog) override;
	virtual bool WaitForPendingConnection(bool& bHasPendingConnection, const FTimespan& WaitTime) override;
	virtual bool HasPendingData(uint32& PendingDataSize) override;
	virtual class FSocket* Accept(const FString& SocketDescription) override;
	virtual class FSocket* Accept(FInternetAddr& OutAddr, const FString& SocketDescription) override;
	virtual bool SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination) override;
	virtual bool Send(const uint8* Data, int32 Count, int32& BytesSent) override;
	virtual bool RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) override;
	virtual bool Recv(uint8* Data,int32 BufferSize,int32& BytesRead, ESocketReceiveFlags::Type Flags = ESocketReceiveFlags::None) override;
	virtual bool Wait(ESocketWaitConditions::Type Condition, FTimespan WaitTime) override;
	virtual ESocketConnectionState GetConnectionState() override;
	virtual void GetAddress(FInternetAddr& OutAddr) override;
	virtual bool GetPeerAddress(FInternetAddr& OutAddr) override;
	virtual bool SetNonBlocking(bool bIsNonBlocking = true) override;
	virtual bool SetNoDelay(bool bIsNoDelay = true) override;
	virtual bool SetBroadcast(bool bAllowBroadcast = true) override;
	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress) override;
	virtual bool JoinMulticastGroup(const FInternetAddr& GroupAddress, const FInternetAddr& InterfaceAddress) override;
	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress) override;
	virtual bool LeaveMulticastGroup(const FInternetAddr& GroupAddress, const FInternetAddr& InterfaceAddress) override;
	virtual bool SetMulticastLoopback(bool bLoopback) override;
	virtual bool SetMulticastTtl(uint8 TimeToLive) override;
	virtual bool SetMulticastInterface(const FInternetAddr& InterfaceAddress) override;
	virtual bool SetReuseAddr(bool bAllowReuse = true) override;
	virtual bool SetLinger(bool bShouldLinger = true, int32 Timeout = 0) override;
	virtual bool SetRecvErr(bool bUseErrorQueue = true) override;
	virtual bool SetSendBufferSize(int32 Size,int32& NewSize) override;
	virtual bool SetReceiveBufferSize(int32 Size,int32& NewSize) override;
	virtual int32 GetPortNo() override;
	bool SetIPv6Only(bool bIPv6Only);
```

### Shutdown

参数`ESocketShutdownMode`,三种值：

```c++
enum class ESocketShutdownMode
{
	/**
	 * Disables reading on the socket.
	 */
	Read,
	/**
	 * Disables writing on the socket.
	 */
	Write,
	/**
	 * Disables reading and writing on the socket.
	 */
	ReadWrite
};
```

分别用于不让读，不让写，不让读写。函数内部根据mode的不同调用底层`shutdown`函数。`return shutdown(Socket, InternalMode) == 0;`

### Close

调用底层函数`closesocket`。`closesocket`中优雅的关闭。

```c++
	//SocketSubsystemBSDPrivate.h
	
	inline int32 closesocket(SOCKET Socket)
	{
		//优雅的关闭
		shutdown(Socket, SHUT_RDWR); // gracefully shutdown if connected
		return close(Socket);
	}
```

### Connect

内部调用底层connect

### Listen

调用底层listen同时返回布尔型表示是否监听

### WaitForPendingConnection

内部会调用`HasState`函数，状态传入为CanRead，此函数其实是`select()`的封装，让平台用这个替换。

内部其实就是根据传入的state转调用select，注意此函数值内select只针对一个socket。

### HasPendingData

确定socket可以读并且有数据时，返回true；

### Accept

重载函数，有两个，一个无外来地址，一个有。

```c++
FSocket* FSocketBSD::Accept(const FString& InSocketDescription)
{
	//获取一个全新描述符
	SOCKET NewSocket = accept(Socket, NULL, NULL);
	FSocketSubsystemBSD* BSDSystem = static_cast<FSocketSubsystemBSD*>(SocketSubsystem);
	return BSDSystem->InternalBSDSocketFactory(NewSocket, SocketType, InSocketDescription, SocketProtocol);
}
FSocket* FSocketBSD::Accept(FInternetAddr& OutAddr, const FString& InSocketDescription)
{
	FInternetAddrBSD& BSDAddr = static_cast<FInternetAddrBSD&>(OutAddr);
	SOCKLEN SizeOf = sizeof(sockaddr_storage);
	SOCKET NewSocket = accept(Socket, (sockaddr*)&(BSDAddr.Addr), &SizeOf);
	FSocketSubsystemBSD* BSDSystem = static_cast<FSocketSubsystemBSD*>(SocketSubsystem);
	return BSDSystem->InternalBSDSocketFactory(NewSocket, SocketType, InSocketDescription, BSDAddr.GetProtocolType());
}
```

一个仅仅是获取一个全新描述符，另一个则是获取一个有连接地址的socket。

注意accept仅在TCP服务器中调用。

### SendTo

```c++
bool FSocketBSD::SendTo(const uint8* Data, int32 Count, int32& BytesSent, const FInternetAddr& Destination)
```

向目的地发送数据。底层调用sendto。

```c++
// Write the data and see how much was written
BytesSent = sendto(Socket, (const char*)Data, Count, 0, (const sockaddr*)&(BSDAddr.Addr), BSDAddr.GetStorageSize());
```

注意此函数更新了`LastActivityTime`：

```c++
if (Result)
{
	LastActivityTime = FPlatformTime::Seconds();
}
```

注意有地址参数，所以一般用于UDP中。发送数据的主要函数。

主要逻辑：发送给定数量的数据，查看真正发送的数据是否>0,是则返回true，更新LastActivityTime。

### Send

内部逻辑相同，不需要目的地址。(TCP)

### RecvFrom

从目的地址接收数据。

```c++
bool FSocketBSD::RecvFrom(uint8* Data, int32 BufferSize, int32& BytesRead, FInternetAddr& Source, ESocketReceiveFlags::Type Flags)
```

调用recvfrom进行数据读，并对实际读的数量进行检测，返回是否读成功并且通过参数返回读的数量，更新`LastActivityTime`。

### Recv

内部逻辑与RecvFrom相同。

### Wait

根据condition不同调用HasState，即调用select，等待条件成立。

### GetConnectionState

通过HasState获取state。如Error，Connected，NotConnected。

### GetAddress

调用`getsockname`,获取与套接字关联的本地协议地址。

### GetPeerAddress

调用`getpeername`，获取与套接字关联的外地协议地址。

其他的就不再看了，用的时候再看。

## ISocketSubsystem

作为一个基类，具体的SocketSubsystem还是要根据对应的平台而不同。

在SocketSubsystem的基础上还封装了一层Module来管理。因此，在游戏启动的时候，会先加载SocketSubsystemModule，然后触发对应平台的SocketSubsystem的创建，再执行对应的BSDSocket的创建。

## FOnlineSubsystemModule

加载此Module时，执行完该Module后，会调用一次StartupModule，在不同的Module中实现不同，

```c++
void FOnlineSubsystemModule::StartupModule()
{
	// These should not be LoadModuleChecked because these modules might not exist
	// Load dependent modules to ensure they will still exist during ShutdownModule.
	// We will always load these modules at the cost of extra modules loaded for the few OSS (like Null) that don't use it.
	if (FModuleManager::Get().ModuleExists(TEXT("HTTP")))
	{
		FModuleManager::Get().LoadModule(TEXT("HTTP"));
	}
	if (FModuleManager::Get().ModuleExists(TEXT("XMPP")))
	{
		FModuleManager::Get().LoadModule(TEXT("XMPP"));
	}
	ProcessConfigDefinedModuleRedirects();
	LoadDefaultSubsystem();	
	// Also load the console/platform specific OSS which might not necessarily be the default OSS instance
	FString InterfaceString;
	GConfig->GetString(TEXT("OnlineSubsystem"), TEXT("NativePlatformService"), InterfaceString, GEngineIni);
	NativePlatformService = FName(*InterfaceString);
	ProcessConfigDefinedSubsystems();
	IOnlineSubsystem::GetByPlatform();
}
```

首先会读取配置信息。然后加载在配置文件中指定的默认subsystem。读取在配置文件中指定的配置。获取当前硬件的本机联机子系统。

# 网络核心类

NetDriver和Connection,并不完全是上下层的关系，更像是互相依托的关系。实际应用中，会在NetDriver基础上进一步封装出子类IPNetDriver。

在每个单独的UE进程中，只会有一个NetDriver实例，具体是IPNetDriver。简单来说，构建NetDriver，然后注册NetConnection，在NetConnection注册ActorChannel，ControlChannel,VoiceChannel。更新的时候，从world触发tick，然后触发到netdriver，然后逐个触发上面的connection。netdriver和netconnection中会有一些和RPC相关的操作接口，主要的RPC接口还是在UChannel上。

## NetDriver

### 主要变量

#### ServerConnection

```c++
	UPROPERTY()
	class UNetConnection* ServerConnection;
```

对服务器的connection，只有在客户端才会赋值。

#### ClientConnections

```c++
	/** Array of connections to clients (this net driver is a host) - unsorted, and ordering changes depending on actor replication */
	UPROPERTY()
	TArray<UNetConnection*> ClientConnections;
```

连接到服务器的所有clientconnection，在服务器维护。注意是一个数组，无序，actor的赋值会导致顺序改变。

####  ConnectionlessHandler

```c++
	/** Serverside PacketHandler for managing connectionless packets */
	TUniquePtr<PacketHandler> ConnectionlessHandler;
```

服务端的PacketHandler用以管理无连接的packets。

#### StatelessConnectComponent

```c++
	/** Reference to the PacketHandler component, for managing stateless connection handshakes */
	TWeakPtr<StatelessConnectHandlerComponent> StatelessConnectComponent;
```

#### World

对PacketHandler组建的引用，管理无状态连接的握手。

```c++
	/** World this net driver is associated with */
	UPROPERTY()
	class UWorld* World;
```

netdriver所关联的world。

#### ChannelDefinitionMap

```c++
	/** Used for faster lookup of channel definitions by name. */
	UPROPERTY()
	TMap<FName, FChannelDefinition> ChannelDefinitionMap;
```

通过名字可以快速查找channel的定义。

#### RepChangedPropertyTrackerMap

```c++
	/** Maps FRepChangedPropertyTracker to active objects that are replicating properties */
	TMap<UObject*, FRepChangedPropertyTrackerWrapper>	RepChangedPropertyTrackerMap;
```

存储正在赋值属性的对象所对应的`FRepChangedPropertyTracker`。

#### RepLayoutMap

```c++
	/** Maps FRepLayout to the respective UClass */
	TMap< TWeakObjectPtr< UObject >, TSharedPtr< FRepLayout > >					RepLayoutMap;
```

注意其中的**FRepLayout**,这个类保存一个给定类型的所有属性，很多辅助函数帮助读写和属性状态对比。

具体使用可看注释，注释比较详细。

#### FReplicationChangelistMgrWrapper

```c++
	/** Maps an object to the respective FReplicationChangelistMgr */
	TMap< UObject*, FReplicationChangelistMgrWrapper >	ReplicationChangeListMap;
```

#### DDoS

	//DDoS防御管理
	/** DDoS detection management */
	FDDoSDetection DDoS;
#### NetworkObjects

```c++
/** Stores the list of objects to replicate into the replay stream. This should be a TUniquePtr, but it appears the generated.cpp file needs the full definition of the pointed-to type. */
TSharedPtr<FNetworkObjectList> NetworkObjects;
```
存储要复制到重播流中的对象列表

### 主要函数

#### 构造函数

```c++
	// Constructors.
	ENGINE_API UNetDriver(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
```

主要是对类内成员初始化。

#### UObject接口函数

```c++
	//~ Begin UObject Interface.
	ENGINE_API virtual void PostInitProperties() override;
	ENGINE_API virtual void PostReloadConfig(FProperty* PropertyToLoad) override;//重新加载配置
	ENGINE_API virtual void FinishDestroy() override;//销毁操作
	ENGINE_API virtual void Serialize( FArchive& Ar ) override;//自身的序列化函数，接收访问者archive
	ENGINE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);//允许对象注册它自身的引用（还没有被token stream覆盖到的话）
	//~ End UObject Interface.
```

##### PostInitProperties

初始化相关属性。注册相关level委托：

```c++
		OnLevelRemovedFromWorldHandle = FWorldDelegates::LevelRemovedFromWorld.AddUObject(this, &UNetDriver::OnLevelRemovedFromWorld);
		OnLevelAddedToWorldHandle = FWorldDelegates::LevelAddedToWorld.AddUObject(this, &UNetDriver::OnLevelAddedToWorld);
		PostGarbageCollectHandle = FCoreUObjectDelegates::GetPostGarbageCollect().AddUObject(this, &UNetDriver::PostGarbageCollect);
```

#### Init相关函数

```c++
//在server和client之间建立连接的常规初始化
	ENGINE_API virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error);
//在客户端模式下的初始化net driver
	ENGINE_API virtual bool InitConnect(class FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error ) PURE_VIRTUAL( UNetDriver::InitConnect, return true;);

//在服务器模式下的初始化(监听模式)
	ENGINE_API virtual bool InitListen(class FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error) PURE_VIRTUAL( UNetDriver::InitListen, return true;);

//从当前world初始化已销毁的网络启动时的actor列表
	/** Initialize the list of destroyed net startup actors from the current World */
	ENGINE_API virtual void InitDestroyedStartupActors();

//初始化服务器这边的PacketHandler,用于处理无连接的packets
	ENGINE_API virtual void InitConnectionlessHandler();

//刷新所有PacketHandler所排队的所有packets
	ENGINE_API virtual void FlushHandler();


	//初始化网络连接类用于新的连接
	/** Initializes the net connection class to use for new connections */
	ENGINE_API virtual bool InitConnectionClass(void);

//初始化复制驱动类
	/** Initialized the replication driver class to use for this driver */
	ENGINE_API virtual bool InitReplicationDriverClass();

//关闭此netdriver所管理的所有连接
	/** Shutdown all connections managed by this net driver */
	ENGINE_API virtual void Shutdown();
//关闭socket，释放操作系统分配的内存
	/* Close socket and Free the memory the OS allocated for this socket */
	ENGINE_API virtual void LowLevelDestroy();
```

#### ServerReplicateActors

复制actor到客户端的主函数。在TickFlush中进行调用，每一帧都会执行。

有四个此函数的辅助函数:

```c++
	int32 ServerReplicateActors_PrepConnections( const float DeltaSeconds );
	void ServerReplicateActors_BuildConsiderList( TArray<FNetworkObjectInfo*>& OutConsiderList, const float ServerTickTime );
	int32 ServerReplicateActors_PrioritizeActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, const TArray<FNetworkObjectInfo*> ConsiderList, const bool bCPUSaturated, FActorPriority*& OutPriorityList, FActorPriority**& OutPriorityActors );
	int32 ServerReplicateActors_ProcessPrioritizedActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, FActorPriority** PriorityActors, const int32 FinalSortedCount, int32& OutUpdated );
```

##### ServerReplicateActors_PrepConnections

获取已经准备好的连接数。

##### ServerReplicateActors_BuildConsiderList

生成ConsiderList

##### ServerReplicateActors_PrioritizeActors

优先级排序？

##### ServerReplicateActors_ProcessPrioritizedActors

处理优先级actor

主要逻辑如下：

1. 获取所有客户端连接数。

#### ProcessRemoteFunction

处理远端的函数调用，即处理RPC函数的一个接口。

#### InternalProcessRemoteFunction

内部处理RPC的

#### ProcessRemoteFunctionForChannel

#### Tick相关

##### TickDispatch

处理time更新，读并处理packets

##### PostTickDispatch

##### TickFlush

复制actor并刷新

##### PostTickFlush

#### LowLevelSend

将无连接数据报发送到指定地址。

#### ProcessLocalServerPackets

处理本地的需要发送到客户端的talker包

#### ProcessLocalClientPackets

处理本地的需要发送到服务器的talker包

#### ReplicateVoicePacket

进行语音包的网络复制。

#### CMD处理

```c++
bool HandleSocketsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandlePackageMapCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandleNetFloodCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandleNetDebugTextCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandleNetDisconnectCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandleNetDumpServerRPCCommand( const TCHAR* Cmd, FOutputDevice& Ar );
bool HandleNetDumpDormancy( const TCHAR* Cmd, FOutputDevice& Ar );
void HandlePacketLossBurstCommand( int32 DurationInMilliseconds );
```
#### FlushActorDormancy

刷新处理睡眠状态的actor列表

#### ForcePropertyCompare

强制对actor的属性对比

#### ForceActorRelevantNextUpdate

强制actor至少一次更新变为相关的

#### AddNetworkActor

告诉netdriver，一个网络的actor产生了。

#### Notify相关

##### NotifyActorDormancyChange

actor睡眠状态改变时调用

##### NotifyActorDestroyed

当actor被销毁时调用

##### NotifyActorRenamed

当actor被重命名时调用

##### NotifyActorTearOff

##### NotifyActorLevelUnloaded

##### NotifyStreamingLevelUnload

#### CreateChild

添加一个child连接

## IpNetDriver

在实际的网络连接中，一般情况下使用的其实是UIpNetDriver进行实际操作。

### 主要变量

#### Socket

```c++
FSocket* Socket;
//创建和获取分别使用CreateSocket和GetSocket
```

### 主要接口函数

```c++
	virtual bool IsAvailable() const override;
	virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error) override;
	virtual bool InitConnect( FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error ) override;
	virtual bool InitListen( FNetworkNotify* InNotify, FURL& LocalURL, bool bReuseAddressAndPort, FString& Error ) override;
	virtual void TickDispatch( float DeltaTime ) override;
	virtual void LowLevelSend(TSharedPtr<const FInternetAddr> Address, void* Data, int32 CountBits, FOutPacketTraits& Traits) override;
	virtual FString LowLevelGetNetworkNumber() override;
	virtual void LowLevelDestroy() override;
	virtual class ISocketSubsystem* GetSocketSubsystem() override;
	virtual bool IsNetResourceValid(void) override
	{
		return GetSocket() != nullptr;
	}
```

注释可在父类netdriver中找到，功能可能多添加了一点。

## UNetConnection

### 主要变量:

#### Children

```c++
//connection的孩子
/** child connections for secondary viewports */
UPROPERTY(transient)
TArray<class UChildConnection*> Children;
```

孩子连接（对于次视口）

#### Driver

```c++
/** Owning net driver */
UPROPERTY()
class UNetDriver* Driver;	
```

当前connection所关联的driver

#### PackageMapClass

```c++
/** The class name for the PackageMap to be loaded */
UPROPERTY()
TSubclassOf<UPackageMap> PackageMapClass;
```

#### PackageMap

```c++
//local和remote之间的package映射
UPROPERTY()
/** Package map between local and remote. (negotiates net serialization) */
class UPackageMap* PackageMap;
```
#### OpenChannels

```c++
	/** @todo document */ //一个优化？详见下面ChannelsToTick
	UPROPERTY()
	TArray<class UChannel*> OpenChannels;//打开的通道
```

#### ChannelsToTick

```cpp
/**
 * The channels that need ticking. This will be a subset of OpenChannels, only including
 * channels that need to process either dormancy or queued bunches. Should be a significant
 * optimization over ticking and calling virtual functions on the potentially hundreds of
 * OpenChannels every frame.
 */
UPROPERTY()
TArray<UChannel*> ChannelsToTick;
```
#### SentTemporaries

```c++
	//这些actor是只有初始化packet会复制，之后不再复制
	/** This actor is bNetTemporary, which means it should never be replicated after it's initial packet is complete */
	UPROPERTY()
	TArray<class AActor*> SentTemporaries;
```

#### ViewTarget

```c++
//actor现在正在被viewed/控制 通过拥有的controller
/** The actor that is currently being viewed/controlled by the owning controller */
UPROPERTY()
class AActor* ViewTarget;
```
当前connection所关联的actor

#### OwningActor

```c++
/** Reference to controlling actor (usually PlayerController) */
UPROPERTY()
class AActor* OwningActor;
```
当前connection所关联的controller。

#### URL

```c++
	struct FURL			URL;				// URL of the other side.
```

#### RemoteAddr

connection的远程地址，通常从URL生成。

#### NumPacketIdBits

```c++
/** Number of bits used for the packet id in the current packet. */
int NumPacketIdBits;
```
#### NumBunchBits

```c++
/** Number of bits used for bunches in the current packet. */
int NumBunchBits;
```
#### NumAckBits

```c++
/** Number of bits used for acks in the current packet. */
int NumAckBits;
```
#### NumPaddingBits

```c++
/** Number of bits used for padding in the current packet. */
int NumPaddingBits;
```
#### State

连接状态

```c++
	EConnectionState	State;					// State this connection is in.
```

#### Handler

```c++
/** PacketHandler, for managing layered handler components, which modify packets as they are sent/received */
TUniquePtr<PacketHandler> Handler;
```
#### StatelessConnectComponent

```C++
/** Reference to the PacketHandler component, for managing stateless connection handshakes */
TWeakPtr<StatelessConnectHandlerComponent> StatelessConnectComponent;
```
#### bNeedsByteSwapping

该通道是否需要对所有数据进行字节交换

#### PlayerId

```c++
/** Net id of remote player on this connection. Only valid on client connections (server side).*/
UPROPERTY()
FUniqueNetIdRepl PlayerId;
```
#### 还有很多，遇到再添

### 主要函数

#### Actor通道的访问

##### RemoveActorChannel

##### AddActorChannel

##### FindActorChannelRef

##### FindActorChannel

##### ContainsActorChannel

##### ActorChannelsNum

##### ActorChannelConstIterator

##### ActorChannelMap

#### LowLevelSend

```c++
/**
 * Sends a byte stream to the remote endpoint using the underlying socket
 *
 * @param Data			The byte stream to send
 * @param CountBits		The length of the stream to send, in bits (to support bit-level additions to packets, from PacketHandler's)
 * @param Traits		Special traits for the packet, passed down from the NetConnection through the PacketHandler
 */
// @todo: Traits should be passed within bit readers/writers, eventually
ENGINE_API virtual void LowLevelSend(void* Data, int32 CountBits, FOutPacketTraits& Traits)
	PURE_VIRTUAL(UNetConnection::LowLevelSend,);
```
#### ValidateSendBuffer

验证FBitWriter确保其不处在错误的状态。

#### InitSendBuffer

重设FBitWriter到默认状态

#### FlushNet

```c++
/**
 * flushes any pending data, bundling it into a packet and sending it via LowLevelSend()
 * also handles network simulation settings (simulated lag, packet loss, etc) unless bIgnoreSimulation is true
 */
ENGINE_API virtual void FlushNet(bool bIgnoreSimulation = false);
```
刷新任何挂起的数据，将其绑定到一个包中并通过LowLevelSend()函数进行发送。

#### Tick

轮询连接，如果超时就关闭它。

#### IsNetReady

返回通道是否已经准备好发送。

#### HandleClientPlayer

处理player控制的客户端

#### Init函数

##### InitBase

对连接实例初始化一些设置

##### InitRemoteConnection

从一个远端源初始化一个连接实例

##### InitLocalConnection

初始化一个连接实例到一个远端源

##### InitConnection

通过传入的设置，初始化“无地址”的连接。

##### InitHandler

初始化PacketHandler

##### InitSequence

初始化连接顺序

#### 加密相关

```c++
/**
 * Sets the encryption key and enables encryption.
 */
UE_DEPRECATED(4.24, "Use SetEncryptionData instead.")
ENGINE_API void EnableEncryptionWithKey(TArrayView<const uint8> Key);

/**
 * Sets the encryption data and enables encryption.
 */
ENGINE_API void EnableEncryption(const FEncryptionData& EncryptionData);

/**
 * Sets the encryption key, enables encryption, and sends the encryption ack to the client.
 */
UE_DEPRECATED(4.24, "Use SetEncryptionData instead.")
ENGINE_API void EnableEncryptionWithKeyServer(TArrayView<const uint8> Key);

/**
 * Sets the encryption data, enables encryption, and sends the encryption ack to the client.
 */
ENGINE_API void EnableEncryptionServer(const FEncryptionData& EncryptionData);

/**
 * Sets the key for the underlying encryption packet handler component, but doesn't modify encryption enabled state.
 */
UE_DEPRECATED(4.24, "Use SetEncryptionData instead.")
ENGINE_API void SetEncryptionKey(TArrayView<const uint8> Key);

/**
 * Sets the data for the underlying encryption packet handler component, but doesn't modify encryption enabled state.
 */
ENGINE_API void SetEncryptionData(const FEncryptionData& EncryptionData);

/**
 * Sends an NMT_EncryptionAck message
 */
ENGINE_API void SendClientEncryptionAck();

/**
 * Enables encryption for the underlying encryption packet handler component.
 */
ENGINE_API void EnableEncryption();

/**
 * Returns true if encryption is enabled for this connection.
 */
ENGINE_API virtual bool IsEncryptionEnabled() const;
```
#### WriteBitsToSendBuffer

```c++
/** 
 * Appends the passed in data to the SendBuffer to be sent when FlushNet is called
 * @param Bits Data as bits to be appended to the send buffer
 * @param SizeInBits Number of bits to append
 * @param ExtraBits (optional) Second set of bits to be appended to the send buffer that need to send with the first set of bits
 * @param ExtraSizeInBits (optional) Number of secondary bits to append
 * @param TypeOfBits (optional) The type of data being written, for profiling and bandwidth tracking purposes
 */
//将传入的数据追加到FlushNet调用时要发送的Sendbuffer后。
int32 WriteBitsToSendBuffer( 
	const uint8 *	Bits, 
	const int32		SizeInBits, 
	const uint8 *	ExtraBits = NULL, 
	const int32		ExtraSizeInBits = 0,
	EWriteBitsDataType DataType =  EWriteBitsDataType::Unknown);
```
#### ReceivedRawPacket

处理接收到的raw数据。

#### SendRawBunch

发送一个raw bunch

#### ReceivedPacket

处理我们刚接收到的包。

#### ReceivedNak

packet是被negatively acknowledged(Nak)。即没有接收到。

#### GetVoiceChannel

```c++
/**
 * @return Finds the voice channel for this connection or NULL if none
 */
ENGINE_API class UVoiceChannel* GetVoiceChannel();
```
#### FlushDormancy

对当前actor和actor中的需要复制的组件进行**FlushDormancyForObject**的操作

#### FlushDormancyForObject

检测objects的睡眠状态，准备再复制对象。

```c++
/** Wrapper for validating an objects dormancy state, and to prepare the object for replication again */
void FlushDormancyForObject( UObject* Object );
```
#### ForcePropertyCompare

强制此actor的属性做一个对比

```cpp
/** Forces properties on this actor to do a compare for one frame (rather than share shadow state) */
ENGINE_API void ForcePropertyCompare( AActor* Actor );
```
#### SetClientLoginState

设置客户端登陆状态

#### FlushPacketOrderCache

刷新正在等待一个丢失的包的排序好的包的缓存。

#### ConsumeSaturationAnalytics

返回当前的饱和度分析并且重置它们。

#### ConsumePacketAnalytics

返回当前的包的稳定性分析并且重置它们。

#### DestroyIgnoredActor

内部调用，销毁actor。

#### HandleConnectionTimeout

当一个连接超时时被调用。

#### Write，Read，Receive，Process，Prepare

```cpp
/** Write packetHeader */
void WritePacketHeader(FBitWriter& Writer);

/** Write placeholder bits for data filled before the real send (ServerFrameTime, JitterClockTime) */
void WriteDummyPacketInfo(FBitWriter& Writer);

/** Overwrite the dummy packet info values with real values before sending */
void WriteFinalPacketInfo(FBitWriter& Writer, double PacketSentTimeInS);

/** Read extended packet header information (ServerFrameTime) */
bool ReadPacketInfo(FBitReader& Reader, bool bHasPacketInfoPayload);
	/** Write data to outgoing send buffer, PrepareWriteBitsToSendBuffer should be called before to make sure that
		data will fit in packet. */
	int32 WriteBitsToSendBufferInternal( 
		const uint8 *	Bits, 
		const int32		SizeInBits, 
		const uint8 *	ExtraBits, 
		const int32		ExtraSizeInBits,
		EWriteBitsDataType DataType =  EWriteBitsDataType::Unknown);
	/** Packet was acknowledged as delivered */
	void ReceivedAck(int32 AckPacketId);

	/** Calculate the average jitter while adding the new packet's jitter value */
	void ProcessJitter(uint32 PacketJitterClockTimeMS);

	/** Flush outgoing packet if needed before we write data to it */
	void PrepareWriteBitsToSendBuffer(const int32 SizeInBits, const int32 ExtraSizeInBits);
```
## UIpConnection

### 主要变量

#### Socket

```cpp
	//这个指针是被其他地方拥有的套接字
/** This is a non-owning pointer to a socket owned elsewhere, IpConnection will not destroy the socket through this pointer. */
class FSocket*				Socket;
```
#### SocketSendResults

发送任务的套接字的发送结果。

#### BindSockets

一个sockets数组，绑定到每个绑定地址。

### 主要接口函数

```cpp
virtual void InitBase(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) override;
virtual void InitRemoteConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, const class FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) override;
virtual void InitLocalConnection(UNetDriver* InDriver, class FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket = 0, int32 InPacketOverhead = 0) override;
virtual void LowLevelSend(void* Data, int32 CountBits, FOutPacketTraits& Traits) override;
FString LowLevelGetRemoteAddress(bool bAppendPort=false) override;
FString LowLevelDescribe() override;
virtual void Tick() override;
virtual void CleanUp() override;
virtual void ReceivedRawPacket(void* Data, int32 Count) override;
virtual float GetTimeoutValue() override;
```
详见父类注释

#### HandleSocketSendResult

在game线程上处理任何SendTo错误。

#### HandleSocketRecvError

提醒我们当接收一个包时遇到了一个错误。

#### HandleConnectionTimeout

处理超时。

此类大部分还是继承于netconnection，并没有重写太多东西。

## UChannel

### 主要变量：

#### Connection

```cpp
UPROPERTY()
class UNetConnection*	Connection;		// Owner connection.
```
#### 多个变量:

```c++
	// Variables.
	uint32				OpenAcked:1;		// If OpenedLocally is true, this means we have acknowledged the packet we sent the bOpen bunch on. Otherwise, it means we have received the bOpen bunch from the server.
	uint32				Closing:1;			// State of the channel.
	uint32				Dormant:1;			// Channel is going dormant (it will close but the client will not destroy
	uint32				bIsReplicationPaused:1;	// Replication is being paused, but channel will not be closed
	uint32				OpenTemporary:1;	// Opened temporarily.
	uint32				Broken:1;			// Has encountered errors and is ignoring subsequent packets.
	uint32				bTornOff:1;			// Actor associated with this channel was torn off
	uint32				bPendingDormancy:1;	// Channel wants to go dormant (it will check during tick if it can go dormant)
	uint32				bPausedUntilReliableACK:1; // Unreliable property replication is paused until all reliables are ack'd.
	uint32				SentClosingBunch:1;	// Set when sending closing bunch to avoid recursion in send-failure-close case.
	uint32				bPooled:1;			// Set when placed in the actor channel pool
	//这个通道是本地开启的还是远程
	uint32				OpenedLocally:1;	// Whether channel was opened locally or by remote.
	uint32				bOpenedForCheckpoint:1;	// Whether channel was opened by replay checkpoint recording
	int32				ChIndex;			// Index of this channel.
	FPacketIdRange		OpenPacketId;		// If OpenedLocally is true, this is the packet we sent the bOpen bunch on. Otherwise, it's the packet we received the bOpen bunch on.
	UE_DEPRECATED(4.22, "ChType has been deprecated in favor of ChName.")
	EChannelType		ChType;				// Type of this channel.
	FName				ChName;				// Name of the type of this channel.
	int32				NumInRec;			// Number of packets in InRec.
	int32				NumOutRec;			// Number of packets in OutRec.
	class FInBunch*		InRec;				// Incoming data with queued dependencies.
	class FOutBunch*	OutRec;				// Outgoing reliable unacked data.
	class FInBunch*		InPartialBunch;		// Partial bunch we are receiving (incoming partial bunches are appended to this)
```

### 主要函数:

#### BeginDestroy

对UObject内的此函数的重写。

#### Serialize

重写

#### Init

初始化此通道。

#### Close

关闭此channel。返回向send buffer内写入了多少位。

#### Receive函数

```cpp
/** Handle an incoming bunch. */
virtual void ReceivedBunch( FInBunch& Bunch ) PURE_VIRTUAL(UChannel::ReceivedBunch,);
/** Positive acknowledgment processing. */
virtual void ReceivedAck( int32 AckPacketId );
//接收到nak的处理
/** Negative acknowledgment processing. */
virtual void ReceivedNak( int32 NakPacketId );
// General channel functions.
/** Handle an acknowledgment on this channel. */
void ReceivedAcks();
/** Process a properly-sequenced bunch. */
bool ReceivedSequencedBunch( FInBunch& Bunch );
/** 
* Process a raw, possibly out-of-sequence bunch: either queue it or dispatch it.
* The bunch is sure not to be discarded.
*/
void ReceivedRawBunch( FInBunch & Bunch, bool & bOutSkipAck );
```
#### Tick

处理此通道的时间

#### SendBunch

```cpp
//发送一个bunch如果它没有溢出，如果是可靠的对它进行排队
/** Send a bunch if it's not overflowed, and queue it if it's reliable. */
virtual FPacketIdRange SendBunch(FOutBunch* Bunch, bool Merge);
```
#### private内的函数

```c++
/** Just sends the bunch out on the connection */
int32 SendRawBunch(FOutBunch* Bunch, bool Merge, const FNetTraceCollector* Collector = nullptr);
/** Final step to prepare bunch to be sent. If reliable, adds to acknowldege list. */
FOutBunch* PrepBunch(FOutBunch* Bunch, FOutBunch* OutBunch, bool Merge);
/** Received next bunch to process. This handles partial bunches */
bool ReceivedNextBunch( FInBunch & Bunch, bool & bOutSkipAck );
```
### 主要函数：

## UActorChannel

### 主要变量:

#### Actor

```cpp
//对应的actor
// Variables.
UPROPERTY()
AActor* Actor;					// Actor this corresponds to.
```
#### ActorNetGUID

```cpp
//Actor的GUID,在actor还没被解析出来时使用。目前只在客户端有效。
FNetworkGUID	ActorNetGUID;		// Actor GUID (useful when we don't have the actor resolved yet). Currently only valid on clients.
```
### 主要函数：

```cpp
virtual void Init( UNetConnection* InConnection, int32 InChIndex, EChannelCreateFlags CreateFlags ) override;
virtual void SetClosingFlag() override;
virtual void ReceivedBunch( FInBunch& Bunch ) override;
virtual void Tick() override;
virtual bool CanStopTicking() const override;

void ProcessBunch( FInBunch & Bunch );
bool ProcessQueuedBunches();

virtual void ReceivedNak( int32 NakPacketId ) override;
virtual int64 Close(EChannelCloseReason Reason) override;
virtual FString Describe() override;
```
重写的看父类注释。

#### ReplicateActor

复制这个通道的不同地方，并返回复制了多少位。

```cpp
/** Replicate this channel's actor differences. Returns how many bits were replicated (does not include non-bunch packet overhead) */
int64 ReplicateActor();
```
#### SetChannelActor

	/**
	 * Set this channel's actor to the given actor.
	 * It's expected that InActor is either null (releasing the channel's reference) or
	 * a valid actor that is not PendingKill or PendingKillPending.
	 */
	void SetChannelActor(AActor* InActor, ESetChannelActorFlags Flags);
#### Serialize

函数重写。

#### AddReferencedObjects

垃圾回收相关

####  QueueRemoteFunctionBunch

```CPP
/** Queue a function bunch for this channel to be sent on the next property update. */
void QueueRemoteFunctionBunch( UObject* CallTarget, UFunction* Func, FOutBunch &Bunch );
```
#### ReadyForDormancy

此通道是否准备休眠。

#### ReadWrite函数

```cpp
/** Writes the header for a content block of properties / RPCs for the given object (either the actor a subobject of the actor) */
void WriteContentBlockHeader( UObject* Obj, FNetBitWriter &Bunch, const bool bHasRepLayout );

/** Writes the header for a content block specifically for deleting sub-objects */
void WriteContentBlockForSubObjectDelete( FOutBunch & Bunch, FNetworkGUID & GuidToDelete );

/** Writes header and payload of content block */
int32 WriteContentBlockPayload( UObject* Obj, FNetBitWriter &Bunch, const bool bHasRepLayout, FNetBitWriter& Payload );

/** Reads the header of the content block and instantiates the subobject if necessary */
UObject* ReadContentBlockHeader( FInBunch& Bunch, bool& bObjectDeleted, bool& bOutHasRepLayout );

/** Reads content block header and payload */
UObject* ReadContentBlockPayload( FInBunch &Bunch, FNetBitReader& OutPayload, bool& bOutHasRepLayout );

/** Writes property/function header and data blob to network stream */
int32 WriteFieldHeaderAndPayload( FNetBitWriter& Bunch, const FClassNetCache* ClassCache, const FFieldNetCache* FieldCache, FNetFieldExportGroup* NetFieldExportGroup, FNetBitWriter& Payload, bool bIgnoreInternalAck=false );

/** Reads property/function header and data blob from network stream */
bool ReadFieldHeaderAndPayload( UObject* Object, const FClassNetCache* ClassCache, FNetFieldExportGroup* NetFieldExportGroup, FNetBitReader& Bunch, const FFieldNetCache** OutField, FNetBitReader& OutPayload ) const;
```
#### ReplicateSubobject

子对象的复制。

#### ReplicateSubobjectList

复制待复制的子对象列表。