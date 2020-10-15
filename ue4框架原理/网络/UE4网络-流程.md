[参考](https://zhuanlan.zhihu.com/p/34723199)

# 服务器和客户端初始化

## FEngineLoop::Init

先初始化GEngine，通过GConfig获取Engine的类名，然后通过反射获取Engine的UClass类，然后通过UClass类NewObject出GEngine。

```c++
GConfig->GetString(TEXT("/Script/Engine.Engine"), TEXT("GameEngine"), GameEngineClassName, GEngineIni);
EngineClass = StaticLoadClass( UGameEngine::StaticClass(), nullptr, *GameEngineClassName);
GEngine = NewObject<UEngine>(GetTransientPackage(), EngineClass);
```

调用`GEngine->ParseCommandline()`函数。

调用`GEngine->Init(this)`。此步骤根据引擎类型调用不同的子类的init函数。客户端和服务器一般都会继承自UGameEngine类，可能自己会添加一些东西？可以看到ShooterGame中是自己继承此类，但init只是简单的转调用。

调用Engine的回调函数。`	   UEngine::OnPostEngineInit.Broadcast();FCoreDelegates::OnPostEngineInit.Broadcast();`

调用`GEngine->Start()`启动游戏。

## UGameEngine::Init

调用基类Init:`UEngine::Init`在此基类中调用`FURL::StaticInit()`。`	FLinkerLoad::StaticInit(UTexture2D::StaticClass());`，并且调用`AddToRoot`（gc相关，添加到rootset中，不被回收）。（当然，初始化的东西很多）

之后加载游戏设置。

创建GameInstance。对于GameEngine来说，这是唯一的一个。

```c++
FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
UClass* GameInstanceClass = (GameInstanceClassName.IsValid() ? LoadObject<UClass>(NULL, *GameInstanceClassName.ToString()) : UGameInstance::StaticClass());
GameInstance = NewObject<UGameInstance>(this, GameInstanceClass);
```

创建过程依然三步走：获取类名，获取UClass，newobject。

之后对GameInstance调用InitializeStandalone函数，此函数内部所做的事：创建worldcontext，仅有一个，创建DummyWorld。之后此函数调用init函数，OnlineSession就是在此处赋值的。

此后如果是客户端会初始化viewportclient变量。

## UGameEngine::Start

此函数是转调用，即`GameInstance->StartGameInstance();`。首先创建一个defaultURL，然后获取默认的地图设置和默认地图名，最终组合成FURL类型，调用`Engine->Browse`函数，此函数加载地图。详细如下：

## UEngine::Browse

```c++
EBrowseReturnVal::Type UEngine::Browse( FWorldContext& WorldContext, FURL URL, FString& Error )
```

此函数接收worldcontext参数和url参数。因为world可能会被destory，因此需要知道world归属。可以把worldcontext看作一个跟踪（类似程序上下文切换？）。

此函数主要进行了地图的加载。

## FURL::StaticInit

此函数初始化了默认的服务器信息，如protocol,name,host,portal,port。并将`bDefaultsInitialized`设置为`true`。

# 连接流程：

## 服务器：

对于服务器，在初始化时调用UEngine::Browse时，此函数内，如果满足`URL.IsLocalInternal`，即服务器地址是空的且协议是默认协议"unreal"（即是服务器），则直接进行地图的加载`LoadMap`。

loadmap函数主要做的是地图的切换操作，进行旧地图的清理和新地图的加载。

### LoadMap大致流程

#### 清理我们离开的地图和加载的包

```c++
	// clean up any per-map loaded packages for the map we are leaving
	if (WorldContext.World() && WorldContext.World()->PersistentLevel)
	{
		CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Map, WorldContext.World()->PersistentLevel->GetOutermost()->GetName());
	}
```

#### 清理上个游戏中的包

```c++
	CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Game_PreLoadClass, TEXT(""));
	CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Game_PostLoadClass, TEXT(""));
	CleanupPackagesToFullyLoad(WorldContext, FULLYLOAD_Mutator, TEXT(""));
```

#### 卸载当前world

#### 修剪合并内存，清除上一关卡的分配

#### 进行新的map和world的加载。根据是传入的URL的信息。

```cpp
NewWorld = UWorld::FindWorldInPackage(WorldPackage);
```

#### 之后一系列操作:

```c++
NewWorld->SetGameInstance(WorldContext.OwningGameInstance);
GWorld = NewWorld;
WorldContext.SetCurrentWorld(NewWorld);
WorldContext.World()->AddToRoot();
WorldContext.World()->InitWorld();
WorldContext.World()->SetGameMode(URL);
```

#### 如果不是客户端并且有URL有listen选择，进行监听操作:

`WorldContext.World()->Listen(URL)`。

#### UWorld::Listen

主要操作如下:

##### 创建NetDriver。

##### 通过netdriver调用initlisten。

其实调用的是IpNetDriver的此函数。

#### UIpNetDriver::InitListen

主要调用两个函数:

##### InitBase

进行基础信息的初始化，socket的初始化。此处FNetworkNotify内部有一个world关联，因此DS是在world里处理`ControlMessage`的，客户端则是在`UPendingNetGame`中处理。

注意此处socket初始化好后就会开始监听。

##### InitConnectionlessHandler

初始化`ConnectionlessHandler`

```c++
ConnectionlessHandler = MakeUnique<PacketHandler>(&DDoS);
```

初始化`StatelessConnectComponent`

```c++
StatelessConnectComponent = StaticCastSharedPtr<StatelessConnectHandlerComponent>(NewComponent);
```

#### 接着world::listen之后

另外初始化了一些其他东西，暂略。

## 客户端

### UEngine::SetClientTravel

此函数发起地图切换。此函数并没有真正进行，只是将URL等信息记录下来。

### UEngine::TickWorldTravel

此函数检测服务器或者客户端的地图切换请求，对于客户端，如果有新的TravelURL（setclienttravel设置的），则进行地图的切换。（对于服务器则是nexturl）。

具体还是通过Browse函数进行实际的操作。此处详细看下客户端部分：

### UEngine::Browse

此函数内，如果URL满足`IsLocalInternal()`函数，表明是服务端，而`( URL.IsInternal() && GIsClient )`则是客户端。如果是客户端，则需要通过URL发起服务器的连接流程。

首先，关闭已有的`PendingNetGame`对象，清除当前world的`NetDriver/socket`，并调用NewObject新建一个`PendingNetGame`。

然后调用`Initialize`函数进行自身初始化，之后调用`InitNetDriver`进行netdriver的构建：

#### UPendingNetGame::InitNetDriver

首先创建NetDriver:

```c++
if (GEngine->CreateNamedNetDriver(this, NAME_PendingNetDriver, NAME_GameNetDriver))
{
	NetDriver = GEngine->FindNamedNetDriver(this, NAME_PendingNetDriver);
}
```

然后进行连接初始化:

`NetDriver->InitConnect( this, URL, ConnectionError );`

连接成功后就开始握手。

```c++
ServerConn->Handler->BeginHandshaking(
					FPacketHandlerHandshakeComplete::CreateUObject(this, &UPendingNetGame::SendInitialJoin));
```

`BeginHandshaking`函数内部调用`StatelessConnectHandlerComponent`的`NotifyHandshakeBegin`进行握手，通过`ServerConn->LowLevelSend(InitialPacket.GetData(), InitialPacket.GetNumBits(), Traits);`将包发送出去。

#### UIpNetDriver::InitConnect

首先先调用`UIpNetDriver::InitBase`进行操作。此类内进行基础信息初始化和socket初始化。

之后进行连接的创建:

```c++
ServerConnection = NewObject<UNetConnection>(GetTransientPackage(), NetConnectionClass);
UIpConnection* IPConnection = CastChecked<UIpConnection>(ServerConnection);
```

然后：

```c++
ServerConnection->InitLocalConnection(this, Socket, ConnectURL, USOCK_Pending);
```

进行Connection的初始化操作。注意此时状态为`USOCK_Pending`，即等待状态。此函数具体操作见下:

最后进行一个回调函数的注册，后台运行。此函数最终将addressResult信息传入ipConnection中。

然后创建Client端的不同`Channels`。

#### UNetDriver::CreateInitialClientChannels

此次运行主要创建了三种channel，即control，actor，以及voice。通过Channel名字来创建:

```c++
for (const FChannelDefinition& ChannelDef : ChannelDefinitions)
{
	if (ChannelDef.bInitialClient && (ChannelDef.ChannelClass != nullptr))
	{
		ServerConnection->CreateChannelByName(ChannelDef.ChannelName, EChannelCreateFlags::OpenedLocally, ChannelDef.StaticChannelIndex);
	}
}
```

可以看到IPConnection类通过Channel名字来进行具体channel的创建。

#### UIpConnection::InitLocalConnection

先调用父类的`InitBase`函数，具体看此函数介绍。

之后判断`IsAddressResolutionEnabled`，如果不是则转换ResolutionState状态并退出。

#### UIpNetDriver::InitBase

##### 首先调用父类的此函数：

仅针对客户端来说：

首先从URL中获取能获取到的信息。

然后调用`InitConnectionClass`进行ConnectionClass的初始化。

```c++
NetConnectionClass = LoadClass<UNetConnection>(NULL,*NetConnectionClassName,NULL,LOAD_None,NULL);
```

注册Notify:

```c++
Notify = InNotify;
```

##### 之后是子类额外做的：

创建并设置此`netdriver`的`FSocket`对象并直接对地址和端口进行绑定，设置发送和接受缓存大小。

如果满足某些条件，可能会创建单独的接收线程。

#### UIpConnection::InitBase

##### 先调用父类的InitBase函数：

首先初始化一下Driver。

设置状态信息，比如上次接收确认时间。

初始化URL。

初始化类中的`PacketHandler`。注意最终此handler对象进行握手操作。

初始化`UPackageMapClient`类型。



#  主函数解析

# FPacketIterator类

# FRecvMulti类

# UIpNetDriver::TickDispatch

# 如何接收发送

# 连接流程

# actor同步

## UNetDriver::ServerReplicateActors

此函数内，若`ReplicationDriver`存在，直接调用`ReplicationDriver`的`ServerReplicateActors`。一般是`ReplicationGraph`。

```c++
	if (ReplicationDriver)
	{
		return ReplicationDriver->ServerReplicateActors(DeltaSeconds);
	}
```

如果不使用`ReplicationGraph`，则按照以下逻辑：

主要有四个辅助函数:

```c++
//获取引此次能同步的连接数量，超过引擎每帧可以同步的进行忽略。对每个ClientConnection进行viewtarget的设置。注意此viewtarget是决定其他actor是否同步的关键。
int32 ServerReplicateActors_PrepConnections( const float DeltaSeconds );
//构建ConsiderList,对所有的active的对象，将将要销毁的，所有权为ROLE_NONE,休眠的添加到remove数组中，如果不是将要销毁的且满足其他条件，添加到OutConsiderList中，并对此actor调用PreReplication函数.
void ServerReplicateActors_BuildConsiderList( TArray<FNetworkObjectInfo*>& OutConsiderList, const float ServerTickTime );
//对每个连接，要获取considerlist中排好序的actors。首先查看actor是否有通道channel，如果此actor属于的关卡没有加载到client或者actor与connectionviewers并不相关，跳过同步。如果actor仅是与owner相关，bOnlyRelevantToOwner，则将其他不拥有此actor的connection的通道关闭。另外，将睡眠的跳过，将想变为睡眠的actor的开始睡眠，将与此connection相关的actor添加到list中。将被删除的actor添加进list中，最终对list按照actor优先级进行排序。
int32 ServerReplicateActors_PrioritizeActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, const TArray<FNetworkObjectInfo*> ConsiderList, const bool bCPUSaturated, FActorPriority*& OutPriorityList, FActorPriority**& OutPriorityActors );
//处理已经排好序的actors：
//如果连接饱和，直接返回，不处理
//对于要删除的条目，即actorinfo为空且destructioninfo不为空，则获取其通道，发送一个close bunch，并将其从destruction信息列表中移除。
//正常的actor的复制，获取channel，如果此acotr所属官桥存在，且actor与此connection相关，设置bIsRelevant
//对bIsRelevant或者最近相关的，进行具体操作:
//首先查找或者创建对此actor的通道，并为其设置一个actor。确保网络没有饱和时，通过channel开始复制actor。Channel->ReplicateActor()进行真正同步操作。
//最后，如果不是最近相关的，满足条件则将通道关闭。
int32 ServerReplicateActors_ProcessPrioritizedActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, FActorPriority** PriorityActors, const int32 FinalSortedCount, int32& OutUpdated );
```

首先调用第一个辅助函数获取能同步的连接数量，对每个连接进行viewtarget的设置，然后构建调用第二个辅助函数构建considerlist，之后，对每个连接都要对整个considerlist进行同步操作，注意连接索引超过第一个辅助函数所得数量的此帧不会对此连接操作，将会跳过。如果不跳过，获取此连接的viewtarget，通过此次循环获取的connection来构造netviewer对象，并进行第三个和第四个辅助函数的调用，之后将此帧没有被处理的actor标记，下一帧进行处理。并且，clientconnection循环过后，如果不是所有循环都进行了处理，进行洗牌，将末尾的连接放到前面。

## UActorChannel::ReplicateActor()

此函数复制通道的actor的不同的地方，返回复制了多少位。调用`UPackageMapClient::SerializeNewActor`将actor序列化到bunch内，调用

`ActorReplicator->ReplicateProperties(Bunch, RepFlags)`将属性信息序列化到bunch中，调用`Actor->ReplicateSubobjects(this, &Bunch, &RepFlags);`将子对象序列化到bunch中，然后将其进一步封装并发出。

## AActor::ReplicateSubobjects

## UActorChannel::ReplicateSubobject

## 动态组件，静态组件

对于静态组件：一旦一个Actor被标记为同步，那么这个Actor身上默认所挂载的组件也会随Actor一起同步到客户端（也需要序列化发送）。什么是默认挂载的

件?就是C++构造函数里面创建的默认组件或者在蓝图里面添加构建的组件。所以，这个过程与该组件是否标记为Replicate是没有关系的。

对于动态组件：就是我们在游戏运行的时候，服务器创建或者删除的组件。比如，当玩家走进一个洞穴时，给洞穴里面的火把生成一个粒子特效组件，然后同步

客户端上，当玩家离开的时候再删除这个组件，玩家的客户端上也随之删除这个组件。

```c++
//静态组件，不需要客户端Spawn
FNetworkGUID NetGUID;
UObject * SubObj = NULL;
Connection->PackageMap->SerializeObject(Bunch, UObject::StaticClass(), SubObj, &NetGUID );
//动态组件，需要在客户端Spawn出来
FNetworkGUID ClassNetGUID;
UObject * SubObjClassObj = NULL;
Connection->PackageMap->SerializeObject(Bunch, UObject::StaticClass(), SubObjClassObj, &ClassNetGUID );
```

UObject的指针同步，由于不同机器上其地址不同，因此需要NetworkGUID进行解析。

# 属性同步

## FRepLayout

## FRepState

## FRepChangedPropertyTracker

## FReplicationChangelistMgr

## FObjectReplicator

# rpc实现

# DDoS防御

# UReplicationGraph