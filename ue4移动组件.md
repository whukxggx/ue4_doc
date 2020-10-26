# UProjectileMovementComponent

## 作用：

在另一个组件tick期间更新其位置。

支持碰撞后弹跳和返回目标位置等。

通常所属角色的根组件会被移动，但是可能会选择另一个组件。（SetUpdatedComponent())。如果需要更新的组件是模拟物理的，那么只有初始发射参数才会影响到弹丸，物理模拟也会从哪里接管。

## 变量

[地址](https://docs.unrealengine.com/en-US/API/Runtime/Engine/GameFramework/UProjectileMovementComponent/index.html)

## 遇到的问题1：

如果用:

```
	FTransform SpawnTM(ShootDir.Rotation(), Origin);
	AGrenade* Projectile = Cast<AGrenade>(UGameplayStatics::BeginDeferredActorSpawnFromClass(this, GrenadeConfig.HandGrenadeClass, SpawnTM));
	if (Projectile)
	{
		Projectile->SetInstigator(GetInstigator());
		Projectile->SetOwner(this);
		Projectile->InitVelocity(ShootDir);

		UGameplayStatics::FinishSpawningActor(Projectile, SpawnTM);
	}
```

好像会产生弹射导致方向错误。

如果用:

```
	UWorld *world = GetWorld();
	if (world) {
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = GetInstigator();
		AGrenade* NewGrenade = world->SpawnActor<AGrenade>(GrenadeConfig.HandGrenadeClass, Origin, ShootDir.Rotation(), SpawnParams);
		if (NewGrenade) {
			NewGrenade->InitVelocity(ShootDir);
		}
	}
```

注意在AGrenade类中:

```
void AGrenade::PostInitializeComponents() {
	Super::PostInitializeComponents();
	CollisionComp->MoveIgnoreActors.Add(GetInstigator());
}
```

第一种情况的调用顺序是BeginDeferredActorSpawnFromClass----->InitVelocity---------->FinishSpawningActor-------------->PostInitializeComponents

这样的调用顺序，在忽略actor前就已经有速度，因此出现碰撞问题。虽然构造函数在begindefer阶段触发，但是此时调用GetInstigator()是空的，因此没用。

最终选用第二种。

而射击游戏中的示例代码没有这种反射的原因，没有添加如下两行进行反弹的属性以及生成位置应该是在外部？：

```
	MovementComp->bShouldBounce = true;
	MovementComp->Bounciness = 0.3f;
```

## 关于如何获取到移动后FHitResult

如下：

`MovementComp`是移动组件，那么

```c++
MovementComp->OnProjectileStop.AddDynamic(this,&AGrenade::OnImpact);
```

其中，OnImpact的参数为FHitResult，调用到OnImpact时就会把相应碰撞结果传入。

AddDynamic是一个宏，其实是一个动态委托：

```c++
#define AddDynamic( UserObject, FuncName ) __Internal_AddDynamic( UserObject, FuncName, STATIC_FUNCTION_FNAME( TEXT( #FuncName ) ) )
```

而移动组件的OnProjectileStop是一个`FOnProjectileStopDelegate`类型，即是一个动态委托：

```c++
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnProjectileStopDelegate, const FHitResult&, ImpactResult );
```

此委托所需要的参数即为FHitResult类型。

在往上查找一下具体获取位置，发现从下到上顺序：

`OnProjectileStop.Broadcast(HitResult);`

`StopSimulating(const FHitResult& HitResult)`

之后有多处调用StopSimulating，可找到一个为

```
HandleDeflection(FHitResult& Hit, const FVector& OldVelocity, const uint32 NumBounces, float& SubTickTimeRemaining)
```

此函数在

```
TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
```

内调用，hit在此初始化:

```
void UProjectileMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	FHitResult Hit(1.f);
}
```

