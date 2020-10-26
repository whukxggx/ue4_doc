# 网络复制 使用篇

## 参考文档:

[官方文档](https://docs.unrealengine.com/en-US/Gameplay/Networking/index.html)

[文章1](https://michaeljcole.github.io/wiki.unrealengine.com/Replication/#Replication_of_Actor_Components_and_Subobjects)

[文章2](https://gameinstitute.qq.com/community/detail/107666)

[技巧](https://www.unrealengine.com/zh-CN/blog/network-tips-and-tricks)

## 游戏启动

### 监听服务器

　　`UE4Editor.exe ProjectName MapName?Listen -game`

### 专用服务器

　　`UE4Editor.exe ProjectName MapName -server -game -log`

### 客户端

　　`UE4Editor.exe ProjectName ServerIP -game`

### 连接过程

1. 客户端发送连接请求。

2. 如果服务器接受连接，则发送当前地图。

3. 服务器等待客户端加载此地图。

4. 加载之后，服务器在本地调用AGameMode::PreLogin，这样可以使GameMode有机会拒绝连接。

5. 如果接受连接，服务器调用AGameMode::Login 

   此Login具体在AGameModeBase类，具体作用是创建一个*PlayerController*,可用于在今后复制到新连接的客户端。成功接收后，这个 PlayerController 将替代客户端的临时 PlayerController （之前被用作连接过程中的占位符）。

6. 调用 APlayerController::BeginPlay。应当注意的是，在此 actor 上调用 RPC 函数尚存在安全风险。**您应当等待 AGameMode::PostLogin 被调用完成**。

7. AGameMode::PostLogin 将被调用。

8. 可以让服务器在此PlayerController上开始调用RPC函数。

## 常见复制

| 复制功能                | 描述                                                         |
| :---------------------- | :----------------------------------------------------------- |
| **创造与破坏**          | 当在服务器上生成复制的Actor的权威版本时，它将自动在所有连接的客户端上生成其自身的远程代理。然后它将信息复制到那些远程代理。如果销毁权威Actor，它将自动销毁所有已连接客户端上的远程代理。 |
| **运动复制**            | 如果权威Actor 启用了“ **Replicate Movement**”，或者在C ++中`bReplicateMovement`设置为“ `true`”，它将自动复制其“位置”，“旋转”和“速度”。 |
| **变量复制**            | 只要其值发生更改，任何指定为要复制的变量都将自动从权威角色复制到其远程代理。 |
| **组件复制**            | Actor组件复制为拥有它们的Actor的一部分。Component中任何指定为要复制的变量都将被复制，并且在该组件内调用的任何RPC都将与Actor类中调用的RPC保持一致。 |
| **远程过程调用（RPC）** | RPC是特殊函数，可在网络游戏中传输到特定计算机。无论最初在什么计算机上调用RPC，RPC的实现都只会在它打算使用的计算机上运行。它们可以指定为服务器（仅在服务器上运行），客户端（仅在Actor拥有的客户端上运行）或NetMulticast（在连接到会话的每台计算机上运行，包括服务器上）。 |

以下常见特点无法被复制:

- **骨架网格物体**和**静态网格物体**组件
- **用料**
- **动画蓝图**
- **粒子系统**
- **声音发射器**
- **物理对象**

这些中的每一个都在所有客户端上单独运行。但是，如果复制了驱动这些视觉元素的变量，则将确保所有客户端具有相同的信息，因此将以近似相同的方式模拟它们。

## 权限问题

**authoritative**角色被认为可控制Actor的状态，并可将信息复制到网路多人游戏会话的其他机器上。

(**remote proxy**)远程代理是该Actor在远程机器上的副本，将接受authoritative角色的复制信息。其由 **Local Role** 和 **Remote Role** 变量进行追踪，可取以下值：

| 网络Acotr            | 说明                                                         |
| -------------------- | ------------------------------------------------------------ |
| **None**             | Actor在网络游戏中无角色，不会复制。                          |
| **Authority**        | Actor为授权状态，会将其信息复制到其他机器上的远程代理。      |
| **Simulated Proxy**  | Actor为远程代理，由另一台机器上的授权Actor完全控制。网络游戏中如拾取物、发射物或交互对象等多数Actor将在远程客户端上显示为模拟代理。 |
| **Autonomous Proxy** | Actor为远程代理，能够本地执行部分功能，但会接收授权Actor中的矫正。自主代理通常为玩家直接控制的actor所保留，如pawn。 |

虚幻引擎使用的默认模型是  **server-authoritative**，意味着服务器对游戏状态固定具有权限，而信息固定从服务器复制到客户端。服务器上的Actor应具有授权的本地角色，而其在远程客户端上的对应Actor应具有模拟或自主代理的本地角色。

### 客户端拥有权

特定客户端机器上的PlayController拥有网络游戏中的pawn。pawn调用纯客户端函数时，其将无视调用函数的机器。若将Actor的Owner变量设置为特定pawn，则通过关联，该Actor属于该Pawn的拥有客户端，并将纯客户端函数指向其拥有者的机器。可使用C++中的 `IsLocallyControlled` 函数以决定Pawn是否在其拥有客户端上。

*注意构造期间pawn可能未指定controller，因此避免在自定义pawn类的构造函数中使用IsLocallyController。*

## Actor复制

**Actor** 是实现复制的主要推动者。服务器将保留一份 Actor 列表并定期更新客户端，以便客户端保留每个 Actor （那些需要被同步的Actor）的近似副本。

### 两种方式更新

属性复制和RPC

区别：

属性可以在发生变化时随时自动更新，而 RPC 只能在被执行时获得调用更新。

#### 属性复制

每个actor维护一个全属性列表，其中包含Replicated说明符。每当复制的属性值发生变化时，服务器会向所有客户端发送更新。客户端会将其应用到Actor的本地版本上。这些更新只会来自服务器，客户端永远不会向服务器或其他客户端发送属性更新。

*我们不推荐在客户端上更改复制的变量值。该值将始终与服务器端的值不一致，直到服务器下一次侦测到变更并发送更新为止。如果服务器版本的属性不是经常更新，那客户端就需要等待很长时间才能被纠正。*

##### 设置步骤

* 在定义属性处，需用replicated作为UPROPERTY的参数之一。

* 需要实现GetLifetimeReplicatedProps函数，并将属性添加到里面。

  ```c++
  void AActor::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
  {
      DOREPLIFETIME( AActor, Owner );
  }
  ```

* 构造函数中确保`bReplicates=true;`

##### 更新优化

######  数据驱动型网络更新频率

Actor将观察在其"NetUpdateFrequency"变量中设置的最大更新频率。在不重要或者不频繁变化的actor上**降低该变量**网络更新可以变得高效。同时在有限带宽的场景中可能会带来更流畅的游戏体验。

###### 自适应型网络更新频率

默认关闭，通过将net.UseAdaptiveNetUpdateFrequency设置为1激活。

##### 条件属性复制

**当属性被注册进行复制后，您将无法再取消注册**（涉及到生存期这一话题）。之所以会这样，是因为我们要预制尽可能多的信息，以便针对同一组属性将某一工作分担给多个连接。这样可以节省大量的计算时间。

```
void AActor::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
    DOREPLIFETIME_CONDITION( AActor, ReplicatedMovement, COND_SimulatedOnly );
}
```

如上，只会复制到拥有此actor模拟副本的客户端，这样节省了带宽。

[目前支持的条件](https://docs.unrealengine.com/zh-CN/Gameplay/Networking/Actors/Properties/Conditions/index.html)

`DOREPLIFETIME_ACTIVE_OVERRIDE`可以进行全面控制，自己定制条件。

```
void AActor::PreReplication( IRepChangedPropertyTracker & ChangedPropertyTracker )
{
    DOREPLIFETIME_ACTIVE_OVERRIDE( AActor, ReplicatedMovement, bReplicateMovement );
}
```

现在 ReplicatedMovement 属性只会在 bReplicateMovement 为 true 时复制。

##### 复制对象引用

[加replicated](https://docs.unrealengine.com/zh-CN/Gameplay/Networking/Actors/Properties/ObjectReferences/index.html)

##### 属性复制时获取事件

如果您需要知道何时从服务器获取属性值的更新，可以使用ReplicatedUsing。注意：该事件不会在服务器上触发。**注意：ReplicatedUsing指定的函数调用时属性值已经改变。**

#### 组件复制

大多数组件都不会复制。

组件复制中涉及两大类组件。一种是随 Actor 一起创建的静态组件。也就是说，在客户端或服务器上生成 所属 Actor 时，这些组件也会同时生成，与组件是否被复制无关。服务器不会告知客户端显式生成这些组件。 在此背景下，静态组件是作为默认子对象在 C++ 构造函数中创建，或是在蓝图编辑器的组件模式中创建。静态组件无需通过复制存在于 客户端；它们将默认存在。**只有在属性或事件需要在服务器和客户端之间自动同步时，才需要进行复制。**

动态组件是在运行时在服务器上生成的组件种，其创建和删除操作也将被复制到客户端。它们的运行方式与 Actor 极为一致。与静态组件不同， 动态组件需通过复制的方式存在于所有客户端。

要进行组件复制，只需调用 `AActorComponent::SetIsReplicated(true)` 即可。

#### actor及其所属连接

每个连接都有一个专门为其创建的 PlayerController。每个出于此原因创建的 PlayerController 都归这个连接所有。要确定一个 actor 是否归某一连接所有，您可以查询这个 actor 最外围的所有者，如果所有者是一个 PlayerController，则这个 actor 同样归属于拥有 PlayerController 的那个连接。

连接所有权是重要的：*RPC 需要确定哪个客户端将执行**run-on-client RPC* Actor replication和连接相关性。在涉及所有者时的 Actor 属性复制条件

连接所有权对于 RPC 这样的机制至关重要，因为当您在 actor 上调用 RPC 函数时，除非 RPC 被标记为多播，否则就需要知道要在哪个客户端上执行该 RPC。它可以查找所属连接来确定将 RPC 发送到哪条连接。

#### actor相关性和优先级

##### 相关性

场景的规模可能非常大，在特定时刻某个玩家只能看到关卡中的一小部分 Actor。被服务器认为可见或能够影响客户端的 Actor 组会被视为该客户端的相关 Actor 组。虚幻引擎的网络代码中包含一处重要的带宽优化：**服务器只会让客户端知道其相关组内的 Actor。**

虚幻引擎（依次）参照以下规则确定玩家的相关 Actor 组。这些测试是在虚拟函数 `AActor::IsNetRelevantFor()` 中实施。

1. 如果 Actor 是 bAlwaysRelevant、归属于 Pawn 或 PlayerController、本身为 Pawn 或者 Pawn 是某些行为（如噪音或伤害）的发起者，则其具有相关性。
2. 如果 Actor 是 bNetUseOwnerRelevancy 且拥有一个所有者，则使用所有者的相关性。
3. 如果 Actor 是 bOnlyRelevantToOwner 且没有通过第一轮检查，则不具有相关性。
4. 如果 Actor 被附加到另一个 Actor 的骨架模型，它的相关性将取决于其所在基础的相关性。
5. 如果 Actor 是不可见的 (bHidden == *true*) 并且它的 Root Component 并没有碰撞，那么则不具有相关性， * 如果没有 Root Component 的话，`AActor::IsNetRelevantFor()` 会记录一条警告，提示是否要将它设置为 bAlwaysRelevant=_true_。
6. 如果 AGameNetworkManager 被设置为使用基于距离的相关性，则只要 Actor 低于净剔除距离，即被视为具有相关性。

具体代码如下：

```c++
bool AActor::IsNetRelevantFor(const AActor* RealViewer, const AActor* ViewTarget, const FVector& SrcLocation) const
{
	if (bAlwaysRelevant || IsOwnedBy(ViewTarget) || IsOwnedBy(RealViewer) || this == ViewTarget || ViewTarget == GetInstigator())
	{
		return true;
	}
	else if (bNetUseOwnerRelevancy && Owner)
	{
		return Owner->IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	}
	else if (bOnlyRelevantToOwner)
	{
		return false;
	}
	else if (RootComponent && RootComponent->GetAttachParent() && RootComponent->GetAttachParent()->GetOwner() && (Cast<USkeletalMeshComponent>(RootComponent->GetAttachParent()) || (RootComponent->GetAttachParent()->GetOwner() == Owner)))
	{
		return RootComponent->GetAttachParent()->GetOwner()->IsNetRelevantFor(RealViewer, ViewTarget, SrcLocation);
	}
	else if(IsHidden() && (!RootComponent || !RootComponent->IsCollisionEnabled()))
	{
		return false;
	}

	if (!RootComponent)
	{
		UE_LOG(LogNet, Warning, TEXT("Actor %s / %s has no root component in AActor::IsNetRelevantFor. (Make bAlwaysRelevant=true?)"), *GetClass()->GetName(), *GetName() );
		return false;
	}
	
	return !GetDefault<AGameNetworkManager>()->bUseDistanceBasedRelevancy ||
			IsWithinNetRelevancyDistance(SrcLocation);
}
```

##### 优先级

每个 Actor 都有一个名为 NetPriority 的浮点变量。这个变量的数值越大，Actor 相对于其他"同伴"的带宽就越多。和优先级为 1.0 的 Actor 相比，优先级是 2.0 的 Actor 可以得到两倍的更新频度。唯一影响优先顺序的就是它们的比值；所以很显然，您无法通过提高所有优先级的数值来增加虚幻引擎的 网络性能。下面是我们在性能调整中分配的部分 NetPriority 值：

- Actor = 1.0
- Matinee = 2.7
- Pawn = 3.0
- PlayerController = 3.0

#### actor复制流程

大多数 actor 复制操作都发生在 `UNetDriver::ServerReplicateActors` 内。在这里，服务器将收集所有被认定与各个客户端相关的 actor，并发送那些自上次（已连接的）客户端更新后出现变化的所有属性。

这里还定义了一个专门流程，指定了 actor 的更新方式、要调用的特定框架回调，以及在此过程中使用的特定属性。

* `AActor::NetUpdateFrequency` - 用于确定 actor 的复制频度
* `AActor::PreReplication` - 在复制发生前调用
* *`AActor::bOnlyRelevantToOwner` - 如果此 actor 仅复制到所有者，则值为 true* 
* `AActor::IsRelevancyOwnerFor` - 用于确定 bOnlyRelevantToOwner 为 true 时的相关性
* `AActor::IsNetRelevantFor` - 用于确定 bOnlyRelevantToOwner 为 false 时的相关性

相应的高级流程如下：  循环每一个主动复制的 actor（`AActor::SetReplicates( true )`）

```
* 确定这个 actor 是否在一开始出现休眠（`DORM_Initial`），如果是这样，则立即跳过。
* 通过检查 NetUpdateFrequency 的值来确定 actor 是否需要更新，如果不需要就跳过
* 如果 `AActor::bOnlyRelevantToOwner` 为 true，则检查此 actor 的所属连接以寻找相关性（对所属连接的观察者调用 `AActor::IsRelevancyOwnerFor`）。如果相关，则添加到此连接的已有相关列表。
    * 此时，这个 actor 只会发送到单个连接。
* 对于任何通过这些初始检查的 actor，都将调用 `AActor::PreReplication`。
    * PreReplication 可以让您决定是否针对连接来复制属性。这时要使用 `DOREPLIFETIME_ACTIVE_OVERRIDE`。
* 如果通过过了以上步骤，则添加到所考虑的列表
```

\* 对于每个连接：

```
* 对于每个所考虑的上述 actor
    * 确定是否休眠
    * 是否还没有通道
        * 确定客户端是否加载了 actor 所处的场景
            * 如未加载则跳过
        * 针对连接调用 `AActor::IsNetRelevantFor`，以确定 actor 是否相关
            * 如不相关则跳过
* 在归连接所有的相关列表上添加上述任意 actor
* 这时，我们拥有了一个针对此连接的相关 actor 列表
* 按照优先级对 actor 排序
* 对于每个排序的 actor：
    * 如果连接没有加载此 actor 所在的关卡，则关闭通道（如存在）并继续
    * 每 1 秒钟调用一次 AActor::IsNetRelevantFor，确定 actor 是否与连接相关
    * 如果不相关的时间达到 5 秒钟，则关闭通道
    * 如果相关且没有通道打开，则立即打开一个通道
    * 如果此连接出现饱和
        * 对于剩下的 actor
            * 如果保持相关的时间不到 1 秒，则强制在下一时钟单位进行更新
            * 如果保持相关的时间超过 1 秒，则调用 `AActor::IsNetRelevantFor` 以确定是否应当在下一时钟单位更新
    * 对于通过了以上这几点的 actor，将调用 `UChannel::ReplicateActor` 将其复制到连接
```

##### 将actor复制到连接

`UChannel::ReplicateActor` 将负责把 actor 及其所有组件复制到连接中。

 \* 确定这是不是此 actor 通道打开后的第一次更新

```
* 如果是，则将所需的特定信息（初始方位、旋转等）序列化
```

\* 确定该连接是否拥有这个 actor

```
* 如果没有，而且这个 actor 的角色是 `ROLE_AutonomousProxy`，则降级为 `ROLE_SimulatedProxy`
```

#### actor的role和romoteRole

这两个属性告诉我们：谁拥有actor的主控权,actor是否被复制

要确定当前运行的引擎实例是否有主控者，需要查看 Role 属性是否为 `ROLE_Authority`。如果是，就表明这个运行中的引擎实例负责掌管此 actor（决定其是否被复制）。

如果 Role 是 `ROLE_Authority`，RemoteRole 是 `ROLE_SimulatedProxy` 或 `ROLE_AutonomousProxy`，就说明这个引擎实例负责将此 actor 复制到远程连接。

只有服务器能够向已连接的客户端同步 Actor （客户端永远都不能向服务器同步）。始终记住这一点， *只有* 服务器才能看到 `Role == ROLE_Authority` 和 `RemoteRole == ROLE_SimulatedProxy` 或者 `ROLE_AutonomousProxy`。

##### Role/RemoteRole 对调

对于不同的数值观察者，它们的 Role 和 RemoteRole 值可能发生对调。例如，如果您的服务器上有这样的配置： *`Role == ROLE_Authority`* `RemoteRole == ROLE_SimulatedProxy`

客户端会将其识别为以下形式： *`Role == ROLE_SimulatedProxy`* `RemoteRole == ROLE_Authority`

这种情况是正常的，因为服务器要负责掌管 actor 并将其复制到客户端。而客户端只是接收更新，并在更新的间歇模拟 actor。

#####  复制模式

服务器不会在每次更新时复制 actor。这会消耗太多的带宽和 CPU 资源。实际上，服务器会按照 `AActor::NetUpdateFrequency` 属性指定的频度来复制 actor。因此在 actor 更新的间歇，会有一些时间数据被传递到客户端。这会导致 actor 呈现出断续、不连贯的移动。为了弥补这个缺陷，**客户端将在更新的间歇中模拟 actor。**

目前共有两种类型的模拟。

###### `ROLE_SimulatedProxy`

这是标准的模拟途径，通常是根据上次获得的速率对移动进行推算。当服务器为特定的 actor 发送更新时，客户端将向着新的方位调整其位置，然后利用更新的间歇，根据由服务器发送的最近的速率值来继续移动 actor。

使用上次获得的速率值进行模拟，只是普通模拟方式中的一种。您完全可以编写自己的定制代码，在服务器更新的间隔使用其他的一些信息来进行推算。

###### `ROLE_AutonomousProxy`

这种模拟通常只用于 PlayerController 所拥有的 actor。这说明此 actor 会接收来自真人控制者的输入，所以在我们进行推算时，我们会有更多一些的信息，而且能使用真人输入内容来补足缺失的信息。

## RPC

### RPC的使用

要将一个函数声明为 RPC，您只需将 `Server`、`Client` 或 `NetMulticast` 关键字添加到 `UFUNCTION` 声明。

Client，在服务器调用，在客户端执行。

Server，在客户端调用，在服务器执行。

`NetMulticast`，服务器调用，服务器和当前连接所有客户端执行。

以下为具体的调用和执行情况：

#### 要求和注意事项

1. 必须从actor上调用

2. actor必须被复制

3. 如果rpc是从服务器调用并在客户端执行，则只有实际拥有这个actor的客户端才会执行函数。

4. 如果rpc是从客户端调用并在服务器上执行，客户端就必须拥有调用rpc的actor。

5. 多播rpc：

   ```
   * 如果它们是从服务器调用，服务器将在本地和所有已连接的客户端上执行它们。
   * 如果它们是从客户端调用，则只在本地而非服务器上执行。
   ```

#### 从服务器调用的 RPC

| Actor 所有权           | 未复制         | `NetMulticast`             | `Server`       | `Client`                    |
| :--------------------- | :------------- | :------------------------- | :------------- | :-------------------------- |
| **Client-owned actor** | 在服务器上运行 | 在服务器和所有客户端上运行 | 在服务器上运行 | 在 actor 的所属客户端上运行 |
| **Server-owned actor** | 在服务器上运行 | 在服务器和所有客户端上运行 | 在服务器上运行 | 在服务器上运行              |
| **Unowned actor**      | 在服务器上运行 | 在服务器和所有客户端上运行 | 在服务器上运行 | 在服务器上运行              |

#### 从客户端调用的 RPC

| Actor 所有权                    | 未复制                   | `NetMulticast`           | `Server`       | `Client`                 |
| :------------------------------ | :----------------------- | :----------------------- | :------------- | :----------------------- |
| **Owned by invoking client**    | 在执行调用的客户端上运行 | 在执行调用的客户端上运行 | 在服务器上运行 | 在执行调用的客户端上运行 |
| **Owned by a different client** | 在执行调用的客户端上运行 | 在执行调用的客户端上运行 | 丢弃           | 在执行调用的客户端上运行 |
| **Server-owned actor**          | 在执行调用的客户端上运行 | 在执行调用的客户端上运行 | 丢弃           | 在执行调用的客户端上运行 |
| **Unowned actor**               | 在执行调用的客户端上运行 | 在执行调用的客户端上运行 | 丢弃           | 在执行调用的客户端上运行 |

默认情况下，RPC 并不可靠。要确保在远程机器上执行 RPC 调用，您可以指定 `Reliable` 关键字：

#### 验证

检测错误数据/输入的一个手段。其主要思路是：如果 RPC 的验证函数检测到任何 参数存在问题，就会通知系统将发起 RPC 调用的客户端/服务器断开。

要为 RPC 声明一个验证函数，只需将 `WithValidation` 关键字添加到 `UFUNCTION` 声明语句：

```
UFUNCTION( Server, WithValidation )
void SomeRPCFunction( int32 AddHealth );
```

然后在实施函数旁边加入验证函数：

```c++
bool SomeRPCFunction_Validate( int32 AddHealth )
{
    if ( AddHealth > MAX_ADD_HEALTH )
    {
        return false;                       // This will disconnect the caller
    }
return true;                              // This will allow the RPC to be called
}

void SomeRPCFunction_Implementation( int32 AddHealth )
{
    Health += AddHealth;
}
```