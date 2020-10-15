# GC中的锁等同步措施

## FThreadSafeCounter

一个线程安全的计数器，大致实现如下：

```c++
class FThreadSafeCounter
{
public:
	typedef int32 IntegerType;
	FThreadSafeCounter()
	{
		Counter = 0;
	}
	FThreadSafeCounter( const FThreadSafeCounter& Other )
	{
		Counter = Other.GetValue();
	}
	FThreadSafeCounter( int32 Value )
	{
		Counter = Value;
	}
	int32 Increment()
	{
		return FPlatformAtomics::InterlockedIncrement(&Counter);
	}
	int32 Add( int32 Amount )
	{
		return FPlatformAtomics::InterlockedAdd(&Counter, Amount);
	}
	int32 Decrement()
	{
		return FPlatformAtomics::InterlockedDecrement(&Counter);
	}
	int32 Subtract( int32 Amount )
	{
		return FPlatformAtomics::InterlockedAdd(&Counter, -Amount);
	}
	int32 Set( int32 Value )
	{
		return FPlatformAtomics::InterlockedExchange(&Counter, Value);
	}
	int32 Reset()
	{
		return FPlatformAtomics::InterlockedExchange(&Counter, 0);
	}
	int32 GetValue() const
	{
		return FPlatformAtomics::AtomicRead(&const_cast<FThreadSafeCounter*>(this)->Counter);
	}
private:
	/** Hidden on purpose as usage wouldn't be thread safe. */
	void operator=( const FThreadSafeCounter& Other ){}
	//看了很多回答，似乎volatile关键字并不需要？，在c++中，volatile与多线程无关。
	/** Thread-safe counter */
	volatile int32 Counter;
};

```

可以看到，相应的改变Counter对象的操作都是对应平台的原子操作，

至于volatile关键字，可以看如下两个回答：

[c++多线程有必要加volatile吗](https://www.zhihu.com/question/31490495)

[多线程编程中什么情况下需要加volatile](https://www.zhihu.com/question/31490495)

## FPThreadsCriticalSection

Critical Section 临界区段

[临界区段]([https://zh.wikipedia.org/wiki/%E8%87%A8%E7%95%8C%E5%8D%80%E6%AE%B5](https://zh.wikipedia.org/wiki/臨界區段))

```c++
/**
 * This is the PThreads version of a critical section. It uses pthreads
 * to implement its locking.
 */
//critical section 临界区段 是指一个访问共享资源的程序片段
class FPThreadsCriticalSection
{
	/**
	 * The pthread-specific critical section
	 */
	pthread_mutex_t Mutex;

public:
	/**
	 * Constructor that initializes the aggregated critical section
	 */
	FORCEINLINE FPThreadsCriticalSection(void)
	{
		// make a recursive mutex
		pthread_mutexattr_t MutexAttributes;
		pthread_mutexattr_init(&MutexAttributes);
		pthread_mutexattr_settype(&MutexAttributes, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&Mutex, &MutexAttributes);
		pthread_mutexattr_destroy(&MutexAttributes);
	}

	/**
	 * Destructor cleaning up the critical section
	 */
	FORCEINLINE ~FPThreadsCriticalSection(void)
	{
		pthread_mutex_destroy(&Mutex);
	}

	/**
	 * Locks the critical section
	 */
	FORCEINLINE void Lock(void)
	{
		pthread_mutex_lock(&Mutex);
	}
	
	/**
	 * Attempt to take a lock and returns whether or not a lock was taken.
	 *
	 * @return true if a lock was taken, false otherwise.
	 */
	FORCEINLINE bool TryLock()
	{
		return 0 == pthread_mutex_trylock(&Mutex);
	}

	/**
	 * Releases the lock on the critical seciton
	 */
	FORCEINLINE void Unlock(void)
	{
		pthread_mutex_unlock(&Mutex);
	}

private:
	FPThreadsCriticalSection(const FPThreadsCriticalSection&);
	FPThreadsCriticalSection& operator=(const FPThreadsCriticalSection&);
};

```

注意，pthreads，即POSIX Threads,是独立于语言的一个执行模型，以及并行执行模型。它允许程序控制在时间上重叠的多个工作流。

[wiki关于pthreads](https://en.wikipedia.org/wiki/POSIX_Threads)

此类主要是通过pthreads来实现了一个临界区。

## FEvent

```c++
/**
 * Interface for waitable events.
 *
 * This interface has platform-specific implementations that are used to wait for another
 * thread to signal that it is ready for the waiting thread to do some work. It can also
 * be used for telling groups of threads to exit.
 */
class FEvent
{
public:
	/**
	 * Creates the event.
	 *
	 * Manually reset events stay triggered until reset.
	 * Named events share the same underlying event.
	 *
	 * @param bIsManualReset Whether the event requires manual reseting or not.
	 * @return true if the event was created, false otherwise.
	 */
	virtual bool Create( bool bIsManualReset = false ) = 0;
	/**
	 * Whether the signaled state of this event needs to be reset manually.
	 *
	 * @return true if the state requires manual resetting, false otherwise.
	 * @see Reset
	 */
	virtual bool IsManualReset() = 0;

	/**
	 * Triggers the event so any waiting threads are activated.
	 *
	 * @see IsManualReset, Reset
	 */
	virtual void Trigger() = 0;

	/**
	 * Resets the event to an untriggered (waitable) state.
	 *
	 * @see IsManualReset, Trigger
	 */
	virtual void Reset() = 0;

	/**
	 * Waits the specified amount of time for the event to be triggered.
	 *
	 * A wait time of MAX_uint32 is treated as infinite wait.
	 *
	 * @param WaitTime The time to wait (in milliseconds).
	 * @param bIgnoreThreadIdleStats If true, ignores ThreadIdleStats
	 * @return true if the event was triggered, false if the wait timed out.
	 */
	virtual bool Wait( uint32 WaitTime, const bool bIgnoreThreadIdleStats = false ) = 0;

	/**
	 * Waits an infinite amount of time for the event to be triggered.
	 *
	 * @return true if the event was triggered.
	 */
	bool Wait()
	{
		return Wait(MAX_uint32);
	}

	/**
	 * Waits the specified amount of time for the event to be triggered.
	 *
	 * @param WaitTime The time to wait.
	 * @param bIgnoreThreadIdleStats If true, ignores ThreadIdleStats
	 * @return true if the event was triggered, false if the wait timed out.
	 */
	bool Wait( const FTimespan& WaitTime, const bool bIgnoreThreadIdleStats = false )
	{
		check(WaitTime.GetTicks() >= 0);
		return Wait((uint32)FMath::Clamp<int64>(WaitTime.GetTicks() / ETimespan::TicksPerMillisecond, 0, MAX_uint32), bIgnoreThreadIdleStats);
	}

	/** Default constructor. */
	FEvent()
		: EventId( 0 )
		, EventStartCycles( 0 )
	{}

	/** Virtual destructor. */
	virtual ~FEvent() 
	{}

	// DO NOT MODIFY THESE

	/** Advances stats associated with this event. Used to monitor wait->trigger history. */
	void AdvanceStats();

protected:
	/** Sends to the stats a special messages which encodes a wait for the event. */
	void WaitForStats();

	/** Send to the stats a special message which encodes a trigger for the event. */
	void TriggerForStats();

	/** Resets start cycles to 0. */
	void ResetForStats();

	/** Counter used to generate an unique id for the events. */
	static TAtomic<uint32> EventUniqueId;

	/** An unique id of this event. */
	uint32 EventId;

	/** Greater than 0, if the event called wait. */
	TAtomic<uint32> EventStartCycles;
};

```

这个接口有特定平台的实现，用于等待其他线程发出信号，表示它已准备好等待线程执行某些工作，它也可以用来通知线程组退出。

## FGCCSyncObject

此类内部封装了多个关于锁和同步的变量，用于垃圾回收的同步。如果没其他锁存在，能够锁GC。

```c++
/**
* Garbage Collection synchronization objects
* Will not lock other threads if GC is not running.
* Has the ability to only lock for GC if no other locks are present.
*/
class FGCCSyncObject
{
	/** Non zero if any of the non-game threads is blocking GC */
	FThreadSafeCounter AsyncCounter;
	/** Non zero if GC is running */
	FThreadSafeCounter GCCounter;
	/** Non zero if GC wants to run but is blocked by some other thread \
	    This flag is not automatically enforced on the async threads, instead
			threads have to manually implement support for it. */
	FThreadSafeCounter GCWantsToRunCounter;
	/** Critical section for thread safe operations */
	FCriticalSection Critical;
	/** Event used to block non-game threads when GC is running */
	FEvent* GCUnlockedEvent;

public:

	FGCCSyncObject();
	~FGCCSyncObject();

	//
	/** Creates the singleton object */
	static void Create();

	/** Gets the singleton object */
	static FGCCSyncObject& Get();

	/** Lock on non-game thread. Will block if GC is running. */
	void LockAsync()
	{
		if (!IsInGameThread())
		{
			// Wait until GC is done if it was running when entering this function
			bool bLocked = false;
			do
			{
				if (GCCounter.GetValue() > 0)
				{
					GCUnlockedEvent->Wait();
				}
				{
					FScopeLock CriticalLock(&Critical);
					if (GCCounter.GetValue() == 0)
					{
						AsyncCounter.Increment();
						bLocked = true;
					}
				}
			} while (!bLocked);
		}
	}
	/** Release lock from non-game thread */
	void UnlockAsync()
	{
		if (!IsInGameThread())
		{
			AsyncCounter.Decrement();
		}
	}
	/** Lock for GC. Will block if any other thread has locked. */
	void GCLock()
	{
		// Signal other threads that GC wants to run
		SetGCIsWaiting();

		// Wait until all other threads are done if they're currently holding the lock
		bool bLocked = false;
		do
		{
			FPlatformProcess::ConditionalSleep([&]()
			{
				return AsyncCounter.GetValue() == 0;
			});
			{
				FScopeLock CriticalLock(&Critical);
				if (AsyncCounter.GetValue() == 0)
				{
					GCUnlockedEvent->Reset();
					int32 GCCounterValue = GCCounter.Increment();
					check(GCCounterValue == 1); // GCLock doesn't support recursive locks
					// At this point GC can run so remove the signal that it's waiting
					FPlatformMisc::MemoryBarrier();
					ResetGCIsWaiting();
					bLocked = true;
				}
			}
		} while (!bLocked);
	}
	/** Checks if any async thread has a lock */
	bool IsAsyncLocked() const
	{
		return AsyncCounter.GetValue() != 0;
	}
	/** Checks if GC has a lock */
	bool IsGCLocked() const
	{
		return GCCounter.GetValue() != 0;
	}
	/** Lock for GC. Will not block and return false if any other thread has already locked. */
	bool TryGCLock()
	{
		bool bSuccess = false;
		FScopeLock CriticalLock(&Critical);
		// If any other thread is currently locking we just exit
		if (AsyncCounter.GetValue() == 0)
		{
			GCUnlockedEvent->Reset();
			int32 GCCounterValue = GCCounter.Increment();
			check(GCCounterValue == 1); // GCLock doesn't support recursive locks
			bSuccess = true;
		}
		return bSuccess;
	}
	/** Unlock GC */
	void GCUnlock()
	{
		GCUnlockedEvent->Trigger();
		GCCounter.Decrement();
	}

	/** Manually mark GC state as 'waiting to run' */
	void SetGCIsWaiting()
	{
		GCWantsToRunCounter.Increment();
	}

	/** Manually reset GC 'waiting to run' state*/
	void ResetGCIsWaiting()
	{
		GCWantsToRunCounter.Set(0);
	}

	/** True if GC wants to run on the game thread but is maybe blocked by some other thread */
	FORCEINLINE bool IsGCWaiting() const
	{
		return !!GCWantsToRunCounter.GetValue();
	}
};
```

功能上注释的比较清楚了。

注意`FCriticalSection`是类型别名，根据不同平台可能有不同实现，`FPThreadsCriticalSection`是其中一种实现。

关于`FGCCSyncObject::Create()`方法，此处用了一种技巧。

```c++
FGCCSyncObject* GGCSingleton;

void FGCCSyncObject::Create()
{
	struct FSingletonOwner
	{
		FGCCSyncObject Singleton;

		FSingletonOwner()	{ GGCSingleton = &Singleton; }
		~FSingletonOwner()	{ GGCSingleton = nullptr;	}
	};
	static const FSingletonOwner MagicStaticSingleton;
}
FGCCSyncObject& FGCCSyncObject::Get()
{
	FGCCSyncObject* Singleton = GGCSingleton;
	check(Singleton);
	return *Singleton;
}
```

这里在Create内添加了一个结构体

# GC

## CollectGarbage

```C++
void CollectGarbage(EObjectFlags KeepFlags, bool bPerformFullPurge)
{
	// No other thread may be performing UObject operations while we're running
	AcquireGCLock();

	// Perform actual garbage collection
	CollectGarbageInternal(KeepFlags, bPerformFullPurge);

	// Other threads are free to use UObjects
	ReleaseGCLock();
}
```

```c++
/** 
 * Deletes all unreferenced objects, keeping objects that have any of the passed in KeepFlags set
 *
 * @param	KeepFlags			objects with those flags will be kept regardless of being referenced or not
 * @param	bPerformFullPurge	if true, perform a full purge after the mark pass
 */
void CollectGarbageInternal(EObjectFlags KeepFlags, bool bPerformFullPurge)
{
	DECLARE_SCOPE_CYCLE_COUNTER( TEXT( "CollectGarbageInternal" ), STAT_CollectGarbageInternal, STATGROUP_GC );
	STAT_ADD_CUSTOMMESSAGE_NAME( STAT_NamedMarker, TEXT( "GarbageCollection - Begin" ) );

	// We can't collect garbage while there's a load in progress. E.g. one potential issue is Import.XObject
	check(!IsLoading());

	// Reset GC skip counter
	GNumAttemptsSinceLastGC = 0;

	// Flush streaming before GC if requested
	if (GFlushStreamingOnGC)
	{
		if (IsAsyncLoading())
		{
			UE_LOG(LogGarbage, Log, TEXT("CollectGarbageInternal() is flushing async loading"));
		}
		FGCCSyncObject::Get().GCUnlock();
		FlushAsyncLoading();
		FGCCSyncObject::Get().GCLock();
	}

	// Route callbacks so we can ensure that we are e.g. not in the middle of loading something by flushing
	// the async loading, etc...
	FCoreUObjectDelegates::GetPreGarbageCollectDelegate().Broadcast();
	GLastGCFrame = GFrameCounter;

	{
		// Set 'I'm garbage collecting' flag - might be checked inside various functions.
		// This has to be unlocked before we call post GC callbacks
		FGCScopeLock GCLock;

		UE_LOG(LogGarbage, Log, TEXT("Collecting garbage%s"), IsAsyncLoading() ? TEXT(" while async loading") : TEXT(""));

		// Make sure previous incremental purge has finished or we do a full purge pass in case we haven't kicked one
		// off yet since the last call to garbage collection.
		if (GObjIncrementalPurgeIsInProgress || GObjPurgeIsRequired)
		{
			IncrementalPurgeGarbage(false);
			FMemory::Trim();
		}
		check(!GObjIncrementalPurgeIsInProgress);
		check(!GObjPurgeIsRequired);

		// This can happen if someone disables clusters from the console (gc.CreateGCClusters)
		if (!GCreateGCClusters && GUObjectClusters.GetNumAllocatedClusters())
		{
			GUObjectClusters.DissolveClusters(true);
		}

#if VERIFY_DISREGARD_GC_ASSUMPTIONS
		// Only verify assumptions if option is enabled. This avoids false positives in the Editor or commandlets.
		if ((GUObjectArray.DisregardForGCEnabled() || GUObjectClusters.GetNumAllocatedClusters()) && GShouldVerifyGCAssumptions)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("CollectGarbageInternal.VerifyGCAssumptions"), STAT_CollectGarbageInternal_VerifyGCAssumptions, STATGROUP_GC);
			const double StartTime = FPlatformTime::Seconds();
			VerifyGCAssumptions();
			VerifyClustersAssumptions();
			UE_LOG(LogGarbage, Log, TEXT("%f ms for Verify GC Assumptions"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}
#endif

		// Fall back to single threaded GC if processor count is 1 or parallel GC is disabled
		// or detailed per class gc stats are enabled (not thread safe)
		// Temporarily forcing single-threaded GC in the editor until Modify() can be safely removed from HandleObjectReference.
		const bool bForceSingleThreadedGC = ShouldForceSingleThreadedGC();
		// Run with GC clustering code enabled only if clustering is enabled and there's actual allocated clusters
		const bool bWithClusters = !!GCreateGCClusters && GUObjectClusters.GetNumAllocatedClusters();

		// Perform reachability analysis.
		{
			const double StartTime = FPlatformTime::Seconds();
			FRealtimeGC TagUsedRealtimeGC;
			TagUsedRealtimeGC.PerformReachabilityAnalysis(KeepFlags, bForceSingleThreadedGC, bWithClusters);
			UE_LOG(LogGarbage, Log, TEXT("%f ms for GC"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}

		// Reconstruct clusters if needed
		if (GUObjectClusters.ClustersNeedDissolving())
		{
			const double StartTime = FPlatformTime::Seconds();
			GUObjectClusters.DissolveClusters();
			UE_LOG(LogGarbage, Log, TEXT("%f ms for dissolving GC clusters"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}

		// Fire post-reachability analysis hooks
		FCoreUObjectDelegates::PostReachabilityAnalysis.Broadcast();

		{			
			FGCArrayPool::Get().ClearWeakReferences(bPerformFullPurge);

			GatherUnreachableObjects(bForceSingleThreadedGC);

			if (bPerformFullPurge || !GIncrementalBeginDestroyEnabled)
			{
				UnhashUnreachableObjects(/**bUseTimeLimit = */ false);
				FScopedCBDProfile::DumpProfile();
			}
		}

		// Set flag to indicate that we are relying on a purge to be performed.
		GObjPurgeIsRequired = true;

		// Perform a full purge by not using a time limit for the incremental purge. The Editor always does a full purge.
		if (bPerformFullPurge || GIsEditor)
		{
			IncrementalPurgeGarbage(false);
		}

		if (bPerformFullPurge)
		{
			ShrinkUObjectHashTables();
		}

		// Destroy all pending delete linkers
		DeleteLoaders();

		// Trim allocator memory
		FMemory::Trim();
	}

	// Route callbacks to verify GC assumptions
	FCoreUObjectDelegates::GetPostGarbageCollect().Broadcast();

	STAT_ADD_CUSTOMMESSAGE_NAME( STAT_NamedMarker, TEXT( "GarbageCollection - End" ) );
}
```

此函数是内部真正进行GC的地方，删除所有未引用的objects。

## FlushAsyncLoading

阻塞，知道所有挂起的package和linker请求完成，

```c++
	if (GFlushStreamingOnGC)
	{
		if (IsAsyncLoading())
		{
			UE_LOG(LogGarbage, Log, TEXT("CollectGarbageInternal() is flushing async loading"));
		}
		FGCCSyncObject::Get().GCUnlock();
		FlushAsyncLoading();
		FGCCSyncObject::Get().GCLock();
	}
```

判断是否在GC时flushstreaming，如果是就先释放gc锁等待异步加载完成。

## IncrementalPurgeGarbage

```c++
if (GObjIncrementalPurgeIsInProgress || GObjPurgeIsRequired)
{
	IncrementalPurgeGarbage(false);
	FMemory::Trim();
}
```

确保之前的**增量清除**已经完成，否则做一个完全清除，防止自上次调用垃圾回收以来尚未启动一次。

```c++
/**
 * Incrementally purge garbage by deleting all unreferenced objects after routing Destroy.
 *
 * Calling code needs to be EXTREMELY careful when and how to call this function as 
 * RF_Unreachable cannot change on any objects unless any pending purge has completed!
 *
 * @param	bUseTimeLimit	whether the time limit parameter should be used
 * @param	TimeLimit		soft time limit for this function call
 */
void IncrementalPurgeGarbage(bool bUseTimeLimit, float TimeLimit)
{
	SCOPED_NAMED_EVENT(IncrementalPurgeGarbage, FColor::Red);
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("IncrementalPurgeGarbage"), STAT_IncrementalPurgeGarbage, STATGROUP_GC);
	CSV_SCOPED_TIMING_STAT_EXCLUSIVE(GarbageCollection);

	if (GExitPurge)
	{
		GObjPurgeIsRequired = true;
		GUObjectArray.DisableDisregardForGC();
		GObjCurrentPurgeObjectIndexNeedsReset = true;
	}
	// Early out if there is nothing to do.
	if (!GObjPurgeIsRequired)
	{
		return;
	}

	bool bCompleted = false;

	struct FResetPurgeProgress
	{
		bool& bCompletedRef;
		FResetPurgeProgress(bool& bInCompletedRef)
			: bCompletedRef(bInCompletedRef)
		{
			// Incremental purge is now in progress.
			GObjIncrementalPurgeIsInProgress = true;
			FPlatformMisc::MemoryBarrier();
		}
		~FResetPurgeProgress()
		{
			if (bCompletedRef)
			{
				GObjIncrementalPurgeIsInProgress = false;
				FPlatformMisc::MemoryBarrier();
			}
		}

	} ResetPurgeProgress(bCompleted);
	
	{
		// Lock before settting GCStartTime as it could be slow to lock if async loading is in progress
		// but we still want to perform some GC work otherwise we'd be keeping objects in memory for a long time
		FConditionalGCLock ScopedGCLock;

		// Keep track of start time to enforce time limit unless bForceFullPurge is true;
		GCStartTime = FPlatformTime::Seconds();
		bool bTimeLimitReached = false;

		if (GUnrechableObjectIndex < GUnreachableObjects.Num())
		{
			bTimeLimitReached = UnhashUnreachableObjects(bUseTimeLimit, TimeLimit);

			if (GUnrechableObjectIndex >= GUnreachableObjects.Num())
			{
				FScopedCBDProfile::DumpProfile();
			}
		}

		if (!bTimeLimitReached)
		{
			bCompleted = IncrementalDestroyGarbage(bUseTimeLimit, TimeLimit);
		}
	}
}
```

可以看到主要内容是:

```c++
		if (GUnrechableObjectIndex < GUnreachableObjects.Num())
		{
			bTimeLimitReached = UnhashUnreachableObjects(bUseTimeLimit, TimeLimit);

			if (GUnrechableObjectIndex >= GUnreachableObjects.Num())
			{
				FScopedCBDProfile::DumpProfile();
			}
		}
```

调用`UnhashUnreachableObjects`进行object的回收操作。

## UnhashUnreachableObjects

```c++
bool UnhashUnreachableObjects(bool bUseTimeLimit, float TimeLimit)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("UnhashUnreachableObjects"), STAT_UnhashUnreachableObjects, STATGROUP_GC);

	TGuardValue<bool> GuardObjUnhashUnreachableIsInProgress(GObjUnhashUnreachableIsInProgress, true);

	FCoreUObjectDelegates::PreGarbageCollectConditionalBeginDestroy.Broadcast();

	// Unhash all unreachable objects.
	const double StartTime = FPlatformTime::Seconds();
	const int32 TimeLimitEnforcementGranularityForBeginDestroy = 10;
	int32 Items = 0;
	int32 TimePollCounter = 0;
	const bool bFirstIteration = (GUnrechableObjectIndex == 0);

	while (GUnrechableObjectIndex < GUnreachableObjects.Num())
	{
		//@todo UE4 - A prefetch was removed here. Re-add it. It wasn't right anyway, since it was ten items ahead and the consoles on have 8 prefetch slots

		FUObjectItem* ObjectItem = GUnreachableObjects[GUnrechableObjectIndex++];
		{
			UObject* Object = static_cast<UObject*>(ObjectItem->Object);
			FScopedCBDProfile Profile(Object);
			// Begin the object's asynchronous destruction.
			Object->ConditionalBeginDestroy();
		}

		Items++;

		const bool bPollTimeLimit = ((TimePollCounter++) % TimeLimitEnforcementGranularityForBeginDestroy == 0);
		if (bUseTimeLimit && bPollTimeLimit && ((FPlatformTime::Seconds() - StartTime) > TimeLimit))
		{
			break;
		}
	}

	const bool bTimeLimitReached = (GUnrechableObjectIndex < GUnreachableObjects.Num());

	if (!bUseTimeLimit)
	{
		UE_LOG(LogGarbage, Log, TEXT("%f ms for %sunhashing unreachable objects (%d objects unhashed)"),
		(FPlatformTime::Seconds() - StartTime) * 1000,
		bUseTimeLimit ? TEXT("incrementally ") : TEXT(""),
			Items,
		GUnrechableObjectIndex, GUnreachableObjects.Num());
	}
	else if (!bTimeLimitReached)
	{
		// When doing incremental unhashing log only the first and last iteration (this was the last one)
		UE_LOG(LogGarbage, Log, TEXT("Finished unhashing unreachable objects (%d objects unhashed)."), GUnreachableObjects.Num());
	}
	else if (bFirstIteration)
	{
		// When doing incremental unhashing log only the first and last iteration (this was the first one)
		UE_LOG(LogGarbage, Log, TEXT("Starting unhashing unreachable objects (%d objects to unhash)."), GUnreachableObjects.Num());
	}

	FCoreUObjectDelegates::PostGarbageCollectConditionalBeginDestroy.Broadcast();

	// Return true if time limit has been reached
	return bTimeLimitReached;
}
```

此处主要看下没有时间限制的情况下，即一次性清楚完毕，主体在while循环中:

```c++
	while (GUnrechableObjectIndex < GUnreachableObjects.Num())
	{
		//@todo UE4 - A prefetch was removed here. Re-add it. It wasn't right anyway, since it was ten items ahead and the consoles on have 8 prefetch slots

		FUObjectItem* ObjectItem = GUnreachableObjects[GUnrechableObjectIndex++];
		{
			UObject* Object = static_cast<UObject*>(ObjectItem->Object);
			FScopedCBDProfile Profile(Object);
			// Begin the object's asynchronous destruction.
			Object->ConditionalBeginDestroy();
		}
		Items++;
		const bool bPollTimeLimit = ((TimePollCounter++) % TimeLimitEnforcementGranularityForBeginDestroy == 0);
		if (bUseTimeLimit && bPollTimeLimit && ((FPlatformTime::Seconds() - StartTime) > TimeLimit))
		{
			break;
		}
	}
```

对每个未标记的物体执行`ConditionalBeginDestory`操作，

## ConditionalBeginDestroy

```c++
/**
* Called before destroying the object.  This is called immediately upon deciding to destroy the object, to allow the object    * to begin an asynchronous cleanup process.
*/
//在销毁object之前调用。一旦决定destory这个object，就立即调用，以允许这个object开始一个异步的清理过程。
bool UObject::ConditionalBeginDestroy()
{
#if !UE_BUILD_SHIPPING
	// if this object wasn't marked (but some were) then that means it was created and destroyed since the SpikeMark command was given
	// this object is contributing to the spike that is being investigated
	if (DebugSpikeMarkAnnotation.Num() > 0)
	{
		if(!DebugSpikeMarkAnnotation.Get(this))
		{
			DebugSpikeMarkNames.Add(GetFullName());
		}
	}
#endif
	
	check(IsValidLowLevel());
	if( !HasAnyFlags(RF_BeginDestroyed) )
	{
		SetFlags(RF_BeginDestroyed);
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		checkSlow(!DebugBeginDestroyed.Contains(this));
		DebugBeginDestroyed.Add(this);
#endif

#if PROFILE_ConditionalBeginDestroy
		double StartTime = FPlatformTime::Seconds();
#endif

		BeginDestroy();

#if PROFILE_ConditionalBeginDestroy
		float ThisTime = float(FPlatformTime::Seconds() - StartTime);

		FTimeCnt& TimeCnt = MyProfile.FindOrAdd(GetClass()->GetFName());
		TimeCnt.Count++;
		TimeCnt.TotalTime += ThisTime;

		static float TotalTime = 0.0f;
		static int32 TotalCnt = 0;

		TotalTime += ThisTime;
		if ((++TotalCnt) % 1000 == 0)
		{
			UE_LOG(LogObj, Log, TEXT("ConditionalBeginDestroy %d cnt %fus"), TotalCnt, 1000.0f * 1000.0f * TotalTime / float(TotalCnt));

			MyProfile.ValueSort(TLess<FTimeCnt>());

			int32 NumPrint = 0;
			for (auto& Item : MyProfile)
			{
				UE_LOG(LogObj, Log, TEXT("    %6d cnt %6.2fus per   %6.2fms total  %s"), Item.Value.Count, 1000.0f * 1000.0f * Item.Value.TotalTime / float(Item.Value.Count), 1000.0f * Item.Value.TotalTime, *Item.Key.ToString());
				if (NumPrint++ > 30)
				{
					break;
				}
			}
		}
#endif


#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
		if( DebugBeginDestroyed.Contains(this) )
		{
			// class might override BeginDestroy without calling Super::BeginDestroy();
			UE_LOG(LogObj, Fatal, TEXT("%s failed to route BeginDestroy"), *GetFullName() );
		}
#endif
		return true;
	}
	else 
	{
		return false;
	}
}
```

在销毁object之前调用。一旦决定destory这个object，就立即调用，以允许这个object开始一个异步的清理过程。

## BeginDestroy

```c++
void UObject::BeginDestroy()
{
	// Remove from linker's export table.
	SetLinker( NULL, INDEX_NONE );

	LowLevelRename(NAME_None);
}
```

此函数主要做了两件事，`SetLinker`和`LowLevelRename`两个函数。一个是将其从linker的导出表中移除，一个是改变名字和outer为空，把名字从hash表中移除。

# 可达性分析（标记阶段）

## PerformReachabilityAnalysis

```c++
	/**
	 * Performs reachability analysis.
	 *
	 * @param KeepFlags		Objects with these flags will be kept regardless of being referenced or not
	 */
	void PerformReachabilityAnalysis(EObjectFlags KeepFlags, bool bForceSingleThreaded, bool bWithClusters)
	{
		/** Growing array of objects that require serialization */
		FGCArrayStruct* ArrayStruct = FGCArrayPool::Get().GetArrayStructFromPool();
		TArray<UObject*>& ObjectsToSerialize = ArrayStruct->ObjectsToSerialize;

		// Reset object count.
		GObjectCountDuringLastMarkPhase.Reset();

		// Make sure GC referencer object is checked for references to other objects even if it resides in permanent object pool
		if (FPlatformProperties::RequiresCookedData() && FGCObject::GGCObjectReferencer && GUObjectArray.IsDisregardForGC(FGCObject::GGCObjectReferencer))
		{
			ObjectsToSerialize.Add(FGCObject::GGCObjectReferencer);
		}

		{
			const double StartTime = FPlatformTime::Seconds();
			//此处进行标记操作，根据是否有簇和是否有多线程调用不同的函数
			(this->*MarkObjectsFunctions[GetGCFunctionIndex(!bForceSingleThreaded, bWithClusters)])(ObjectsToSerialize, KeepFlags);
			UE_LOG(LogGarbage, Verbose, TEXT("%f ms for Mark Phase (%d Objects To Serialize"), (FPlatformTime::Seconds() - StartTime) * 1000, ObjectsToSerialize.Num());
		}

		{
			const double StartTime = FPlatformTime::Seconds();
			//此处进行可达性分析
			PerformReachabilityAnalysisOnObjects(ArrayStruct, bForceSingleThreaded, bWithClusters);
			UE_LOG(LogGarbage, Verbose, TEXT("%f ms for Reachability Analysis"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}
        
		// Allowing external systems to add object roots. This can't be done through AddReferencedObjects
		// because it may require tracing objects (via FGarbageCollectionTracer) multiple times
		FCoreUObjectDelegates::TraceExternalRootsForReachabilityAnalysis.Broadcast(*this, KeepFlags, bForceSingleThreaded);

		FGCArrayPool::Get().ReturnToPool(ArrayStruct);
	}
	virtual void PerformReachabilityAnalysisOnObjects(FGCArrayStruct* ArrayStruct, bool bForceSingleThreaded, bool bWithClusters) override
	{
		(this->*ReachabilityAnalysisFunctions[GetGCFunctionIndex(!bForceSingleThreaded, bWithClusters)])(ArrayStruct);
	}
```

## MarkObjectsAsUnreachable

标记所有没有KeepFlags和EInternalObjectFlags::GarbageCollectionKeepFlags的对象为不可达。

```c++
	/** 
	 * Marks all objects that don't have KeepFlags and EInternalObjectFlags::GarbageCollectionKeepFlags as unreachable
	 * This function is a template to speed up the case where we don't need to assemble the token stream (saves about 6ms on PS4)
	 */
	//标记objects为不可达
	//ObjectToSerialize引用类型，作为类似返回值的东西
	template <bool bParallel, bool bWithClusters>
	void MarkObjectsAsUnreachable(TArray<UObject*>& ObjectsToSerialize, const EObjectFlags KeepFlags)
	{
		const EInternalObjectFlags FastKeepFlags = EInternalObjectFlags::GarbageCollectionKeepFlags;

		TLockFreePointerListFIFO<UObject, PLATFORM_CACHE_LINE_SIZE> ObjectsToSerializeList;
		TLockFreePointerListFIFO<FUObjectItem, PLATFORM_CACHE_LINE_SIZE> ClustersToDissolveList;
		TLockFreePointerListFIFO<FUObjectItem, PLATFORM_CACHE_LINE_SIZE> KeepClusterRefsList;

		int32 MaxNumberOfObjects = GUObjectArray.GetObjectArrayNum() - GUObjectArray.GetFirstGCIndex();
		int32 NumThreads = FMath::Max(1, FTaskGraphInterface::Get().GetNumWorkerThreads());
		int32 NumberOfObjectsPerThread = (MaxNumberOfObjects / NumThreads) + 1;		

		// Iterate over all objects. Note that we iterate over the UObjectArray and usually check only internal flags which
		// are part of the array so we don't suffer from cache misses as much as we would if we were to check ObjectFlags.
		//遍历所有objects。
		ParallelFor(NumThreads, [&ObjectsToSerializeList, &ClustersToDissolveList, &KeepClusterRefsList, FastKeepFlags, KeepFlags, NumberOfObjectsPerThread, NumThreads, MaxNumberOfObjects](int32 ThreadIndex)
		{
			int32 FirstObjectIndex = ThreadIndex * NumberOfObjectsPerThread + GUObjectArray.GetFirstGCIndex();
			int32 NumObjects = (ThreadIndex < (NumThreads - 1)) ? NumberOfObjectsPerThread : (MaxNumberOfObjects - (NumThreads - 1) * NumberOfObjectsPerThread);
			int32 LastObjectIndex = FMath::Min(GUObjectArray.GetObjectArrayNum() - 1, FirstObjectIndex + NumObjects - 1);
			int32 ObjectCountDuringMarkPhase = 0;

			for (int32 ObjectIndex = FirstObjectIndex; ObjectIndex <= LastObjectIndex; ++ObjectIndex)
			{
				FUObjectItem* ObjectItem = &GUObjectArray.GetObjectItemArrayUnsafe()[ObjectIndex];
				if (ObjectItem->Object)
				{
					UObject* Object = (UObject*)ObjectItem->Object;

					// We can't collect garbage during an async load operation and by now all unreachable objects should've been purged.
					checkf(!ObjectItem->IsUnreachable(), TEXT("%s"), *Object->GetFullName());

					// Keep track of how many objects are around.
					ObjectCountDuringMarkPhase++;
					
					//模板中的变量
					if (bWithClusters)
					{
						ObjectItem->ClearFlags(EInternalObjectFlags::ReachableInCluster);
					}
					// Special case handling for objects that are part of the root set.
					if (ObjectItem->IsRootSet())
					{
						// IsValidLowLevel is extremely slow in this loop so only do it in debug
						//只在debug的时候做IsValidLowLevel因为它实在太慢了
						checkSlow(Object->IsValidLowLevel());
						// We cannot use RF_PendingKill on objects that are part of the root set.
#if DO_GUARD_SLOW
						checkCode(if (ObjectItem->IsPendingKill()) { UE_LOG(LogGarbage, Fatal, TEXT("Object %s is part of root set though has been marked RF_PendingKill!"), *Object->GetFullName()); });
#endif
						if (bWithClusters)
						{
							if (ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot) || ObjectItem->GetOwnerIndex() > 0)
							{
								KeepClusterRefsList.Push(ObjectItem);
							}
						}

						ObjectsToSerializeList.Push(Object);
					}
					// Regular objects or cluster root objects
					else if (!bWithClusters || ObjectItem->GetOwnerIndex() <= 0)
					{
						bool bMarkAsUnreachable = true;
						if (!ObjectItem->IsPendingKill())
						{
							// Internal flags are super fast to check
							if (ObjectItem->HasAnyFlags(FastKeepFlags))
							{
								bMarkAsUnreachable = false;
							}
							// If KeepFlags is non zero this is going to be very slow due to cache misses
							else if (KeepFlags != RF_NoFlags && Object->HasAnyFlags(KeepFlags))
							{
								bMarkAsUnreachable = false;
							}
						}
						else if (bWithClusters && ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
						{
							ClustersToDissolveList.Push(ObjectItem);
						}

						// Mark objects as unreachable unless they have any of the passed in KeepFlags set and it's not marked for elimination..
						if (!bMarkAsUnreachable)
						{
							// IsValidLowLevel is extremely slow in this loop so only do it in debug
							checkSlow(Object->IsValidLowLevel());
							ObjectsToSerializeList.Push(Object);

							if (bWithClusters)
							{
								if (ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
								{
									KeepClusterRefsList.Push(ObjectItem);
								}
							}
						}
						else
						{
							ObjectItem->SetFlags(EInternalObjectFlags::Unreachable);
						}
					}
				}
			}

			GObjectCountDuringLastMarkPhase.Add(ObjectCountDuringMarkPhase);
		}, !bParallel);
		
		ObjectsToSerializeList.PopAll(ObjectsToSerialize);

		if (bWithClusters)
		{
			TArray<FUObjectItem*> ClustersToDissolve;
			ClustersToDissolveList.PopAll(ClustersToDissolve);
			for (FUObjectItem* ObjectItem : ClustersToDissolve)
			{
				// Check if the object is still a cluster root - it's possible one of the previous
				// DissolveClusterAndMarkObjectsAsUnreachable calls already dissolved its cluster
				if (ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
				{
					GUObjectClusters.DissolveClusterAndMarkObjectsAsUnreachable(ObjectItem);
					GUObjectClusters.SetClustersNeedDissolving();
				}
			}
		}

		if (bWithClusters)
		{
			TArray<FUObjectItem*> KeepClusterRefs;
			KeepClusterRefsList.PopAll(KeepClusterRefs);
			for (FUObjectItem* ObjectItem : KeepClusterRefs)
			{
				if (ObjectItem->GetOwnerIndex() > 0)
				{
					checkSlow(!ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot));
					bool bNeedsDoing = !ObjectItem->HasAnyFlags(EInternalObjectFlags::ReachableInCluster);
					if (bNeedsDoing)
					{
						ObjectItem->SetFlags(EInternalObjectFlags::ReachableInCluster);
						// Make sure cluster root object is reachable too
						const int32 OwnerIndex = ObjectItem->GetOwnerIndex();
						FUObjectItem* RootObjectItem = GUObjectArray.IndexToObjectUnsafeForGC(OwnerIndex);
						checkSlow(RootObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot));
						// if it is reachable via keep flags we will do this below (or maybe already have)
						if (RootObjectItem->IsUnreachable()) 
						{
							RootObjectItem->ClearFlags(EInternalObjectFlags::Unreachable);
							// Make sure all referenced clusters are marked as reachable too
							FGCReferenceProcessor<false, bWithClusters>::MarkReferencedClustersAsReachable(RootObjectItem->GetClusterIndex(), ObjectsToSerialize);
						}
					}
				}
				else
				{
					checkSlow(ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot));
					// this thing is definitely not marked unreachable, so don't test it here
					// Make sure all referenced clusters are marked as reachable too
					FGCReferenceProcessor<false, bWithClusters>::MarkReferencedClustersAsReachable(ObjectItem->GetClusterIndex(), ObjectsToSerialize);
				}
			}
		}
	}

```

### GUObjectArray

GUobjectArray是一个全局数组，全部object对象都在此数组内，在objectbase初始化的阶段就会将object添加到此数组内：

```c++
void UObjectBase::AddObject(FName InName, EInternalObjectFlags InSetInternalFlags)
{
	AllocateUObjectIndexForCurrentThread(this);
	if (InternalFlagsToSet != EInternalObjectFlags::None)
	{
		GUObjectArray.IndexToObject(InternalIndex)->SetFlags(InternalFlagsToSet);
	}	
}
void AllocateUObjectIndexForCurrentThread(UObjectBase* Object)
{
	GUObjectArray.AllocateUObjectIndex(Object);
}
```

注意在这个数组内，有部分对象是永久不会被回收的，

```c++
	FORCEINLINE bool IsDisregardForGC(const class UObjectBase* Object)
	{
		return Object->InternalIndex <= ObjLastNonGCIndex;
	}
```

在`ObjLastNonGCIndex`之前的都不会被回收。

### 再回到主函数

通过全局object数组获取总object数量后，通过线程数将其分一定数量通过`ParallelFor`进行每部分的处理。内部对每个对象的主要逻辑：

注意object被objectItem封装。

整个并行循环所做的事主要是通过判断各种条件将UObject和UObjectItem添加到相应List内。

```c++
		TLockFreePointerListFIFO<UObject, PLATFORM_CACHE_LINE_SIZE> ObjectsToSerializeList;
		TLockFreePointerListFIFO<FUObjectItem, PLATFORM_CACHE_LINE_SIZE> ClustersToDissolveList;
		TLockFreePointerListFIFO<FUObjectItem, PLATFORM_CACHE_LINE_SIZE> KeepClusterRefsList;
```

`ObjectsToSerializeList`最终将会赋值给此函数传入的参数即`ObjectsToSerialize`,而

## PerformReachabilityAnalysisOnObjectsInternal

## TFastReferenceCollector

通过遍历UClass的令牌流来查找UObject引用并调用AddReferencedObjects。

这个类依靠三个组件，`ReferenceProcessor,ReferenceCollector,ArrayPool`，假设这三个组件内容：

```c++
  	class FSampleReferenceProcessor
   	{
   	public:
     	int32 GetMinDesiredObjectsPerSubTask() const;
		 void HandleTokenStreamObjectReference(TArray<UObject*>& ObjectsToSerialize, UObject* ReferencingObject, UObject*& Object, const int32 TokenIndex, bool bAllowReferenceElimination);
		 void UpdateDetailedStats(UObject* CurrentObject, uint32 DeltaCycles);
		 void LogDetailedStatsSummary();
	};

	 class FSampleCollctor : public FReferenceCollector 
	 {
	   // Needs to implement FReferenceCollector pure virtual functions
	 };
   
	 class FSampleArrayPool
	 {
	   	 static FSampleArrayPool& Get();
		 FGCArrayStruct* GetArrayStructFromPool();
		 void ReturnToPool(FGCArrayStruct*ArrayStruct);
	 };
```

## FGCReferenceProcessor

## FGCCollector

## FGCArrayPool

## FGCReferenceTokenStream

## 模板技巧

## 可达性分析流程

#### CollectGarbageInternal

```c++
		// Perform reachability analysis.
		//进行可达性分析
		{
			const double StartTime = FPlatformTime::Seconds();
			FRealtimeGC TagUsedRealtimeGC;
			TagUsedRealtimeGC.PerformReachabilityAnalysis(KeepFlags, bForceSingleThreadedGC, bWithClusters);
			UE_LOG(LogGarbage, Log, TEXT("%f ms for GC"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}
```

#### PerformReachabilityAnalysis

```c++
		{
			const double StartTime = FPlatformTime::Seconds();
			//此处进行标记操作，根据是否有簇和是否有多线程调用不同的函数
			(this->*MarkObjectsFunctions[GetGCFunctionIndex(!bForceSingleThreaded, bWithClusters)])(ObjectsToSerialize, KeepFlags);
			UE_LOG(LogGarbage, Verbose, TEXT("%f ms for Mark Phase (%d Objects To Serialize"), (FPlatformTime::Seconds() - StartTime) * 1000, ObjectsToSerialize.Num());
		}

		{
			const double StartTime = FPlatformTime::Seconds();
			//此处进行可达性分析
			PerformReachabilityAnalysisOnObjects(ArrayStruct, bForceSingleThreaded, bWithClusters);
			UE_LOG(LogGarbage, Verbose, TEXT("%f ms for Reachability Analysis"), (FPlatformTime::Seconds() - StartTime) * 1000);
		}
```

分别进行标记和可达性分析，用了函数指针数组来达到对于是否可以并行，是由有簇操作的四种情况调用同一模板函数,具体见上方模板技巧。

#### MarkObjectsAsUnreachable

将没有KeepFlags和EInternalObjectFlags::GarbageCollectionKeepFlags的标记为不可达,上方的`MarkObjectsFunctions`指向此函数。

主体部分迭代全局GUObjectArray的内容，进行标记。

#### PerformReachabilityAnalysisOnObjectsInternal

进行可达性分析的主体函数，

```c++
	template <bool bParallel, bool bWithClusters>
	void PerformReachabilityAnalysisOnObjectsInternal(FGCArrayStruct* ArrayStruct)
	{
		FGCReferenceProcessor<bParallel, bWithClusters> ReferenceProcessor;
		// NOTE: we want to run with automatic token stream generation off as it should be already generated at this point,
		// BUT we want to be ignoring Noop tokens as they're only pointing either at null references or at objects that never get GC'd (native classes)
		TFastReferenceCollector<bParallel, 
			FGCReferenceProcessor<bParallel, bWithClusters>, 
			FGCCollector<bParallel, bWithClusters>, 
			FGCArrayPool, 
			/* bAutoGenerateTokenStream = */ false, 
			/* bIgnoreNoopTokens = */ true> ReferenceCollector(ReferenceProcessor, FGCArrayPool::Get());
		ReferenceCollector.CollectReferences(*ArrayStruct);
	}
```

主要借助`TFastReferenceCollector`进行可达性分析,而`TFastReferenceCollector`依赖与`TGCReferenceProcessor,FGCCollector,FGCArrayPool`三个对象辅助实现:

#### CollectReferences

进行可达性分析。注意`ObjectsToSerialize`引用的是`ArrayStruct->ObjectsToSerialize`,因此此处传入的主题内容是带有之前标记阶段添加的objects。

```c++
		TArray<UObject*>& ObjectsToCollectReferencesFor = ArrayStruct.ObjectsToSerialize;
		if (ObjectsToCollectReferencesFor.Num())
		{
			if (!bParallel)
			{
				FGraphEventRef InvalidRef;
				ProcessObjectArray(ArrayStruct, InvalidRef);
			}
        }
```

主要看下非并行化的部分：

#### ProcessObjectArray

遍历UObject的token流来查找现有引用

```c++
	//
	/**
	 * Traverses UObject token stream to find existing references
	 *
	 * @param InObjectsToSerializeArray Objects to process
	 * @param MyCompletionGraphEvent Task graph event
	 */
	void ProcessObjectArray(FGCArrayStruct& InObjectsToSerializeStruct, const FGraphEventRef& MyCompletionGraphEvent)
	{

     	UObject* CurrentObject = nullptr;
		/** Growing array of objects that require serialization */
		FGCArrayStruct&	NewObjectsToSerializeStruct = *ArrayPool.GetArrayStructFromPool();

		// Ping-pong between these two arrays if there's not enough objects to spawn a new task
		TArray<UObject*>& ObjectsToSerialize = InObjectsToSerializeStruct.ObjectsToSerialize;
		TArray<UObject*>& NewObjectsToSerialize = NewObjectsToSerializeStruct.ObjectsToSerialize;
		int32 CurrentIndex = 0;
		do
		{
			CollectorType ReferenceCollector(ReferenceProcessor, NewObjectsToSerializeStruct);
			while (CurrentIndex < ObjectsToSerialize.Num())
			{
				CurrentObject = ObjectsToSerialize[CurrentIndex++];
				ObjectClass->AssembleReferenceTokenStream();
				ReferenceProcessor.SetCurrentObject(CurrentObject);
				
				// Get pointer to token stream and jump to the start.
				FGCReferenceTokenStream* RESTRICT TokenStream = &CurrentObject->GetClass()->ReferenceTokenStream;
				uint32 TokenStreamIndex = 0;
				// Keep track of index to reference info. Used to avoid LHSs.
				uint32 ReferenceTokenStreamIndex = 0;

				// Create stack entry and initialize sane values.
				uint8* StackEntryData = (uint8*)CurrentObject;
				// Keep track of token return count in separate integer as arrays need to fiddle with it.
				int32 TokenReturnCount = 0;

				//解析token流
				// Parse the token stream.
				while (true)
				{
					// Cache current token index as it is the one pointing to the reference info.
					ReferenceTokenStreamIndex = TokenStreamIndex;
					TokenStreamIndex++;
					FGCReferenceInfo ReferenceInfo = TokenStream->AccessReferenceInfo(ReferenceTokenStreamIndex);
					switch(ReferenceInfo.Type)
					{
					case GCRT_Object:
					case GCRT_Class:
					{
						// We're dealing with an object reference (this code should be identical to GCRT_NoopClass if !bIgnoreNoopTokens)
						//通过偏移量找到对象指针，最终找到对象引用
						UObject**	ObjectPtr = (UObject**)(StackEntryData + ReferenceInfo.Offset);
						UObject*&	Object = *ObjectPtr;
						TokenReturnCount = ReferenceInfo.ReturnCount;
						//注意，此时是CurrentObject引用object
						ReferenceProcessor.HandleTokenStreamObjectReference(NewObjectsToSerialize, CurrentObject, Object, ReferenceTokenStreamIndex, true);
					}
					}

			}

		}while (CurrentIndex < ObjectsToSerialize.Num());
	}
```

先对每个对象生成token流，

```c++
UClass* ObjectClass = CurrentObject->GetClass();
if (!ObjectClass->HasAnyClassFlags(CLASS_TokenStreamAssembled))
{
//生成token流
ObjectClass->AssembleReferenceTokenStream();
```

#### AssembleReferenceTokenStream

生成token流。主要操作对象为类的属性和类的父类。

```c++
		for( TFieldIterator<FProperty> It(this,EFieldIteratorFlags::ExcludeSuper); It; ++It)
		{
			FProperty* Property = *It;
			Property->EmitReferenceInfo(*this, 0, EncounteredStructProps);
		}
		if (UClass* SuperClass = GetSuperClass())
		{	
			// Make sure super class has valid token stream.
			SuperClass->AssembleReferenceTokenStream();
			if (!SuperClass->ReferenceTokenStream.IsEmpty())
			{
				// Prepend super's stream. This automatically handles removing the EOS token.
				ReferenceTokenStream.PrependStream(SuperClass->ReferenceTokenStream);
			}
		}
```

对于属性，会调用到不同属性所实现的`EmitReferenceInfo`：

```c++
void FObjectProperty::EmitReferenceInfo(UClass& OwnerClass, int32 BaseOffset, TArray<const FStructProperty*>& EncounteredStructProps)
{
	FGCReferenceFixedArrayTokenHelper FixedArrayHelper(OwnerClass, BaseOffset + GetOffset_ForGC(), ArrayDim, sizeof(UObject*), *this);
	OwnerClass.EmitObjectReference(BaseOffset + GetOffset_ForGC(), GetFName(), GCRT_Object);
}
```

可以看到最终调用到`EmitObjectReference`

```c++

void UClass::EmitObjectReference(int32 Offset, const FName& DebugName, EGCReferenceType Kind)
{
	FGCReferenceInfo ObjectReference(Kind, Offset);
	//将ObjectReference作为int型传入token流，objectreference由union类型，int32的前8位，中5位，后19位对应不同的信息。
	ReferenceTokenStream.EmitReferenceInfo(ObjectReference, DebugName);
}
```

`ReferenceTokenStream`是UClass的一个类内公共成员:

```c++
	/** Reference token stream used by realtime garbage collector, finalized in AssembleReferenceTokenStream */
	FGCReferenceTokenStream ReferenceTokenStream;
```

以上先将类型信息和偏移量存入一个`FGCReferenceInfo`中，然后将FGCReferenceInfo对象传入Token流对象中作为一个int值（由于`FGCReferenceInfo`内部是一个union，因此可以在一个结构体和一个int之间转换）。

```c++
/** 
 * Convenience struct containing all necessary information for a reference.
 */
//包含了所有必要的引用信息
struct FGCReferenceInfo
{
	/**
	 * Constructor
	 *
	 * @param InType	type of reference
	 * @param InOffset	offset into object/ struct
	 */
	FORCEINLINE FGCReferenceInfo( EGCReferenceType InType, uint32 InOffset )
	:	ReturnCount( 0 )
	,	Type( InType )
	,	Offset( InOffset )	
	{
		check( InType != GCRT_None );
		check( (InOffset & ~0x7FFFF) == 0 );
	}
	/**
	 * Constructor
	 *
	 * @param InValue	value to set union mapping to a uint32 to
	 */
	FORCEINLINE FGCReferenceInfo( uint32 InValue )
	:	Value( InValue )
	{}
	/**
	 * uint32 conversion operator
	 *
	 * @return uint32 value of struct
	 */
	FORCEINLINE operator uint32() const 
	{ 
		return Value; 
	}

	/** Mapping to exactly one uint32 */
	union
	{
		/** Mapping to exactly one uint32 */
		struct
		{
			/** Return depth, e.g. 1 for last entry in an array, 2 for last entry in an array of structs of arrays, ... */
			uint32 ReturnCount	: 8;
			/** Type of reference *///引用类型
			uint32 Type			: 5;
			/** Offset into struct/ object *///偏移量
			uint32 Offset		: 19;
		};
		/** uint32 value of reference info, used for easy conversion to/ from uint32 for token array */
		uint32 Value;
	};
};

```

可以看到这个存储引用信息的类，构造函数用于接收具体信息，一个union结构用于和int的转化，`operator`转换使类可以转换为int。

再看tokenStream的`EmitReferenceInfo`函数:

```c++
int32 FGCReferenceTokenStream::EmitReferenceInfo(FGCReferenceInfo ReferenceInfo, const FName& DebugName)
{
	int32 TokenIndex = Tokens.Add(ReferenceInfo);
#if ENABLE_GC_OBJECT_CHECKS
	check(TokenDebugInfo.Num() == TokenIndex);
	TokenDebugInfo.Add(DebugName);
#endif
	return TokenIndex;
}
```

主要关注下tokens数组的添加，由于Tokens是一个int数组，因此`ReferenceInfo`调用转换函数变为int传入，到合适时候又可以通过索引取出此值，转换为info后可以获取相应类型和偏移量信息，可以通过此偏移量获取到具体对象指针,即可找到对象。因此用一个int数组即存储了一个对象的引用信息。

生成tokenstream后继续回到ProcessObjectArray中，

此时获取`TokenStream`并进行对tokenstream的解析。

```c++
FGCReferenceTokenStream* RESTRICT TokenStream = &CurrentObject->GetClass()->ReferenceTokenStream;
uint32 TokenStreamIndex = 0;
// Keep track of index to reference info. Used to avoid LHSs.
uint32 ReferenceTokenStreamIndex = 0;
uint8* StackEntryData = (uint8*)CurrentObject;
while (true)
{
// Cache current token index as it is the one pointing to the reference info.
ReferenceTokenStreamIndex = TokenStreamIndex;
TokenStreamIndex++;
FGCReferenceInfo ReferenceInfo = TokenStream->AccessReferenceInfo(ReferenceTokenStreamIndex);
switch(ReferenceInfo.Type)
{
case GCRT_Object:
case GCRT_Class:
{
// We're dealing with an object reference (this code should be identical to GCRT_NoopClass if !bIgnoreNoopTokens)
//通过偏移量找到对象指针，最终找到对象引用
UObject**	ObjectPtr = (UObject**)(StackEntryData + ReferenceInfo.Offset);
UObject*&	Object = *ObjectPtr;
TokenReturnCount = ReferenceInfo.ReturnCount;
//注意，此时是CurrentObject引用object
ReferenceProcessor.HandleTokenStreamObjectReference(NewObjectsToSerialize, CurrentObject, Object, ReferenceTokenStreamIndex, true);
}
}
}
```

可以看到，通过token流获取具体对象引用，然后将当前对象和被引用的对象传入`HandleTokenStreamObjectReference`中进行处理：

#### HandleTokenStreamObjectReference | HandleObjectReference

```c++
	/**
	* Handles UObject reference from the token stream.
	*
	* @param ObjectsToSerialize An array of remaining objects to serialize.
	* @param ReferencingObject Object referencing the object to process.
	* @param TokenIndex Index to the token stream where the reference was found.
	* @param bAllowReferenceElimination True if reference elimination is allowed.
	*/
	//处理UObject的引用 从token流中
	FORCEINLINE void HandleTokenStreamObjectReference(TArray<UObject*>& ObjectsToSerialize, UObject* ReferencingObject, UObject*& Object, const int32 TokenIndex, bool bAllowReferenceElimination)
	{
		HandleObjectReference(ObjectsToSerialize, ReferencingObject, Object, bAllowReferenceElimination);
	}
```

```c++
	//处理对象引用
	FORCEINLINE void HandleObjectReference(TArray<UObject*>& ObjectsToSerialize, const UObject * const ReferencingObject, UObject*& Object, const bool bAllowReferenceElimination)
	{

		//将被引用的对象标记为可达的
		const int32 ObjectIndex = GUObjectArray.ObjectToIndex(Object);
		FUObjectItem* ObjectItem = GUObjectArray.IndexToObjectUnsafeForGC(ObjectIndex);
		// Remove references to pending kill objects if we're allowed to do so.
		//如果允许将pending kill的引用置空
		if (ObjectItem->IsPendingKill() && bAllowReferenceElimination)
		{
			//checkSlow(ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot) == false);
			checkSlow(ObjectItem->GetOwnerIndex() <= 0)

			// Null out reference.
			Object = NULL;
		}
		// Add encountered object reference to list of to be serialized objects if it hasn't already been added.
		else if (ObjectItem->IsUnreachable())
		{
			if (bParallel)
			{
				// Mark it as reachable.
				if (ObjectItem->ThisThreadAtomicallyClearedRFUnreachable())
				{
					// Objects that are part of a GC cluster should never have the unreachable flag set!
					checkSlow(ObjectItem->GetOwnerIndex() <= 0);

					if (!bWithClusters || !ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
					{
						// Add it to the list of objects to serialize.
						ObjectsToSerialize.Add(Object);
					}
					else
					{
						// This is a cluster root reference so mark all referenced clusters as reachable
						MarkReferencedClustersAsReachable(ObjectItem->GetClusterIndex(), ObjectsToSerialize);
					}
				}
			}
			else
			{
				// Mark it as reachable.
				ObjectItem->ClearUnreachable();
				// Objects that are part of a GC cluster should never have the unreachable flag set!
				checkSlow(ObjectItem->GetOwnerIndex() <= 0);
				if (!bWithClusters || !ObjectItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot))
				{
					// Add it to the list of objects to serialize.
					ObjectsToSerialize.Add(Object);
				}
				else
				{
					// This is a cluster root reference so mark all referenced clusters as reachable
					MarkReferencedClustersAsReachable(ObjectItem->GetClusterIndex(), ObjectsToSerialize);
				}
			}
		}
	}
```

可以看到主要做了两件事：1. 将被引用的obejct标记为可达，2. 将obejct添加到`ObjectsToSerialize`中。

由于`ProcessObjectArray`的主循环:

```c++
while (CurrentIndex < ObjectsToSerialize.Num())
```

是每次获取数组数量的，因此添加到此数组内会被最终访问到。

## 清除流程

### GatherUnreachableObjects

遍历全局UObject数组,将不可达的对象收集到一个全局数组内`GUnreachableObjects`。

### UnhashUnreachableObjects

如果是全量清除，对不可达的对象调用ConditionalBeginDestroy。

### DeleteLoaders

销毁所有准备删除的linkers。