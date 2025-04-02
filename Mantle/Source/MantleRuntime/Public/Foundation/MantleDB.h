// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "Containers/AnankeUntypedArrayView.h"
#include "Containers/Array.h"
#include "Containers/BitArray.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "InstancedStruct.h"
#include "MantleSingleton.h"
#include "Templates/SharedPointer.h"

#include "MantleDB.generated.h"

struct FMantleComponentQuery;
struct FMantleIterator;
struct FMantleDBChunk;
struct FMantleDBEntry;
class TestSuite;
class UMantleDB;

namespace Ananke::Mantle
{
	// 128kb. Supposedly chosen based on CPU L1 cache size.
	constexpr int32 kDefaultChunkSize = 128*1024;

	// 1kb. This was chosen somewhat arbitrarily, but we need to have some type of floor for this so that there is room
	// for at least a few component structs per chunk. (1 struct per chunk pretty much makes this entire system
	// pointless).
	constexpr int32 kMinChunkSize = 1*1024;

	// Fire off a warning log if this number of chunks per entry is reached.
	constexpr int32 kChunkCountWarnThreshold = 80;

	constexpr int32 kInvalidIndex = -1;
	constexpr int32 kInvalidSize = -1;
	constexpr int32 kBareEntityChunkIndex = 0; // The first entry in the DB is reserved for bare entities.
}

USTRUCT()
struct FMantleEntity
{
	GENERATED_BODY()

public:
	FMantleEntity() = default;
	FMantleEntity(TBitArray<>& NewArchetype, FGuid& NewChunkId, int32 NewIndex)
	{
		Id = FGuid::NewGuid();
		Archetype = NewArchetype;
		ChunkId = NewChunkId;
		Index = NewIndex;
	}

	FGuid Id;
	TBitArray<> Archetype;
	FGuid ChunkId;
	int32 Index = -1;
};

USTRUCT()
struct FMantleComponentInfo
{
	GENERATED_BODY()

public:
	FString Name = TEXT("");
	int32 ArchetypeIndex = Ananke::Mantle::kInvalidIndex;

	int32 StructSize = Ananke::Mantle::kInvalidSize;
	int32 StructAlignment = Ananke::Mantle::kInvalidSize;

	// A temporary position within a chunk.
	uint8* ChunkLocation = nullptr;
};

USTRUCT()
struct FMantleDBVersion
{
	GENERATED_BODY()

public:
	void Update()
	{
		VersionNumber++;
		bIsValid = true;
	}

	void Invalidate()
	{
		bIsValid = false;
	}

	bool IsValid()
	{
		return bIsValid;
	}

	friend bool operator==(const FMantleDBVersion& lhs, const FMantleDBVersion& rhs)
	{
		return lhs.VersionNumber == rhs.VersionNumber;
	}
	friend bool operator!=(const FMantleDBVersion& lhs, const FMantleDBVersion& rhs)
	{
		return lhs.VersionNumber != rhs.VersionNumber;
	}

private:
	// Max value is 4,294,967,295. We wrap this value in a struct so we can change it later if necessary.
	uint32 VersionNumber = 0;

	bool bIsValid = false;
};

USTRUCT()
struct FMantleCachedEntry
{
	GENERATED_BODY()

public:
	FMantleCachedEntry() = default;
	
	FMantleCachedEntry(TBitArray<>& NewArchetype)
	{
		Archetype = NewArchetype;
	}
	
	int32 NumChunks()
	{
		return ChunkedEntityIds.Num();
	}

	TBitArray<> Archetype;
	
	// [type] -> [chunk][entityComponent]
	TMap<FString, TArray<FAnankeUntypedArrayView>> ChunkedComponents;
	
	// [chunk][entity]
	TArray<TArrayView<FGuid>> ChunkedEntityIds;
	
	TSet<TBitArray<>> MatchingQueries;
	bool bIsValid = false;
};

USTRUCT()
struct FMantleCachedQuery
{
	GENERATED_BODY()

public:
	FMantleCachedQuery() = default;
	
	FMantleCachedQuery(TBitArray<> NewArchetype)
	{
		QueryArchetype = NewArchetype;
	}

	void ClearData()
	{
		MatchingEntries.Empty();
	}
	
	TBitArray<> QueryArchetype;
	TArray<FMantleCachedEntry> MatchingEntries;
	FMantleDBVersion Version;
};

// Information that should be provided to subcomponents of the DB.
USTRUCT()
struct FMantleDBMasterRecord
{
	GENERATED_BODY()

public:
	FGuid RegisterEntity(TBitArray<>& Archetype, FGuid& ChunkId, int32 ChunkIndex)
	{
		FMantleEntity NewEntity = FMantleEntity(Archetype, ChunkId, ChunkIndex);
		EntitiesById.Add(NewEntity.Id, NewEntity);
		return NewEntity.Id;
	}

	void RemoveEntity(FGuid& EntityId)
	{
		if (EntitiesById.Contains(EntityId))
		{
			EntitiesById.Remove(EntityId);
		}
	}

	FMantleCachedEntry& FindOrAddCachedEntry(TBitArray<>& Archetype)
	{
		FMantleCachedEntry* CachedEntry = CachedEntries.Find(Archetype);
		if (!CachedEntry)
		{
			return CachedEntries.Add(Archetype, FMantleCachedEntry(Archetype));
		}

		return *CachedEntry;
	}

	bool ArchetypeHasComponent(TBitArray<>& Archetype, TObjectPtr<UScriptStruct> ComponentType)
	{
		if (!ComponentType)
		{
			return false;
		}
	
		FMantleComponentInfo* ComponentInfo = ComponentInfoMap.Find(ComponentType->GetName());
		if (!ComponentInfo)
		{
			return false;
		}

		return Archetype[ComponentInfo->ArchetypeIndex];	
	}

	// The number of bytes to allocate for each chunk.
	int32 ChunkComponentBlobSize = 0;
	
	TMap<FString, FMantleComponentInfo> ComponentInfoMap;
	TMap<FGuid, FMantleEntity> EntitiesById;

	// Query Caching ---------
	TMap<TBitArray<>, FMantleCachedQuery> CachedQueries;
	TMap<TBitArray<>, FMantleCachedEntry> CachedEntries;

	// Scenario 1: A new archetype is added:
	//   - Create a new 'dirty' cached entry for that archetype
	//   - Loop through all known queries and add them to the match list
	//   - Loop through all known matching queries & add an empty cached entry
	//   - Mark each of those queries as 'dirty' as well

	// Scenario 2: An archetype is modified
	//   - Mark the cached entry for that archetype as 'dirty'
	//   - Mark each matching query as 'dirty' as well

	// Scenario 3: An archetype is removed or emptied:
	//  - Look through all known matching queries and remove the cached entry for that archetype.

	// When query is run:
	//   If not 'dirty' then just return a copy of the query result
	//   Otherwise, update the 'dirty' archetypes/entries.
};

struct FMantleDBChunk
{
public:
	explicit FMantleDBChunk(FGuid& NewChunkId, TBitArray<>& NewArchetype, FMantleDBEntry* NewEntry, FMantleDBMasterRecord* NewMasterRecord);
	~FMantleDBChunk();
	
	int32 GetRemainingCapacity()
	{
		return FMath::Max(TotalCapacity - EntityIds.Num(), 0);
	}

	int32 IsEmpty()
	{
		return EntityIds.Num() == 0;
	}

	int32 AddEntities(
		const TArray<FInstancedStruct>& ComponentsToAdd,
		const int32 NumEntities,
		FMantleCachedEntry& OutResult
	);

	void RemoveEntity(FMantleEntity& Entity, bool bEntityWasMoved=false);

	int32 TakeEntities(
		TArrayView<FGuid>& IdsToTake,
		FMantleDBEntry& TakeFrom,
		TArray<FInstancedStruct>& ComponentsToAdd,
		FMantleCachedEntry& OutResult
	);

	void* GetComponent(FString TypeName, FMantleEntity& Entity)
	{
		return GetComponentInternal(TypeName, Entity.Index);
	}
	
private:
	friend UMantleDB;
	friend FMantleDBEntry;
	friend TestSuite;

	bool MaybeAllocateBlob();
	
	void DeallocateBlob()
	{
		if (ComponentBlob != nullptr)
		{
			FMemory::Free(ComponentBlob);
			ComponentBlob = nullptr;
			MaxLocation = nullptr;
		}
	}
	
	bool ValidateComponentInfo(FMantleComponentInfo& Info)
	{
		return (
			!Info.Name.Equals(TEXT("")) &&
			Info.StructSize > 0 &&
			Info.StructAlignment > 0
		);
	}

	bool LocationIsValid(void* Location)
	{
		if (Location == nullptr || ComponentBlob == nullptr || MaxLocation == nullptr)
		{
			return false;
		}
		
		return Location >= ComponentBlob && Location <= MaxLocation;
	}

	void RegisterEntities(int32 NumEntities, FMantleCachedEntry& OutResult);

	void* GetComponentInternal(FString& TypeName, int32 EntityIndex)
	{
		FMantleComponentInfo* TypeInfo = ComponentTypeInfo.Find(TypeName);

		if (!TypeInfo)
		{
			return nullptr;
		}

		return TypeInfo->ChunkLocation + (EntityIndex * TypeInfo->StructSize);
	}

	int32 TakeBareArchetypeEntities(
		TArrayView<FGuid>& IdsToTake,
		FMantleDBEntry& TakeFrom,
		FMantleCachedEntry& OutResult
	);
	
	// Pointer to raw memory. The uint8 type is used here just to allow for advancing the pointer by some number of bytes.
	uint8* ComponentBlob = nullptr;
	uint8* MaxLocation = nullptr;

	TBitArray<> Archetype;
	
	FGuid ChunkId;

	// The total number of entities supported by this chunk.
	int32 TotalCapacity = 0;

	// The actual entities reserved for this chunk.
	TArray<FGuid> EntityIds;

	FMantleDBEntry* Entry = nullptr;
	FMantleDBMasterRecord* MasterRecord = nullptr;

	// Specifies where in the ComponentBlob to look for a particular component type.
	TMap<FString, FMantleComponentInfo> ComponentTypeInfo;
};

struct FMantleDBEntry
{
public:
	FMantleDBEntry(TBitArray<>& NewArchetype, FMantleDBMasterRecord& NewMasterRecord);
	
	void AddEntities(const TArray<FInstancedStruct>& ComponentsToAdd, const int32 NumEntities, FMantleCachedEntry& OutResult);
	void TakeEntities(TArray<FGuid>& EntityIds, FMantleDBEntry& TakeFrom, TArray<FInstancedStruct>& ComponentsToAdd, FMantleCachedEntry& OutResult);

	FMantleDBChunk& GetAvailableChunk();
	void MakeAvailable(FGuid& ChunkId);
	
	FMantleDBMasterRecord* MasterRecord = nullptr;

	TBitArray<> Archetype;
	TArray<FString> ComponentTypes;
	TMap<FGuid, FMantleDBChunk> Chunks;
	TArray<FGuid> AvailableChunkIds;
	TArray<FGuid> AllChunkIds;
};

UCLASS()
class MANTLERUNTIME_API UMantleDB : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(TArray<UScriptStruct*>& ComponentTypes, int32 ChunkSizeBytes = Ananke::Mantle::kDefaultChunkSize);

	// ENTITY ADD
	FMantleIterator AddEntityAndIterate(const TArray<FInstancedStruct>& InitialComposition);
	FGuid AddEntity(const TArray<FInstancedStruct>& InitialComposition);
	FMantleIterator AddEntities(const TArray<FInstancedStruct>& InitialComposition, const int32 NumEntities);

	// ENTITY REMOVE
	void RemoveEntity(const FGuid EntityId)
	{
		TArray<FGuid> EntityIds;
		EntityIds.Add(EntityId);
		RemoveEntities(EntityIds);
	}
	void RemoveEntities(const TArray<FGuid>& EntityIds);

	// ENTITY UPDATE
	FMantleIterator UpdateEntity(FGuid& EntityId, TArray<FInstancedStruct>& ComponentsToAdd, TArray<UScriptStruct*>& ComponentsToRemove);
	FMantleIterator UpdateEntity(FGuid& EntityId, TArray<FInstancedStruct>& ComponentsToAdd);
	FMantleIterator UpdateEntity(FGuid& EntityId, TArray<UScriptStruct*>& ComponentsToRemove);

	FMantleIterator UpdateEntities(TArray<FGuid>& EntityIds, TArray<FInstancedStruct>& ComponentsToAdd, TArray<UScriptStruct*>& ComponentsToRemove);
	FMantleIterator UpdateEntities(TArray<FGuid>& EntityIds, TArray<FInstancedStruct>& ComponentsToAdd);
	FMantleIterator UpdateEntities(TArray<FGuid>& EntityIds, TArray<UScriptStruct*>& ComponentsToRemove);

	// ENTITY FETCH
	FMantleIterator RunQuery(FMantleComponentQuery& Query);

	template<typename TComponentType>
	TComponentType* GetComponent(FGuid EntityId)
	{
		FMantleEntity* Entity = MasterRecord.EntitiesById.Find(EntityId);
		if (!Entity)
		{
			return nullptr;
		}
		
		if (!MasterRecord.ArchetypeHasComponent(Entity->Archetype, TComponentType::StaticStruct()))
		{
			return nullptr;
		}
		
		FMantleDBChunk* Chunk = GetChunk(Entity->Archetype, Entity->ChunkId);
		if (!Chunk)
		{
			return nullptr;
		}

		UScriptStruct* ComponentStruct = TComponentType::StaticStruct();
		return (TComponentType*)(Chunk->GetComponent(ComponentStruct->GetName(), *Entity));
	}

	// ENTITY UTIL
	bool HasEntity(FGuid EntityId)
	{
		return EntityId.IsValid() && MasterRecord.EntitiesById.Contains(EntityId);
	}

	template<typename ComponentType>
	bool HasComponent(FGuid EntityId)
	{
		FMantleEntity* Entity = MasterRecord.EntitiesById.Find(EntityId);
		if (!Entity)
		{
			return false;
		}
		return MasterRecord.ArchetypeHasComponent(Entity->Archetype, ComponentType::StaticStruct());
	}

	// SINGLETONS
	// TODO(): Add back when there is an actual use-case for this.
	/*
	template<typename SingletonType>
	TObjectPtr<SingletonType> GetSingleton()
	{
		check(SingletonType::StaticClass()->IsChildOf(UMantleSingleton::StaticClass()));

		FString TypeString = SingletonType::StaticClass()->GetName();

		if (Singletons.Contains(TypeString))
		{
			return Cast<SingletonType>(Singletons.Find(TypeString)->Get());
		}

		SingletonType* NewSingleton = TObjectPtr<SingletonType>(NewObject<SingletonType>(this));
		NewSingleton.SetDB(this);
		Singletons.Add(NewSingleton);
		return Cast<SingletonType>(NewSingleton);
	}
	*/

protected:
	friend TestSuite;
	
	void FillArchetype(TBitArray<>& Archetype, TArray<FString>* ToAdd, TArray<FString>* ToRemove = nullptr);
	TSharedPtr<FMantleDBEntry> GetEntry(TBitArray<>& Archetype);
	TSharedPtr<FMantleDBEntry> GetOrCreateEntry(TBitArray<>& Archetype);

	FMantleDBChunk* GetChunk(TBitArray<> Archetype, FGuid ChunkId)
	{
		TSharedPtr<FMantleDBEntry>* Entry = EntriesByArchetype.Find(Archetype);

		if (!Entry || !Entry->IsValid())
		{
			return nullptr;
		}

		return Entry->Get()->Chunks.Find(ChunkId);
	}

	FMantleIterator RunQueryInternal(TBitArray<>& QueryArchetype);
	bool RefreshCachedEntry(FMantleCachedEntry& Entry);
	void EntryWasModified(TBitArray<>& EntryArchetype);
	bool RefreshCachedQuery(TBitArray<>& Archetype);
	
	TMap<TBitArray<>, TSharedPtr<FMantleDBEntry>> EntriesByArchetype;
	TArray<TBitArray<>> ActiveArchetypes; // Allows us to iterate through EnteriesByArchetype in a deterministic way (for testing).
	FMantleDBMasterRecord MasterRecord;

	// TODO(): Add back when there is an actual use-case for this.
	// UPROPERTY()
	// TMap<FString, TObjectPtr<UMantleSingleton>> Singletons;

	bool bIsInitialized = false;
};