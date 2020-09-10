# 序列化(ue4.25)

## 访问者模式

ue4序列化采用了设计模式中的**访问者模式**，[具体介绍](https://www.runoob.com/design-pattern/visitor-pattern.html)

此模式主要是将**数据结构和数据操作分离**。降低耦合。

使用场景：需要对一个对象结构中的对象进行很多不同的并且不相关的操作，而需要避免让这些操作"污染"这些对象的类，使用访问者模式将这些封装到类中。

优点：

1. 符合单一职责原则
2. 优秀的扩展性
3. 灵活性

缺点：

1. 具体元素对访问者公布细节，违反了迪米特法则。
2. 具体元素变更困难。
3. 违反了依赖倒置原则，依赖了具体类，没有依赖抽象。

## 序列化中的访问者模式应用

![](https://github.com/whukxggx/ue4_doc/blob/master/Archive%E8%AE%BF%E9%97%AE%E8%80%85.png?raw=true)

![](https://github.com/whukxggx/ue4_doc/blob/master/UObjcet%E8%A2%AB%E8%AE%BF%E9%97%AE%E8%80%85.png?raw=true)

如上，FArchive为访问者，UObject为被访问者。

其中FArchive主要由两种类型操作，一个是operator<<对于不同数据类型的重载，另一个是多种serialize\****的实现。大多数情况下，UObject或其他类接收到访问者FArchive类型后，调用其重载的<<即可进行相应操作，而不必调用具体Serialize函数。举例如下：

```c++
FArchive& FArchive::operator<<( FText& Value )
{
	FText::SerializeText(*this, Value);
	return *this;
}
	FORCEINLINE friend FArchive& operator<<(FArchive& Ar, ANSICHAR& Value)
	{
#if DEVIRTUALIZE_FLinkerLoad_Serialize
		if (!Ar.FastPathLoad<sizeof(Value)>(&Value))
#endif
		{
			Ar.Serialize(&Value, 1);
		}
		return Ar;
	}
```

```c++
	virtual void Serialize(void* V, int64 Length) { }

	virtual void SerializeBits(void* V, int64 LengthBits)
	{
		Serialize(V, (LengthBits + 7) / 8);

		if (IsLoading() && (LengthBits % 8) != 0)
		{
			((uint8*)V)[LengthBits / 8] &= ((1 << (LengthBits & 7)) - 1);
		}
	}

	virtual void SerializeInt(uint32& Value, uint32 Max)
	{
		ByteOrderSerialize(Value);
	}
```

可以看到operator<<中的实现用的多是具体的Serialize函数，而serialize函数基本都会最终调用到`void Serialize(void* V, int64 Length)`这个函数，即FArchive留给子类实现具体功能的接口。子类通过实现此接口改变具体行为，从而可以使用父类中某些已写好的重载函数。如`FArchiveFileWriterGeneric`和`FArchiveFileReaderGeneric`都是继承自FArchive类，其最主要的区别就是对serialize的重写内容：

```c++
//注意，有大量删减
void FArchiveFileReaderGeneric::Serialize( void* V, int64 Length )
{
	ReadLowLevel(( uint8* )V, Length, Count );
}
```

```c++
//大量删减
void FArchiveFileWriterGeneric::Serialize( void* V, int64 Length )
{
	WriteLowLevel( (uint8*)V, Length );
}
```

由于不同的子类archive通过serialize实现不同的功能，最终反馈到父类的operator<<,最终再用到被访问者(UObject及其子类)

再看UObject类的接口:

```c++
class COREUOBJECT_API UObject : public UObjectBaseUtility{
	virtual void Serialize(FArchive& Ar);
	virtual void Serialize(FStructuredArchive::FRecord Record);
}
```

```c++
IMPLEMENT_FARCHIVE_SERIALIZER(UObject) 

//有大量删减，仅作示意
//FStructuredArchive::FRecord就是FStructuredArchiveRecord
void UObject::Serialize(FStructuredArchive::FRecord Record)
{
    	UClass* ObjClass = GetClass();
		UObject* LoadOuter = GetOuter();
		FName LoadName = GetFName();
		Record << SA_VALUE(TEXT("LoadName"), LoadName);
		Record << SA_VALUE(TEXT("LoadOuter"), LoadOuter);
		Record << SA_VALUE(TEXT("ObjClass"), ObjClass);
}
```

`IMPLEMENT_FARCHIVE_SERIALIZER(UObject) `是`virtual void Serialize(FArchive& Ar);`的实现，最终转调用了void UObject::Serialize(FStructuredArchive::FRecord Record),此函数内具体调用了operator<<函数，

```c++
	template<typename T> FORCEINLINE FStructuredArchiveRecord& operator<<(TNamedValue<T> Item)
	{
		EnterField(Item.Name) << Item.Value;
		return *this;
	}
```

## 一个具体流程（savegame）

![](https://github.com/whukxggx/ue4_doc/blob/master/SaveGame%E4%BF%9D%E5%AD%98%E6%B5%81%E7%A8%8B%E5%9B%BE.png?raw=true)

先看一下USaveGame的具体声明,通过调用UGameplayStatics进行相应操作:

```c++
UCLASS(abstract, Blueprintable, BlueprintType)
class ENGINE_API USaveGame : public UObject
{
	/**
	 *	@see UGameplayStatics::CreateSaveGameObject
	 *	@see UGameplayStatics::SaveGameToSlot
	 *	@see UGameplayStatics::DoesSaveGameExist
	 *	@see UGameplayStatics::LoadGameFromSlot
	 *	@see UGameplayStatics::DeleteGameInSlot
	 */
	GENERATED_UCLASS_BODY()
};
```

先进行实例创建：

```c++
if (UMySaveGame* SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())))
{
    // 设置savegame对象上的数据。
    SaveGameInstance->PlayerName = TEXT("PlayerOne");
    if (UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotNameString, UserIndexInt32))
    {
        // 成功保存。
    }
}
```

然后调用`SaveGameToSlot`进行保存操作，分为两个函数进行，`SaveGameToMemory`和`SaveDataToSlot`,

```c++
bool UGameplayStatics::SaveGameToSlot(USaveGame* SaveGameObject, const FString& SlotName, const int32 UserIndex)
{
	// This is a wrapper around the functions reading to/from a byte array
	TArray<uint8> ObjectBytes;
	if (SaveGameToMemory(SaveGameObject, ObjectBytes))
	{
		return SaveDataToSlot(ObjectBytes, SlotName, UserIndex);
	}
	return false;
}
```

`SaveGameToMemory`函数使用`FMemoryWriter`进行写入操作,其中`SaveGameObject->Serialize(Ar);`由于savegame没有重写则会调用UObject的Serialize函数:

```c++
bool UGameplayStatics::SaveGameToMemory(USaveGame* SaveGameObject, TArray<uint8>& OutSaveData )
{
	if (SaveGameObject)
	{
		FMemoryWriter MemoryWriter(OutSaveData, true);

		FSaveGameHeader SaveHeader(SaveGameObject->GetClass());
		SaveHeader.Write(MemoryWriter);//内部调用其<<操作符进行相关信息操作

		// Then save the object state, replacing object refs and names with strings
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		SaveGameObject->Serialize(Ar);

		return true; // Not sure if there's a failure case here.
	}
	return false;
}
```

之后将调用SaveDataToSlot将信息存入文件,可以看到`FArchiveFileWriterGeneric`进行写入操作：

```c++
bool UGameplayStatics::SaveDataToSlot(const TArray<uint8>& InSaveData, const FString& SlotName, const int32 UserIndex)
{
	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();

	if (SaveSystem && InSaveData.Num() > 0 && SlotName.Len() > 0)
	{
		// Stuff that data into the save system with the desired file name
		return SaveSystem->SaveGame(false, *SlotName, UserIndex, InSaveData);
	}

	return false;
}
```

```c++
	virtual bool SaveGame(bool bAttemptToUseUI, const TCHAR* Name, const int32 UserIndex, const TArray<uint8>& Data) override
	{
		return FFileHelper::SaveArrayToFile(Data, *GetSaveGamePath(Name));
	}
```

```c++
bool FFileHelper::SaveArrayToFile(TArrayView<const uint8> Array, const TCHAR* Filename, IFileManager* FileManager /*= &IFileManager::Get()*/, uint32 WriteFlags)
{
    //此处就跟filearchive联系上了
	TUniquePtr<FArchive> Ar = TUniquePtr<FArchive>( FileManager->CreateFileWriter( Filename, WriteFlags ) );
	if( !Ar )
	{
		return false;
	}
	Ar->Serialize(const_cast<uint8*>(Array.GetData()), Array.Num());
	// Always explicitly close to catch errors from flush/close
	Ar->Close();
	return !Ar->IsError() && !Ar->IsCriticalError();
}
```

总的流程已经在流程图中比较完善了。`LoadGameFromSlot`与此过程相反。其中`CreateFileReader(Writer)`等操作如下：

![](https://github.com/whukxggx/ue4_doc/blob/master/FileReaderWriter%E5%9B%BE.png?raw=true)

CreateFileReader创建FArchiveFileReaderGeneric类，CreateFileWriter创建FArchiveFileWriterGeneric类，两个类分别负责文件读写。

```c++
class CORE_API FArchiveFileReaderGeneric : public FArchive
{
	virtual void Serialize( void* V, int64 Length ) final;
	/** 
	 * Platform specific read
	 * @param Dest - Buffer to fill in
	 * @param CountToRead - Number of bytes to read
	 * @param OutBytesRead - Bytes actually read
	**/
	virtual void ReadLowLevel(uint8* Dest, int64 CountToRead, int64& OutBytesRead);
};
```

```c++
class CORE_API FArchiveFileWriterGeneric : public FArchive
{
	virtual void Serialize( void* V, int64 Length ) final;
	/** 
	 * Platform specific write
	 * @param Src - Buffer to write out
	 * @param CountToWrite - Number of bytes to write
	 * @return false on failure 
	**/
	virtual bool WriteLowLevel(const uint8* Src, int64 CountToWrite);
};
```

重点关注其中的`void Serialize( void* V, int64 Length )`的重写内容,主要操作还是在`ReadLowLevel`和`WriteLowLevel`中进行的,这两个是每个平台都不一样的：

```c++
void FArchiveFileReaderGeneric::Serialize( void* V, int64 Length )
{
	if (Pos + Length > Size)
	{
		ArIsError = true;
		UE_LOG(LogFileManager, Error, TEXT("Requested read of %d bytes when %d bytes remain (file=%s, size=%d)"), Length, Size-Pos, *Filename, Size);
		return;
	}

	while( Length>0 )
	{
		int64 Copy = FMath::Min( Length, BufferBase+BufferArray.Num()-Pos );
		if( Copy<=0 )
		{
			if( Length >= BufferSize )
			{
				int64 Count=0;
				{
					ReadLowLevel(( uint8* )V, Length, Count );
				}
				if( Count!=Length )
				{
					TCHAR ErrorBuffer[1024];
					ArIsError = true;
					UE_LOG( LogFileManager, Warning, TEXT( "ReadFile failed: Count=%lld Length=%lld Error=%s for file %s" ), 
						Count, Length, FPlatformMisc::GetSystemErrorMessage( ErrorBuffer, 1024, 0 ), *Filename );
				}
				Pos += Length;
				return;
			}
			if (!InternalPrecache(Pos, MAX_int32))
			{
				ArIsError = true;
				UE_LOG( LogFileManager, Warning, TEXT( "ReadFile failed during precaching for file %s" ),*Filename );
				return;
			}
			Copy = FMath::Min( Length, BufferBase+BufferArray.Num()-Pos );
			if( Copy<=0 )
			{
				ArIsError = true;
				UE_LOG( LogFileManager, Error, TEXT( "ReadFile beyond EOF %lld+%lld/%lld for file %s" ), 
					Pos, Length, Size, *Filename );
			}
			if( ArIsError )
			{
				return;
			}
		}
		FMemory::Memcpy( V, BufferArray.GetData()+Pos-BufferBase, Copy );
		Pos       += Copy;
		Length    -= Copy;
		V          =( uint8* )V + Copy;
	}
}
```

```c++
void FArchiveFileWriterGeneric::Serialize( void* V, int64 Length )
{
	Pos += Length;
	if ( Length >= BufferSize )
	{
		FlushBuffer();
		if( !WriteLowLevel( (uint8*)V, Length ) )
		{
			ArIsError = true;
			LogWriteError(TEXT("Error writing to file"));
		}
	}
	else
	{
		int64 Copy;
		while( Length >( Copy=BufferSize-BufferArray.Num() ) )
		{
			BufferArray.Append((uint8*)V, Copy);
			Length      -= Copy;
			V            =( uint8* )V + Copy;
			FlushBuffer();
		}
		if( Length )
		{
			BufferArray.Append((uint8*)V, Length);
		}
	}
}
```

其中主要是通过`WriteLowLevel`和`ReadLowLevel`来达到目的。这两个函数的内部实现如下,

```c++
bool FArchiveFileWriterGeneric::WriteLowLevel( const uint8* Src, int64 CountToWrite )
{
	return Handle->Write( Src, CountToWrite );
}
void FArchiveFileReaderGeneric::ReadLowLevel( uint8* Dest, int64 CountToRead, int64& OutBytesRead )
{
	if( Handle->Read( Dest, CountToRead ) )
	{
		OutBytesRead = CountToRead;
	}
	else
	{
		OutBytesRead = 0;
	}
}
```

可以看到，是通过IFleHandle来实现的，此类型是在CreateFileReader(Writer)中进行获取的。

```c++
	FArchive* CreateFileReader( const TCHAR* Filename, uint32 ReadFlags=0 ) override
	{
		return CreateFileReaderInternal( Filename, ReadFlags, PLATFORM_FILE_READER_BUFFER_SIZE );
	}

	FArchive* CreateFileWriter( const TCHAR* Filename, uint32 WriteFlags=0 ) override
	{
		return CreateFileWriterInternal( Filename, WriteFlags, PLATFORM_FILE_WRITER_BUFFER_SIZE );
	}
```

在转调用的函数内部有`	IFileHandle* Handle = GetLowLevel().OpenRead( InFilename, !!(Flags & FILEREAD_AllowWrite) );`，此函数的调用流程如下：

![](https://github.com/whukxggx/ue4_doc/blob/master/FileHandle%E7%A1%AE%E5%AE%9A%E8%BF%87%E7%A8%8B.png?raw=true)

通过此过程获取不同平台的`IPlatformFile`类型和不同平台的句柄,用以进行最终的写入或者读出操作。

总的来看，就是farchive类作为访问者实现了基础类型的`operator<<`和`Serialize***`等，其子类实现`Serialize`从而实现不同的功能，UObject等其他被访问者提供接收访问者的接口并自己做相应的事（代码一样的情况下通过判断是否事加载过程来确定使用哪部分），不同平台的写入通过IFileManager来控制获取。

## FStructuredArchive相关

### pimpl

它通过一个私有的成员指针，将指针所指向的类的内部实现数据进行隐藏。是一种常用的，用来对“类的接口和实现”进行解耦的方法。这个技巧可以避免头文件中暴露相应实现细节。

优点：

1. 减少依赖性(降低耦合性):减少原类不必要的头文件的依赖；同时对Impl类进行修改无序重新编译原类。
2. 接口和实现的分离(隐藏了类的实现)
3. 可使用惰性分配技术，类的某部分实现可以写成按需分配或者实际使用时再分配，从而节省资源。

缺点：

1. 每个类需要占用额外指针
2. 每个类每次访问具体实现时都要多一个间接指针操作的开销，并且在使用、阅读和调试上都可能有所不便。

### 示例代码

```c++
class widget {
    class impl;
    std::experimental::propagate_const<std::unique_ptr<impl>> pImpl;
 public:
    void draw() const; // public API that will be forwarded to the implementation
    void draw();
    bool shown() const { return true; } // public API that implementation has to call
    widget(int);
    ~widget(); // defined in the implementation file, where impl is a complete type
    widget(widget&&); // defined in the implementation file
                      // Note: calling draw() on moved-from object is UB
    widget(const widget&) = delete;
    widget& operator=(widget&&); // defined in the implementation file
    widget& operator=(const widget&) = delete;
};
 
// implementation (widget.cpp)
class widget::impl {
    int n; // private data
 public:
    void draw(const widget& w) const {
        if(w.shown()) // this call to public member function requires the back-reference 
            std::cout << "drawing a const widget " << n << '\n';
    }
    void draw(const widget& w) {
        if(w.shown())
            std::cout << "drawing a non-const widget " << n << '\n';
    }
    impl(int n) : n(n) {}
};
void widget::draw() const { pImpl->draw(*this); }
void widget::draw() { pImpl->draw(*this); }
widget::widget(int n) : pImpl{std::make_unique<impl>(n)} {}
widget::widget(widget&&) = default;
widget::~widget() = default;
widget& widget::operator=(widget&&) = default;
```

而在ue4中，`FStructuredArchiveFromArchive`和`	struct FImpl;`就是类似关系。

注意UObject的`Serialize`的参数的获取就是从此开始的

```c++
#define IMPLEMENT_FARCHIVE_SERIALIZER( TClass ) void TClass::Serialize(FArchive& Ar) { TClass::Serialize(FStructuredArchiveFromArchive(Ar).GetSlot().EnterRecord()); }
```

```c++
class CORE_API FStructuredArchiveFromArchive
{
	UE_NONCOPYABLE(FStructuredArchiveFromArchive)

	static constexpr uint32 ImplSize      = 400;
	static constexpr uint32 ImplAlignment = 8;
	struct FImpl;
public:
	explicit FStructuredArchiveFromArchive(FArchive& Ar);
	~FStructuredArchiveFromArchive();

	FStructuredArchiveSlot GetSlot();

private:
	// Implmented as a pimpl in order to reduce dependencies, but an inline one to avoid heap allocations
	//此处alignas设定对齐方式
	alignas(ImplAlignment) uint8 ImplStorage[ImplSize];
};
```

但是此处FImpl并不直接实现接口,而是存储相应类型：

```c++
struct FStructuredArchiveFromArchive::FImpl
{
	explicit FImpl(FArchive& Ar)
		: Formatter(Ar)
		, StructuredArchive(Formatter)
		, Slot(StructuredArchive.Open())
	{
	}

	FBinaryArchiveFormatter Formatter;
	FStructuredArchive StructuredArchive;
	FStructuredArchive::FSlot Slot;
};
```

可以看到`FStructuredArchiveFromArchive`的实现函数如下：

```c++
FStructuredArchiveFromArchive::FStructuredArchiveFromArchive(FArchive& Ar)
{
	static_assert(FStructuredArchiveFromArchive::ImplSize >= sizeof(FStructuredArchiveFromArchive::FImpl), "FStructuredArchiveFromArchive::ImplSize must fit in the size of FStructuredArchiveFromArchive::Impl");
	static_assert(FStructuredArchiveFromArchive::ImplAlignment >= alignof(FStructuredArchiveFromArchive::FImpl), "FStructuredArchiveFromArchive::ImplAlignment must be compatible with the alignment of FStructuredArchiveFromArchive::Impl");

	new (ImplStorage) FImpl(Ar);
}

FStructuredArchiveFromArchive::~FStructuredArchiveFromArchive()
{
	DestructItem((FImpl*)ImplStorage);
}

FStructuredArchive::FSlot FStructuredArchiveFromArchive::GetSlot()
{
	return ((FImpl*)ImplStorage)->Slot;
}

```

使用placement new方法在类中分配好的内存上进行FImpl类型的初始化，获取具体Slot时则是对具体内存地址进行强转后获取其成员。

可以看到，UObject中的serialize就用了此结构体获取slot：

```c++
#define IMPLEMENT_FARCHIVE_SERIALIZER( TClass ) void TClass::Serialize(FArchive& Ar) { TClass::Serialize(FStructuredArchiveFromArchive(Ar).GetSlot().EnterRecord()); }
```

之后调用另一个以FStructuredArchiveRecord类型为参的Serialize函数。先看一个UML图：

![](https://github.com/whukxggx/ue4_doc/blob/master/FStructuredArchive%E7%9B%B8%E5%85%B3.png?raw=true)

总体初始化逻辑：

```c++
	explicit FImpl(FArchive& Ar)
		: Formatter(Ar)
		, StructuredArchive(Formatter)
		, Slot(StructuredArchive.Open())
	{
	}
```

获取到的slot可以获取record（两者可以相互获取）:

再来具体看一下`FStructuredArchiveRecord`和`FStructuredArchiveSlot`的具体实现

```c++
class CORE_API FStructuredArchiveRecord final : public UE4StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveSlot EnterField(FArchiveFieldName Name);
	FStructuredArchiveSlot EnterField_TextOnly(FArchiveFieldName Name, EArchiveValueType& OutType);
	FStructuredArchiveRecord EnterRecord(FArchiveFieldName Name);
	FStructuredArchiveRecord EnterRecord_TextOnly(FArchiveFieldName Name, TArray<FString>& OutFieldNames);
	FStructuredArchiveArray EnterArray(FArchiveFieldName Name, int32& Num);
	FStructuredArchiveStream EnterStream(FArchiveFieldName Name);
	FStructuredArchiveStream EnterStream_TextOnly(FArchiveFieldName Name, int32& OutNumElements);
	FStructuredArchiveMap EnterMap(FArchiveFieldName Name, int32& Num);

	TOptional<FStructuredArchiveSlot> TryEnterField(FArchiveFieldName Name, bool bEnterForSaving);

	template<typename T> FORCEINLINE FStructuredArchiveRecord& operator<<(TNamedValue<T> Item)
	{
		EnterField(Item.Name) << Item.Value;
		return *this;
	}

private:
	friend FStructuredArchive;
	friend FStructuredArchiveSlot;

	using UE4StructuredArchive_Private::FSlotBase::FSlotBase;
};
```

Record只实现了一种，即`TNamedValue`类型的operator<<重载，可以看到这个函数所做的事，调用`EnterField(Item.Name)`后将真正的Value的操作传给Slot类型。

```c++
class CORE_API FStructuredArchiveSlot final : public UE4StructuredArchive_Private::FSlotBase
{
public:
	FStructuredArchiveRecord EnterRecord();
	FStructuredArchiveRecord EnterRecord_TextOnly(TArray<FString>& OutFieldNames);
	FStructuredArchiveArray EnterArray(int32& Num);
	FStructuredArchiveStream EnterStream();
	FStructuredArchiveStream EnterStream_TextOnly(int32& OutNumElements);
	FStructuredArchiveMap EnterMap(int32& Num);
	FStructuredArchiveSlot EnterAttribute(FArchiveFieldName AttributeName);
	TOptional<FStructuredArchiveSlot> TryEnterAttribute(FArchiveFieldName AttributeName, bool bEnterWhenWriting);

	// We don't support chaining writes to a single slot, so this returns void.
	void operator << (uint8& Value);
	void operator << (uint16& Value);
	void operator << (uint32& Value);
	void operator << (uint64& Value);
	void operator << (int8& Value);
	void operator << (int16& Value);
	void operator << (int32& Value);
	void operator << (int64& Value);
	void operator << (float& Value);
	void operator << (double& Value);
	void operator << (bool& Value);
	void operator << (FString& Value);
	void operator << (FName& Value);
	void operator << (UObject*& Value);
	void operator << (FText& Value);
	void operator << (FWeakObjectPtr& Value);
	void operator << (FSoftObjectPtr& Value);
	void operator << (FSoftObjectPath& Value);
	void operator << (FLazyObjectPtr& Value);


	template <typename T>
	FORCEINLINE void operator<<(TNamedAttribute<T> Item)
	{
		EnterAttribute(Item.Name) << Item.Value;
	}

	template <typename T>
	FORCEINLINE void operator<<(TOptionalNamedAttribute<T> Item)
	{
		if (TOptional<FStructuredArchiveSlot> Attribute = TryEnterAttribute(Item.Name, Item.Value != Item.Default))
		{
			Attribute.GetValue() << Item.Value;
		}
		else
		{
			Item.Value = Item.Default;
		}
	}

	void Serialize(TArray<uint8>& Data);
	void Serialize(void* Data, uint64 DataSize);

	bool IsFilled() const;

private:
	friend FStructuredArchive;
	friend FStructuredArchiveChildReader;
	friend FStructuredArchiveSlot;
	friend FStructuredArchiveRecord;
	friend FStructuredArchiveArray;
	friend FStructuredArchiveStream;
	friend FStructuredArchiveMap;

	using UE4StructuredArchive_Private::FSlotBase::FSlotBase;
};
```

可以看到，slot负责大多数类型的<<重载。

由于两个都继承自FSlotBase，而FSlotBase继承自FSlotPosition，因此两者都有位置信息，enter**函数就是确定位置后进行相应位置的读写。

再回到savegame的具体流程。在`SaveGameToMemory`函数中，

```c++
bool UGameplayStatics::SaveGameToMemory(USaveGame* SaveGameObject, TArray<uint8>& OutSaveData )
{
		FMemoryWriter MemoryWriter(OutSaveData, true);

		FSaveGameHeader SaveHeader(SaveGameObject->GetClass());
		SaveHeader.Write(MemoryWriter);

		// Then save the object state, replacing object refs and names with strings
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		SaveGameObject->Serialize(Ar);
}
```

调用了这样一句话`SaveGameObject->Serialize(Ar);`，由于USaveGame类并没有重写这个类，因此调用的UObject类的Serialize函数。

在上方有UObject类的代码:

```c++
void UObject::Serialize(FStructuredArchive::FRecord Record)
{
		// These three items are very special items from a serialization standpoint. They aren't actually serialized.
		UClass *ObjClass = GetClass();
		UObject* LoadOuter = GetOuter();
		FName LoadName = GetFName();
    //处理完名字后交由FStructuredArchiveSlot处理Value，其中UClass类型由于是UObject子类，因此调用上方的UObject参数类型的重载
		Record << SA_VALUE(TEXT("LoadName"), LoadName);
		Record << SA_VALUE(TEXT("LoadOuter"), LoadOuter);
		Record << SA_VALUE(TEXT("ObjClass"), ObjClass);
}
```

而SA_VALUE的宏定义

```c++
#define SA_VALUE(Name, Value) MakeNamedValue(FArchiveFieldName(Name), Value)

template<typename T> FORCEINLINE TNamedValue<T> MakeNamedValue(FArchiveFieldName Name, T& Value)
{
	return TNamedValue<T>(Name, Value);
}
```

可以看到最终就是创建一个TNamedValue类型交由Record处理，Record处理完名字后将具体的Value交由Slot处理。

大多数Slot的opeartor<<只是转调用

```c++
	FORCEINLINE void FStructuredArchiveSlot::operator<< (double& Value)
	{
		Ar.Formatter.Serialize(Value);
	}
	FORCEINLINE void FStructuredArchiveSlot::operator<< (UObject*& Value)
	{
		Ar.Formatter.Serialize(Value);
	}
```

也有部分比较复杂：

```c++
void FStructuredArchiveSlot::operator<< (FLazyObjectPtr& Value)
{
	Ar.EnterSlot(*this);
	Ar.Formatter.Serialize(Value);
	Ar.LeaveSlot();
}
```

可以看到对于UObject是单纯的转调用的,而 FBinaryArchiveFormatter的Serialize如下：

```c++
inline void FBinaryArchiveFormatter::Serialize(UObject*& Value)
{
	Inner << Value;
}
```

Inner就是FArchive类型，而这个类型是`FObjectAndNameAsStringProxyArchive`类型，可以看到，此类型重写了<<对UObject类型对象的重载

```c++
struct FObjectAndNameAsStringProxyArchive : public FNameAsStringProxyArchive
{
	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InInnerArchive - The inner archive to proxy.
	 * @param bInLoadIfFindFails - Indicates whether to try and load a ref'd object if we don't find it
	 */
	FObjectAndNameAsStringProxyArchive(FArchive& InInnerArchive, bool bInLoadIfFindFails)
		:	FNameAsStringProxyArchive(InInnerArchive)
		,	bLoadIfFindFails(bInLoadIfFindFails)
	{ }

	/** If we fail to find an object during loading, try and load it. */
	bool bLoadIfFindFails;

	COREUOBJECT_API virtual FArchive& operator<<(UObject*& Obj) override;
	COREUOBJECT_API virtual FArchive& operator<<(FWeakObjectPtr& Obj) override;
	COREUOBJECT_API virtual FArchive& operator<<(FSoftObjectPtr& Value) override;
	COREUOBJECT_API virtual FArchive& operator<<(FSoftObjectPath& Value) override;
};

```

跳转到最后，还是由传进去的MemoryWriter进行具体操作的。如下：InnerArchive就是传进去的MemoryWriter：

```c++
FArchive& FObjectAndNameAsStringProxyArchive::operator<<(UObject*& Obj)
{
	if (IsLoading())
	{
		// load the path name to the object
		FString LoadedString;
		InnerArchive << LoadedString;
		// look up the object by fully qualified pathname
		Obj = FindObject<UObject>(nullptr, *LoadedString, false);
		// If we couldn't find it, and we want to load it, do that
		if(!Obj && bLoadIfFindFails)
		{
			Obj = LoadObject<UObject>(nullptr, *LoadedString);
		}
	}
	else
	{
		// save out the fully qualified object name
		FString SavedString(Obj->GetPathName());
		InnerArchive << SavedString;
	}
	return *this;
}
```

而`FMemoryWriter`正是自己实现了` Serialize(void* Data, int64 Num)`函数来达到之前说的实现自己功能的目的。可以看到保存UObject的一部就是将obj的名字保存。`		SaveGameObject->Serialize(Ar);`进行动作，最终调用的还是UObject内的`Serialize`函数。涉及到什么类型，就用什么类型的重载。

## 关于UObject的Serialize函数(ue4.25)

```c++
void UObject::Serialize(FStructuredArchive::FRecord Record)
{
	SCOPED_LOADTIMER(UObject_Serialize);

#if WITH_EDITOR
	bool bReportSoftObjectPathRedirects = false;
	{
		TGuardValue<bool*> GuardValue(
			GReportSoftObjectPathRedirects,
			  GReportSoftObjectPathRedirects
			? GReportSoftObjectPathRedirects
			: (GIsCookerLoadingPackage && HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
			? &bReportSoftObjectPathRedirects
			: nullptr
		);
#endif

		FArchive& UnderlyingArchive = Record.GetUnderlyingArchive();

		// These three items are very special items from a serialization standpoint. They aren't actually serialized.
		UClass *ObjClass = GetClass();
		UObject* LoadOuter = GetOuter();
		FName LoadName = GetFName();

		// Make sure this object's class's data is loaded.
		if(ObjClass->HasAnyFlags(RF_NeedLoad) )
		{
			UnderlyingArchive.Preload(ObjClass);

			// make sure this object's template data is loaded - the only objects
			// this should actually affect are those that don't have any defaults
			// to serialize.  for objects with defaults that actually require loading
			// the class default object should be serialized in FLinkerLoad::Preload, before
			// we've hit this code.
			if ( !HasAnyFlags(RF_ClassDefaultObject) && ObjClass->GetDefaultsCount() > 0 )
			{
				UnderlyingArchive.Preload(ObjClass->GetDefaultObject());
			}
		}

		// Special info.
		if ((!UnderlyingArchive.IsLoading() && !UnderlyingArchive.IsSaving() && !UnderlyingArchive.IsObjectReferenceCollector()))
		{
			Record << SA_VALUE(TEXT("LoadName"), LoadName);
			if (!UnderlyingArchive.IsIgnoringOuterRef())
			{
				Record << SA_VALUE(TEXT("LoadOuter"), LoadOuter);
			}
			if (!UnderlyingArchive.IsIgnoringClassRef())
			{
				Record << SA_VALUE(TEXT("ObjClass"), ObjClass);
			}
		}
		// Special support for supporting undo/redo of renaming and changing Archetype.
		else if (UnderlyingArchive.IsTransacting())
		{
			if (!UnderlyingArchive.IsIgnoringOuterRef())
			{
				if (UnderlyingArchive.IsLoading())
				{
					Record << SA_VALUE(TEXT("LoadName"), LoadName) << SA_VALUE(TEXT("LoadOuter"), LoadOuter);

					// If the name we loaded is different from the current one,
					// unhash the object, change the name and hash it again.
					bool bDifferentName = GetFName() != NAME_None && LoadName != GetFName();
					bool bDifferentOuter = LoadOuter != GetOuter();
					if ( bDifferentName == true || bDifferentOuter == true )
					{
						// Clear the name for use by this:
						UObject* Collision = StaticFindObjectFast(UObject::StaticClass(), LoadOuter, LoadName);
						if(Collision && Collision != this)
						{
							FName NewNameForCollision = MakeUniqueObjectName(LoadOuter, Collision->GetClass(), LoadName);
							checkf( StaticFindObjectFast(UObject::StaticClass(), LoadOuter, NewNameForCollision) == nullptr,
								TEXT("Failed to MakeUniqueObjectName for object colliding with transaction buffer state: %s %s"),
								*LoadName.ToString(),
								*NewNameForCollision.ToString()
							);
							Collision->LowLevelRename(NewNameForCollision,LoadOuter);
#if DO_CHECK
							UObject* SubsequentCollision = StaticFindObjectFast(UObject::StaticClass(), LoadOuter, LoadName);
							checkf( SubsequentCollision == nullptr,
								TEXT("Multiple name collisions detected in the transaction buffer: %x %x with name %s"),
								Collision,
								SubsequentCollision,
								*LoadName.ToString()
							);
#endif
						}
						
						LowLevelRename(LoadName,LoadOuter);
					}
				}
				else
				{
					Record << SA_VALUE(TEXT("LoadName"), LoadName) << SA_VALUE(TEXT("LoadOuter"), LoadOuter);
				}
			}
		}

		//序列化定义在class内部的属性
		//处理派生的UClass对象
		// Serialize object properties which are defined in the class.
		// Handle derived UClass objects (exact UClass objects are native only and shouldn't be touched)
		if (ObjClass != UClass::StaticClass())
		{
			SerializeScriptProperties(Record.EnterField(SA_FIELD_NAME(TEXT("Properties"))));
		}

		//跟踪将要被kill的
		// Keep track of pending kill
		if (UnderlyingArchive.IsTransacting())
		{
			bool WasKill = IsPendingKill();
			if (UnderlyingArchive.IsLoading())
			{
				Record << SA_VALUE(TEXT("WasKill"), WasKill);
				if (WasKill)
				{
					MarkPendingKill();
				}
				else
				{
					ClearPendingKill();
				}
			}
			else if (UnderlyingArchive.IsSaving())
			{
				Record << SA_VALUE(TEXT("WasKill"), WasKill);
			}
		}

		// Serialize a GUID if this object has one mapped to it
		FLazyObjectPtr::PossiblySerializeObjectGuid(this, Record);

		// Invalidate asset pointer caches when loading a new object
		if (UnderlyingArchive.IsLoading())
		{
			FSoftObjectPath::InvalidateTag();
		}

		// Memory counting (with proper alignment to match C++)
		SIZE_T Size = GetClass()->GetStructureSize();
		UnderlyingArchive.CountBytes(Size, Size);
#if WITH_EDITOR
	}

	if (bReportSoftObjectPathRedirects && !GReportSoftObjectPathRedirects)
	{
		UE_ASSET_LOG(LogCore, Warning, this, TEXT("Soft object paths were redirected during cook of '%s' - package should be resaved."), *GetName());
	}
#endif
}
```

反序列化过程：

![](https://github.com/whukxggx/ue4_doc/blob/master/UObject%E5%BA%8F%E5%88%97%E5%8C%96.png?raw=true)

1. 先获取类信息和Outer，

```c++
UClass *ObjClass = GetClass();
UObject* LoadOuter = GetOuter();
FName LoadName = GetFName();
```

2. 判断类class信息是否已经加载，如果没有则预载入类的信息和类默认对象CDO的信息：

```c++
UnderlyingArchive.Preload(ObjClass);
UnderlyingArchive.Preload(ObjClass->GetDefaultObject());
```

3. 加载名字,Outer,和当前对象的类信息

```c++
Record << SA_VALUE(TEXT("LoadName"), LoadName);
Record << SA_VALUE(TEXT("LoadOuter"), LoadOuter);
Record << SA_VALUE(TEXT("ObjClass"), ObjClass);
```

4. 序列化对象的属性信息:

```c++
if (ObjClass != UClass::StaticClass())
{
	SerializeScriptProperties(Record.EnterField(SA_FIELD_NAME(TEXT("Properties"))));
}
```

关于`SerializeScriptProperties`：

```c++
void UObject::SerializeScriptProperties( FStructuredArchive::FSlot Slot ) const
{
	FArchive& UnderlyingArchive = Slot.GetUnderlyingArchive();

	UnderlyingArchive.MarkScriptSerializationStart(this);
	//RF_ClassDefaultObject 判断这个class是不是它的默认obejct
	if( HasAnyFlags(RF_ClassDefaultObject) )
	{
		UnderlyingArchive.StartSerializingDefaults();
	}
    
	ObjClass->SerializeTaggedProperties(Slot, (uint8*)this, HasAnyFlags(RF_ClassDefaultObject) ? ObjClass->GetSuperClass() : ObjClass, (uint8*)DiffObject, bBreakSerializationRecursion ? this : nullptr);

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UnderlyingArchive.StopSerializingDefaults();
	}
	UnderlyingArchive.MarkScriptSerializationEnd(this);
}
```

![](https://github.com/whukxggx/ue4_doc/blob/master/SerializeScriptProperties.png?raw=true)

主要分为以下几个过程：

1. 开始序列化

`UnderlyingArchive.MarkScriptSerializationStart(this);`

2. 序列化对象属性， 并且加入tag

```c++
ObjClass->SerializeTaggedProperties(Slot, (uint8*)this, HasAnyFlags(RF_ClassDefaultObject) ? ObjClass->GetSuperClass() : ObjClass, (uint8*)DiffObject, bBreakSerializationRecursion ? this : nullptr);
```

3. 序列化结束

`UnderlyingArchive.MarkScriptSerializationEnd(this);`

主要看一下序列化对象属性时的操作函数:

```c++
void UStruct::SerializeTaggedProperties(FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad) const
{
	if (Slot.GetArchiveState().UseUnversionedPropertySerialization())
	{
		SerializeUnversionedProperties(this, Slot, Data, DefaultsStruct, Defaults);
	}
	else
	{
		SerializeVersionedTaggedProperties(Slot, Data, DefaultsStruct, Defaults, BreakRecursionIfFullyLoad);
	}
}
```

![](https://github.com/whukxggx/ue4_doc/blob/master/SerializeTaggedProperties.png?raw=true)

其中`SerializeUnversionedProperties`主要是序列化一些特定的struct的未版本化的属性，这里主要关注`SerializeVersionedTaggedProperties`函数。

```c++
void UStruct::SerializeVersionedTaggedProperties(FStructuredArchive::FSlot Slot, uint8* Data, UStruct* DefaultsStruct, uint8* Defaults, const UObject* BreakRecursionIfFullyLoad) const
{
  if (UnderlyingArchive.IsLoading())
  {
	if (UnderlyingArchive.IsTextFormat())
	{
		LoadTaggedPropertiesFromText(Slot, Data, DefaultsStruct, Defaults, BreakRecursionIfFullyLoad);
	}
	else
	{
    }
  }
  else
  {
  }
}
```

分为两个大部分，`IsLoading`（加载）部分和`IsSaving`（保存）部分。其中在`IsLoading`部分分为两部分，通过`UnderlyingArchive.IsTextFormat()`进行分离。如果是文本格式的话，直接调用`LoadTaggedPropertiesFromText`，如果不是则直接在else内进行属性的加载。

`LoadTaggedPropertiesFromText`主要做的事：序列化属性链表，使用属性标记处理不匹配的情况

![](https://github.com/whukxggx/ue4_doc/blob/master/LoadTaggedPropertiesFromText.png?raw=true)

如上，先获取属性数量`NumProperties`，然后按照属性索引从0开始循环`for (int32 PropertyIndex = 0; PropertyIndex < NumProperties; ++PropertyIndex)`:

1. 获取属性名：

```c++
FString PropertyNameString;
FStructuredArchiveSlot PropertySlot = PropertiesMap.EnterElement(PropertyNameString);
FName PropertyName = *PropertyNameString;
```

2. 如果此属性附加了guid，则在开始加载之前，需要将其解析为正确的名称。

```c++
PropertyGuidSlot.GetValue() << PropertyGuid;
FName NewName = FindPropertyNameFromGuid(PropertyGuid);
PropertyName = NewName;
```

3. 如果有重定向，也需要纠正名字

```c++
for (UStruct* CheckStruct = GetOwnerStruct(); CheckStruct; CheckStruct = CheckStruct->GetSuperStruct())
{
	FName NewTagName = FProperty::FindRedirectedPropertyName(CheckStruct, PropertyName);
	if (!NewTagName.IsNone())
	{
		PropertyName = NewTagName;
		break;
	}
}
```

4. 通过名字加载属性

`		FProperty* Property = FindPropertyByName(PropertyName);`

5. 获取属性ID和属性维度，如果维度大于1，用`FStructuredArchiveArray`进行保存

```c++
TOptional<FStructuredArchiveArray> SlotArray;
int32 NumAvailableItems = 0;
SlotArray.Emplace(PropertySlot.EnterArray(NumAvailableItems));
```

6. 按维度数进行循环，获取相应的`FPropertyTag`

```c++
FPropertyTag Tag;
ItemSlot.GetValue() << Tag;
Tag.ArrayIndex = ItemIndex;
Tag.Name = PropertyName;
```

7. 进行真正属性操作(上面都是为了获取正确的属性和tag所做的努力)

```c++
Tag.SerializeTaggedProperty(ItemSlot.GetValue(), Property, DestAddress, DefaultsFromParent);
```

可以看到此函数如下：

```c++
void FPropertyTag::SerializeTaggedProperty(FStructuredArchive::FSlot Slot, FProperty* Property, uint8* Value, uint8* Defaults) const
{
    	if (!UnderlyingArchive.IsTextFormat() && Property->GetClass() == FBoolProperty::StaticClass()){
            
        }else{
		//每种不同的property有不同的实现，基类FProperty声明为纯虚函数
		Property->SerializeItem(Slot, Value, Defaults);
        }
}
```

除了部分布尔类型特殊外，最终都会是属性调用`SerializeItem`函数进行最终操作，而此函数是每种不同的属性有不同实现的，基类的此函数为纯虚函数。

接`SerializeVersionedTaggedProperties`：如果不是文本格式就进入else内进行操作，虽然内容很多但主要方向是两个，找到正确的`FProperty`和`FPropertyTag`对象，并调用`FPropertyTag`对象的`void FPropertyTag::SerializeTaggedProperty(FStructuredArchive::FSlot Slot, FProperty* Property, uint8* Value, uint8* Defaults) const`（在调用此函数前进行了很多检测，确保property和tag的正确性）; 最终的处理交由具体属性来做(即`SerializeItem`)。