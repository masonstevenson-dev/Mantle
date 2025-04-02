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

#include "Foundation/MantleDB.h"
#include "Foundation/MantleQueries.h"
#include "Containers/AnankeUntypedArrayView.h"

#include "MantleRuntimeLoggingDefs.h"
#include "FunctionLibraries/AnankeBitArrayLibrary.h"
#include "MantleComponents/MC_TemporaryEntity.h"

// FMantleDBChunk -------------------------------------------------------------------------------------------------------
FMantleDBChunk::FMantleDBChunk(
	FGuid& NewChunkId, TBitArray<>& NewArchetype, FMantleDBEntry* NewEntry, FMantleDBMasterRecord* NewMasterRecord)
{
	ChunkId = NewChunkId;
	Archetype = NewArchetype;
	Entry = NewEntry;
	MasterRecord = NewMasterRecord;
	
	int32 BytesPerEntity = 0;
	int32 MaxPossibleAlignmentPadding = 0;

	// First, compute sizes
	for (auto Iterator = MasterRecord->ComponentInfoMap.CreateConstIterator(); Iterator; ++Iterator)
	{
		FMantleComponentInfo ComponentInfo = Iterator.Value();
		if (!ValidateComponentInfo(ComponentInfo))
		{
			UE_LOG(LogMantle, Warning, TEXT("Found invalid component (name: %s) while initializing DB chunk. Skipping this component."), *ComponentInfo.Name);
			continue;
		}
		if (!Archetype[ComponentInfo.ArchetypeIndex])
		{
			continue;
		}

		BytesPerEntity += ComponentInfo.StructSize;
		MaxPossibleAlignmentPadding += ComponentInfo.StructAlignment;
		ComponentTypeInfo.Add(Iterator.Key(), ComponentInfo);
	}

	if (BytesPerEntity > 0)
	{
		TotalCapacity = (MasterRecord->ChunkComponentBlobSize - MaxPossibleAlignmentPadding) / BytesPerEntity;
		if (TotalCapacity <= 0)
		{
			UE_LOG(LogMantle, Fatal, TEXT("MantleDB does not support entity size."));
			return;
		}
	}
}

FMantleDBChunk::~FMantleDBChunk()
{
	DeallocateBlob();

	if (EntityIds.Num() > 0)
	{
		DEC_DWORD_STAT_BY(STAT_Mantle_EntityCount, EntityIds.Num());
		EntityIds.Empty();
	}
}

int32 FMantleDBChunk::AddEntities(
	const TArray<FInstancedStruct>& ComponentsToAdd,
	const int32 NumEntities,
	FMantleCachedEntry& OutResult
)
{
	if (NumEntities <= 0)
	{
		return 0;
	}

	if (UAnankeBitArrayLibrary::IsZero(Archetype))
	{
		RegisterEntities(NumEntities, OutResult);
		return NumEntities;
	}

	if (!MaybeAllocateBlob())
	{
		return 0;
	}

	const int32 NumExistingEntities = EntityIds.Num();
	const int32 NumEntitiesToAdd = FMath::Min(NumEntities, GetRemainingCapacity());
	RegisterEntities(NumEntitiesToAdd, OutResult);
	
	for (FInstancedStruct ComponentInstance : ComponentsToAdd)
	{
		FString TypeName = ComponentInstance.GetScriptStruct()->GetName();
		FMantleComponentInfo* ComponentInfo = ComponentTypeInfo.Find(TypeName);

		if (!ComponentInfo || !ComponentInfo->ChunkLocation)
		{
			// todo(): Rollback changes and continue.
			UE_LOG(LogMantle, Fatal,
			       TEXT("AddEntities: ComponentInfo for type %s is invalid. "
				       "(Double check that this component inherits from FMantleComponent.)"), *TypeName);
			return 0;
		}

		uint8* StartingLocation = ComponentInfo->ChunkLocation + (NumExistingEntities * ComponentInfo->StructSize);
		uint8* DestLocation = StartingLocation;
		const uint8* SrcLocation = ComponentInstance.GetMemory();
		
		for (int EntityIndex = 0; EntityIndex < NumEntitiesToAdd; EntityIndex++)
		{
			if (!LocationIsValid(DestLocation))
			{
				UE_LOG(LogMantle, Fatal, TEXT("Attempted to copy to memory address outside of chunk range."));
				return 0;
			}

			// Certain UE types (for ex. FString) will throw runtime errors if you try to copy them into a memory
			// address that has not been initialized, so we have to call InitializeStruct even though we are going to
			// immediately overwrite the data with the instance data.
			ComponentInstance.GetScriptStruct()->InitializeStruct(DestLocation);
			ComponentInstance.GetScriptStruct()->CopyScriptStruct(DestLocation, SrcLocation); // TODO(): make optional.
			
			DestLocation += ComponentInfo->StructSize;
		}

		OutResult.ChunkedComponents.FindOrAdd(TypeName).Add(FAnankeUntypedArrayView(StartingLocation, NumEntitiesToAdd));
	}
	
	return NumEntitiesToAdd;
}

void FMantleDBChunk::RemoveEntity(FMantleEntity& ToRemove, bool bEntityWasMoved)
{
	if (ToRemove.Index < 0 || ToRemove.Index >= EntityIds.Num())
	{
		UE_LOG(LogMantle, Error, TEXT("Entity %s has invalid index"), *ToRemove.Id.ToString());
		return;
	}

	bool bWasFull = (GetRemainingCapacity() == 0);

	int32 SwapIndex = ToRemove.Index;
	int32 LastEntityIndex = EntityIds.Num() - 1;

	EntityIds[SwapIndex] = EntityIds[LastEntityIndex];
	EntityIds.Pop();

	if (!bEntityWasMoved)
	{
		DEC_DWORD_STAT(STAT_Mantle_EntityCount);

		if (MasterRecord->ArchetypeHasComponent(ToRemove.Archetype, FMC_TemporaryEntity::StaticStruct()))
		{
			INC_DWORD_STAT(STAT_Mantle_TempararyEntitiesRemoved);
		}
	}
	
	if (SwapIndex == LastEntityIndex)
	{
		if (LastEntityIndex > 0)
		{
			return;
		}
		
		// TODO(): Consider just removing this entry entirely.
		DeallocateBlob();

		// Edge case where there was only 1 entity in this chunk.
		if (bWasFull)
		{
			Entry->MakeAvailable(ChunkId);
		}
		
		return;
	}

	// Swap data.
	FMantleEntity* SwapEntity = MasterRecord->EntitiesById.Find(EntityIds[SwapIndex]);
	
	for (auto Iterator = ComponentTypeInfo.CreateConstIterator(); Iterator; ++Iterator)
	{
		FMantleComponentInfo* TypeInfo = ComponentTypeInfo.Find(Iterator.Key());
		uint8* OldLoc = TypeInfo->ChunkLocation + (LastEntityIndex * TypeInfo->StructSize);
		uint8* SwapLoc = TypeInfo->ChunkLocation + (SwapIndex * TypeInfo->StructSize);

		if (SwapLoc < ComponentBlob || SwapLoc > MaxLocation)
		{
			UE_LOG(LogMantle, Fatal, TEXT("Attempted to copy to memory address outside of chunk range."));
			return;
		}
		// todo(): Make sure subchunk bounds are valid as well.

		FMemory::Memcpy(SwapLoc, OldLoc, TypeInfo->StructSize);
	}

	SwapEntity->Index = SwapIndex;

	if (bWasFull && GetRemainingCapacity() > 0)
	{
		Entry->MakeAvailable(ChunkId);
	}
}

int32 FMantleDBChunk::TakeEntities(
	TArrayView<FGuid>& IdsToTake,
	FMantleDBEntry& TakeFrom,
	TArray<FInstancedStruct>& ComponentsToAdd,
	FMantleCachedEntry& OutResult
)
{
	if (IdsToTake.Num() == 0)
	{
		UE_LOG(LogMantle, Warning, TEXT("Attempted to take 0 entities."));
		return 0;
	}

	if (UAnankeBitArrayLibrary::IsZero(Archetype))
	{
		return TakeBareArchetypeEntities(IdsToTake, TakeFrom, OutResult);
	}

	if (!MaybeAllocateBlob())
	{
		return 0;
	}
	
	const int32 OldEntityCount = EntityIds.Num();
	const int32 ResultChunkIndex = OutResult.ChunkedEntityIds.Num();
	int32 EntitiesSkipped = 0;
	
	for (int IdIndex = 0; IdIndex < IdsToTake.Num() && GetRemainingCapacity() > 0; ++IdIndex)
	{
		FMantleEntity* Entity = MasterRecord->EntitiesById.Find(IdsToTake[IdIndex]);

		if (!Entity)
		{
			UE_LOG(LogMantle, Error, TEXT("TakeEntities: Cannot find Entity."));
			EntitiesSkipped++;
			continue;
		}

		FMantleDBChunk* OldChunk = TakeFrom.Chunks.Find(Entity->ChunkId);
		if (!OldChunk)
		{
			UE_LOG(LogMantle, Error, TEXT("TakeEntities: Cannot find OldChunk."));
			EntitiesSkipped++;
			continue;
		}

		TSet<FString> TypesUpdated;
		const int32 NewEntityIndex = EntityIds.Num();
		
		for (auto OldTypeInfo = OldChunk->ComponentTypeInfo.CreateConstIterator(); OldTypeInfo; ++OldTypeInfo)
		{
			FString TypeName = OldTypeInfo.Key();
			FMantleComponentInfo* LocalComponentInfo = ComponentTypeInfo.Find(TypeName);
			
			if (!LocalComponentInfo)
			{
				continue;
			}

			const int32 StructSize = OldTypeInfo.Value().StructSize;
			if (StructSize != LocalComponentInfo->StructSize)
			{
				// TODO(): rollback and continue.
				UE_LOG(LogMantle, Fatal, TEXT("TakeEntities: Struct size mismatch."));
				return 0;
			}

			uint8* DestLocation = LocalComponentInfo->ChunkLocation + (NewEntityIndex * StructSize);
			uint8* SrcLocation = OldTypeInfo.Value().ChunkLocation + (Entity->Index * StructSize);

			if (!LocationIsValid(DestLocation))
			{
				// TODO(): rollback and continue.
				UE_LOG(LogMantle, Fatal, TEXT("TakeEntities: New location is invalid."));
				return 0;
			}
			if (!OldChunk->LocationIsValid(SrcLocation))
			{
				// TODO(): rollback and continue.
				UE_LOG(LogMantle, Fatal, TEXT("TakeEntities: OldChunk location is invalid."));
				return 0;
			}

			FMemory::Memcpy(DestLocation, SrcLocation, StructSize);
			TypesUpdated.Add(TypeName);

			// If a result for this type has not yet been recorded.
			if (OutResult.ChunkedComponents.FindOrAdd(TypeName).Num() == ResultChunkIndex)
			{
				// We aren't sure yet how many entities will be added, so we set the size to 0 and update it later.
				OutResult.ChunkedComponents.FindOrAdd(TypeName).Add(FAnankeUntypedArrayView(DestLocation, 0));
			}
		}
		for (FInstancedStruct ComponentInstance : ComponentsToAdd)
		{
			FString TypeName = ComponentInstance.GetScriptStruct()->GetName();
			FMantleComponentInfo* ComponentInfo = ComponentTypeInfo.Find(TypeName);

			if (!ComponentInfo || !ComponentInfo->ChunkLocation)
			{
				// todo(): Rollback changes and continue.
				UE_LOG(LogMantle, Fatal, TEXT("AddEntities: ComponentInfo for type %s is invalid."), *ComponentInfo->Name);
				return 0;
			}
			
			uint8* DestLocation = ComponentInfo->ChunkLocation + (NewEntityIndex * ComponentInfo->StructSize);
			const uint8* SrcLocation = ComponentInstance.GetMemory();
		
			if (!LocationIsValid(DestLocation))
			{
				// todo(): Rollback changes and continue.
				UE_LOG(LogMantle, Fatal, TEXT("Attempted to copy to memory address outside of chunk range."));
				return 0;
			}
			
			ComponentInstance.GetScriptStruct()->InitializeStruct(DestLocation);
			ComponentInstance.GetScriptStruct()->CopyScriptStruct(DestLocation, SrcLocation);
			TypesUpdated.Add(TypeName);

			// If a result for this type has not yet been recorded.
			if (OutResult.ChunkedComponents.FindOrAdd(TypeName).Num() == ResultChunkIndex)
			{
				// We aren't sure yet how many entities will be added, so we set the size to 0 and update it later.
				OutResult.ChunkedComponents.FindOrAdd(TypeName).Add(FAnankeUntypedArrayView(DestLocation, 0));
			}
		}

		// Sanity check. All types should have been updated.
		for (auto TypeIterator = ComponentTypeInfo.CreateConstIterator(); TypeIterator; ++TypeIterator)
		{
			FString ExpectedType = TypeIterator.Key();
			
			if (!TypesUpdated.Contains(ExpectedType))
			{
				// TODO(): Add support for initializing all types, even if they are missing. Then we can log an error
				//         here instead of fatal.
				UE_LOG(LogMantle, Fatal, TEXT("Expected type: %s to be updated."), *TypeIterator.Key());
				return 0;
			}
		}

		OldChunk->RemoveEntity(*Entity, true);

		Entity->Archetype = Archetype;
		Entity->ChunkId = ChunkId;
		Entity->Index = NewEntityIndex;
		EntityIds.Add(Entity->Id);
	}

	int32 EntitiesAdded = EntityIds.Num() - OldEntityCount;

	if (EntitiesAdded <= 0)
	{
		return 0;
	}
	
	OutResult.ChunkedEntityIds.Add(TArrayView<FGuid>(&EntityIds[OldEntityCount], EntitiesAdded));

	// Update the entity sizes on all the result array views.
	for (auto ResultIterator = OutResult.ChunkedComponents.CreateIterator(); ResultIterator; ++ResultIterator)
	{
		if (ResultChunkIndex >= ResultIterator.Value().Num())
		{
			UE_LOG(LogMantle, Error, TEXT("Expected result iterator for type %s to have a value at index %d"),
			       *ResultIterator.Key(), ResultChunkIndex);
			continue;
		}

		ResultIterator.Value()[ResultChunkIndex].SetSize(EntitiesAdded);
	}

	return EntitiesAdded + EntitiesSkipped;
}

bool FMantleDBChunk::MaybeAllocateBlob()
{
	if (!ComponentBlob)
	{
		ComponentBlob = (uint8*)FMemory::Malloc(MasterRecord->ChunkComponentBlobSize);
		uint8* NextSubchunkLocation = ComponentBlob;
		MaxLocation = ComponentBlob + MasterRecord->ChunkComponentBlobSize;

		// Compute positions for each subchunk.
		for (auto Iterator = ComponentTypeInfo.CreateIterator(); Iterator; ++Iterator)
		{
			FMantleComponentInfo& ComponentInfo = Iterator.Value();

			// Align() adds a bit of padding so that the struct starts at a safe address.
			ComponentInfo.ChunkLocation = Align(NextSubchunkLocation, ComponentInfo.StructAlignment);
			NextSubchunkLocation = ComponentInfo.ChunkLocation + (ComponentInfo.StructSize * TotalCapacity);

			if (NextSubchunkLocation > MaxLocation)
			{
				UE_LOG(LogMantle, Fatal, TEXT("Subchunk location %p exceeds max location %p."), NextSubchunkLocation, MaxLocation);
				return false;
			}
		}
	}

	return true;
}

void FMantleDBChunk::RegisterEntities(int32 NumEntities, FMantleCachedEntry& OutResult)
{
	const int32 EntityIndexOffset = EntityIds.Num();
	const int32 StartIndex = EntityIds.Num();
	
	for (int EntityIndex = 0; EntityIndex < NumEntities; EntityIndex++)
	{
		int32 NewEntityIndex = EntityIndexOffset + EntityIndex;
		FGuid NewEntityId = MasterRecord->RegisterEntity(Archetype, ChunkId, NewEntityIndex);
		EntityIds.Add(NewEntityId);
		INC_DWORD_STAT(STAT_Mantle_EntityCount);

		if (MasterRecord->ArchetypeHasComponent(Archetype, FMC_TemporaryEntity::StaticStruct()))
		{
			INC_DWORD_STAT(STAT_Mantle_TempararyEntitiesAdded);
		}
	}

	const int32 EntitiesAdded = EntityIds.Num() - StartIndex;

	if (EntitiesAdded <= 0)
	{
		OutResult.ChunkedEntityIds.Add(TArrayView<FGuid>());
		return;
	}

	OutResult.ChunkedEntityIds.Add(TArrayView<FGuid>(&EntityIds[StartIndex], EntitiesAdded));
}

int32 FMantleDBChunk::TakeBareArchetypeEntities (
	TArrayView<FGuid>& IdsToTake,
	FMantleDBEntry& TakeFrom,
	FMantleCachedEntry& OutResult
)
{
	const int32 StartIndex = EntityIds.Num();
	int32 EntitiesSkipped = 0;
	
	for (const FGuid EntityId : IdsToTake)
	{
		FMantleEntity* Entity = MasterRecord->EntitiesById.Find(EntityId);
		if (!Entity)
		{
			UE_LOG(LogMantle, Error, TEXT("TakeBareArchetypeEntities: Unable to find entity with id %s"), *EntityId.ToString());
			EntitiesSkipped++;
			continue;
		}

		FMantleDBChunk* TakeFromChunk = TakeFrom.Chunks.Find(Entity->ChunkId);
		if (!TakeFromChunk)
		{
			UE_LOG(LogMantle, Error, TEXT("TakeBareArchetypeEntities: Unable to find chunk for entity with id %s"), *EntityId.ToString());
			EntitiesSkipped++;
			continue;
		}

		EntityIds.Add(EntityId);
		TakeFromChunk->RemoveEntity(*Entity, true);
	}

	const int32 EntitiesAdded = EntityIds.Num() - StartIndex;

	if (EntitiesAdded <= 0)
	{
		OutResult.ChunkedEntityIds.Add(TArrayView<FGuid>());
		return 0;
	}

	OutResult.ChunkedEntityIds.Add(TArrayView<FGuid>(&EntityIds[StartIndex], EntitiesAdded));
	return EntitiesAdded + EntitiesSkipped;
}
// End FMantleDBChunk ---------------------------------------------------------------------------------------------------

// FMantleDBEntry -----------------------------------------------------------------------------------------------------
FMantleDBEntry::FMantleDBEntry(TBitArray<>& NewArchetype, FMantleDBMasterRecord& NewMasterRecord)
{
	Archetype = NewArchetype;
	MasterRecord = &NewMasterRecord;

	for (auto Iterator = MasterRecord->ComponentInfoMap.CreateConstIterator(); Iterator; ++Iterator)
	{
		FMantleComponentInfo ComponentInfo = Iterator.Value();
			
		if (!Archetype[ComponentInfo.ArchetypeIndex])
		{
			continue;
		}

		ComponentTypes.Add(ComponentInfo.Name);
	}
}

void FMantleDBEntry::AddEntities(const TArray<FInstancedStruct>& ComponentsToAdd, const int32 NumEntities, FMantleCachedEntry& OutResult)
{
	int32 PendingAllocations = NumEntities;

	while (PendingAllocations > 0)
	{
		FMantleDBChunk& CurrentChunk = GetAvailableChunk();
		PendingAllocations -= CurrentChunk.AddEntities(ComponentsToAdd, PendingAllocations, OutResult);

		// The 'bare' Archetype is always available since it does not store any component data.
		if (CurrentChunk.GetRemainingCapacity() <= 0 && !UAnankeBitArrayLibrary::IsZero(Archetype))
		{
			// Note, a queue might be faster here, but this probably isn't going to be called often enough to matter.
			AvailableChunkIds.Pop();
		}
	}
}

void FMantleDBEntry::TakeEntities(
	TArray<FGuid>& EntityIds,
	FMantleDBEntry& TakeFrom,
	TArray<FInstancedStruct>& ComponentsToAdd,
	FMantleCachedEntry& OutResult
)
{
	int32 EntitiesToTake = EntityIds.Num();

	while (EntitiesToTake > 0)
	{
		FMantleDBChunk& CurrentChunk = GetAvailableChunk();

		int32 IdIndex = EntityIds.Num() - EntitiesToTake;
		if (IdIndex < 0 || IdIndex >= EntityIds.Num())
		{
			UE_LOG(LogMantle, Fatal, TEXT("TakeEntities: IdIndex is invalid."));
			return;
		}

		auto RemainingIds = TArrayView<FGuid>(&EntityIds[IdIndex], EntitiesToTake);
		EntitiesToTake -= CurrentChunk.TakeEntities(RemainingIds, TakeFrom, ComponentsToAdd, OutResult);

		// The 'bare' Archetype is always available since it does not store any component data.
		if (CurrentChunk.GetRemainingCapacity() <= 0 && !UAnankeBitArrayLibrary::IsZero(Archetype))
		{
			// Note, a queue might be faster here, but this probably isn't going to be called often enough to matter.
			AvailableChunkIds.Pop();
		}
	}
}

FMantleDBChunk& FMantleDBEntry::GetAvailableChunk()
{
	FMantleDBChunk* ExistingChunk = nullptr;

	// This shouldn't ever loop more than once, but his should prevent a potential issue where new chunks are
	// unnecessarily allocated.
	while (AvailableChunkIds.Num() > 0 && !ExistingChunk)
	{
		ExistingChunk = Chunks.Find(AvailableChunkIds.Top());

		if (!ExistingChunk)
		{
			UE_LOG(LogMantle, Error, TEXT("A chunk ID marked as available was invalid: %s. Grabbing the next available chunk"), *AvailableChunkIds.Top().ToString());
			AvailableChunkIds.Pop();
		}
	}

	if (ExistingChunk)
	{
		return *ExistingChunk;
	}

	FGuid NewChunkId = FGuid::NewGuid();
	FMantleDBChunk& NewChunk = Chunks.Add(NewChunkId, FMantleDBChunk(NewChunkId, Archetype, this, MasterRecord));
	AllChunkIds.Add(NewChunkId);
	AvailableChunkIds.Add(NewChunkId);

	if (AllChunkIds.Num() == Ananke::Mantle::kChunkCountWarnThreshold)
	{
		// TODO(): log the archetype once archetype.toString() is implemented.
		UE_LOG(LogMantle, Warning, TEXT("Chunk warn threshold reached."));
	}
	
	return NewChunk;
}

void FMantleDBEntry::MakeAvailable(FGuid& ChunkId)
{
	if (!Chunks.Contains(ChunkId))
	{
		UE_LOG(LogMantle, Error, TEXT("Invalid ChunkId in FMantleDBEntry::MakeAvailable()"));
		return;
	}

	AvailableChunkIds.Add(ChunkId);
}

// End FMantleDBEntry -------------------------------------------------------------------------------------------------

// UMantleDB ----------------------------------------------------------------------------------------------------------
void UMantleDB::Initialize(TArray<UScriptStruct*>& ComponentTypes, int32 ChunkSizeBytes)
{
	UE_LOG(LogMantle, Log, TEXT("Initializing MantleDB."));
	
	if (bIsInitialized)
	{
		UE_LOG(LogMantle, Warning, TEXT("Attempted to initialize MantleDB but it was already initialized."));
		return;
	}

	if (ChunkSizeBytes < Ananke::Mantle::kMinChunkSize)
	{
		UE_LOG(LogMantle, Fatal, TEXT("DB chunk size must be at least %d bytes"), Ananke::Mantle::kMinChunkSize);
		return;
	}

	MasterRecord.ChunkComponentBlobSize = ChunkSizeBytes;
	
	int32 NextArchetypeIndex = 0;
	
	for (UScriptStruct* ComponentType : ComponentTypes)
	{
		if (!ComponentType)
		{
			UE_LOG(LogMantle, Error, TEXT("Invalid component type found while initializing MantleDB"));
			continue;
		}

		FMantleComponentInfo NewComponentInfo;
		NewComponentInfo.Name = ComponentType->GetName();
		NewComponentInfo.ArchetypeIndex = NextArchetypeIndex;
		NewComponentInfo.StructSize = ComponentType->GetStructureSize();
		NewComponentInfo.StructAlignment = ComponentType->GetMinAlignment();

		MasterRecord.ComponentInfoMap.Add(NewComponentInfo.Name, NewComponentInfo);
		NextArchetypeIndex++;
	}

	if (MasterRecord.ComponentInfoMap.Num() <= 0)
	{
		return;
	}

	TBitArray<> BareArchetype = TBitArray<>(false, MasterRecord.ComponentInfoMap.Num());
	GetOrCreateEntry(BareArchetype);

	bIsInitialized = true;
	UE_LOG(LogMantle, Log, TEXT("Finished initializing MantleDB."));
}

FMantleIterator UMantleDB::AddEntityAndIterate(const TArray<FInstancedStruct>& InitialComposition)
{
	return AddEntities(InitialComposition, 1);
}

FGuid UMantleDB::AddEntity(const TArray<FInstancedStruct>& InitialComposition)
{
	FMantleIterator Result = AddEntityAndIterate(InitialComposition);
	if(!Result.Next() || Result.GetEntities().Num() != 1)
	{
		UE_LOG(LogMantle, Error, TEXT("Failed to add entity."));
		return FGuid();
	}
	return Result.GetEntities()[0];
}

FMantleIterator UMantleDB::AddEntities(const TArray<FInstancedStruct>& InitialComposition, const int32 NumEntities)
{
	TBitArray<> Archetype = TBitArray<>(false, MasterRecord.ComponentInfoMap.Num());
	TArray<FString> ComponentTypes;

	for (FInstancedStruct ComponentInstance : InitialComposition)
	{
		// TODO(): consider adding some protection to prevent someone from adding multiple structs with the same type.
		ComponentTypes.Add(ComponentInstance.GetScriptStruct()->GetName());
	}

	FillArchetype(Archetype, &ComponentTypes);
	TSharedPtr<FMantleDBEntry> DBEntry = GetOrCreateEntry(Archetype);
	
	FMantleIterator ResultIterator;
	ResultIterator.MasterRecord = &MasterRecord;
	ResultIterator.LocalCache.QueryArchetype = Archetype;
	ResultIterator.LocalCache.MatchingEntries.Add(FMantleCachedEntry(Archetype));
	
	DBEntry->AddEntities(InitialComposition, NumEntities, ResultIterator.LocalCache.MatchingEntries[0]);
	EntryWasModified(Archetype);

	// Make sure that the ResultIterator has a valid matching query in the cache.
	if (!RefreshCachedQuery(Archetype))
	{
		UE_LOG(LogMantle, Error, TEXT("AddEntities: RefreshCachedQuery failed."));
		return FMantleIterator();
	}
	
	ResultIterator.LocalCache.Version = MasterRecord.CachedQueries.Find(Archetype)->Version;
	return ResultIterator;
}

void UMantleDB::RemoveEntities(const TArray<FGuid>& EntityIds)
{
	TSet<TBitArray<>> ModifiedArchetypes;
	
	for (FGuid EntityId : EntityIds)
	{
		if (!EntityId.IsValid())
		{
			UE_LOG(LogMantle, Warning, TEXT("Attempted to remove invalid entity."));
			continue;
		}
		
		FMantleEntity* Entity = MasterRecord.EntitiesById.Find(EntityId);
		if (!Entity)
		{
			UE_LOG(LogMantle, Warning, TEXT("Attempted to remove unknown entity: %s"), *EntityId.ToString());
			continue;
		}

		FMantleDBChunk* Chunk = GetChunk(Entity->Archetype, Entity->ChunkId);

		if (!Chunk)
		{
			UE_LOG(LogMantle, Error, TEXT("Invalid chunk"));
			continue;
		}
		
		Chunk->RemoveEntity(*Entity);
		MasterRecord.RemoveEntity(Entity->Id);
		ModifiedArchetypes.Add(Chunk->Archetype);
	}

	for (TBitArray<> Archetype : ModifiedArchetypes)
	{
		EntryWasModified(Archetype);
	}
}

FMantleIterator UMantleDB::UpdateEntity(
	FGuid& EntityId, TArray<FInstancedStruct>& ComponentsToAdd, TArray<UScriptStruct*>& ComponentsToRemove)
{
	TArray<FGuid> IdArray;
	IdArray.Add(EntityId);
	return UpdateEntities(IdArray, ComponentsToAdd, ComponentsToRemove);
}

FMantleIterator UMantleDB::UpdateEntity(FGuid& EntityId, TArray<FInstancedStruct>& ComponentsToAdd)
{
	TArray<FGuid> IdArray;
	IdArray.Add(EntityId);
	return UpdateEntities(IdArray, ComponentsToAdd);
}

FMantleIterator UMantleDB::UpdateEntity(FGuid& EntityId, TArray<UScriptStruct*>& ComponentsToRemove)
{
	TArray<FGuid> IdArray;
	IdArray.Add(EntityId);
	return UpdateEntities(IdArray, ComponentsToRemove);
}

FMantleIterator UMantleDB::UpdateEntities(
	TArray<FGuid>& EntityIds, TArray<FInstancedStruct>& ComponentsToAdd, TArray<UScriptStruct*>& ComponentsToRemove)
{
	if (EntityIds.Num() == 0)
	{
		return FMantleIterator();
	}

	TBitArray<> OldArchetype;
	TArray<FGuid> ValidEntities;
	
	for (FGuid EntityId : EntityIds)
	{
		FMantleEntity* CurrentEntity = MasterRecord.EntitiesById.Find(EntityId);

		if (!CurrentEntity)
		{
			UE_LOG(LogMantle, Error, TEXT("UpdateEntities: No entity record found for id: %s"), *EntityId.ToString());
			continue;
		}

		if (CurrentEntity->Archetype.IsEmpty())
		{
			UE_LOG(LogMantle, Error, TEXT("UpdateEntities: CurrentEntity archetype is empty."));
			continue;
		}

		if (OldArchetype.IsEmpty())
		{
			OldArchetype = CurrentEntity->Archetype;
		}
		else if (CurrentEntity->Archetype != OldArchetype)
		{
			UE_LOG(LogMantle, Error, TEXT("Unable to batch update entities with mixed archetypes."));
			return FMantleIterator();
		}

		ValidEntities.Add(EntityId);
	}
	
	if (ValidEntities.IsEmpty())
	{
		UE_LOG(LogMantle, Warning, TEXT("UpdateEntities: no valid entities found."))
		return FMantleIterator();
	}

	TBitArray<> NewArchetype = OldArchetype;
	TArray<FString> ToAddNames;
	TArray<FString> ToRemoveNames;
	
	for (FInstancedStruct ComponentInstance : ComponentsToAdd)
	{
		ToAddNames.Add(ComponentInstance.GetScriptStruct()->GetName());
	}
	for (UScriptStruct* ComponentType : ComponentsToRemove)
	{
		if (!ComponentType)
		{
			UE_LOG(LogMantle, Error, TEXT("null script struct while iterating ComponentsToRemove"));
			continue;
		}
		
		ToRemoveNames.Add(ComponentType->GetName());
	}

	FillArchetype(NewArchetype, &ToAddNames, &ToRemoveNames);
	if (NewArchetype == OldArchetype)
	{
		UE_LOG(LogMantle, Error, TEXT("Update Entities: Destination archetype is the same as the source archetype."));
		return FMantleIterator();
	}

	TSharedPtr<FMantleDBEntry> OldEntry = GetEntry(OldArchetype);
	if (!OldEntry.IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("UpdateEntities: can't find old entry."));
		return FMantleIterator();
	}
	
	TSharedPtr<FMantleDBEntry> NewEntry = GetOrCreateEntry(NewArchetype);
	if (!NewEntry.IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("UpdateEntities: can't find new entry."));
		return FMantleIterator();
	}

	FMantleIterator ResultIterator;
	ResultIterator.MasterRecord = &MasterRecord;
	ResultIterator.LocalCache.QueryArchetype = NewArchetype;
	ResultIterator.LocalCache.MatchingEntries.Add(FMantleCachedEntry(NewArchetype));

	NewEntry->TakeEntities(ValidEntities, *OldEntry, ComponentsToAdd, ResultIterator.LocalCache.MatchingEntries[0]);
	EntryWasModified(OldArchetype);
	EntryWasModified(NewArchetype);
	
	// Make sure that the ResultIterator has a valid matching query in the cache.
	if (!RefreshCachedQuery(NewArchetype))
	{
		UE_LOG(LogMantle, Error, TEXT("UpdateEntities: RefreshCachedQuery failed."));
		return FMantleIterator();
	}
	
	ResultIterator.LocalCache.Version = MasterRecord.CachedQueries.Find(NewArchetype)->Version;
	return ResultIterator;
}

FMantleIterator UMantleDB::UpdateEntities(TArray<FGuid>& EntityIds, TArray<FInstancedStruct>& ComponentsToAdd)
{
	TArray<UScriptStruct*> Unused;
	return UpdateEntities(EntityIds, ComponentsToAdd, Unused);
}

FMantleIterator UMantleDB::UpdateEntities(TArray<FGuid>& EntityIds, TArray<UScriptStruct*>& ComponentsToRemove)
{
	TArray<FInstancedStruct> Unused;
	return UpdateEntities(EntityIds, Unused, ComponentsToRemove);
}

FMantleIterator UMantleDB::RunQuery(FMantleComponentQuery& Query)
{
	if (Query.CachedArchetype.IsEmpty())
	{
		Query.CachedArchetype = TBitArray<>(false, MasterRecord.ComponentInfoMap.Num());
		FillArchetype(Query.CachedArchetype, &Query.RequiredComponents);
	}

	return RunQueryInternal(Query.CachedArchetype);
}

void UMantleDB::FillArchetype(TBitArray<>& Archetype, TArray<FString>* ToAdd, TArray<FString>* ToRemove)
{
	if (ToAdd)
	{
		for (FString ComponentType : *ToAdd)
		{
			FMantleComponentInfo* ComponentInfo = MasterRecord.ComponentInfoMap.Find(ComponentType);
			if (!ComponentInfo)
			{
				UE_LOG(LogMantle, Error, TEXT("Attempted to add unknown component type: %s"), *ComponentType);
				continue;
			}

			Archetype[ComponentInfo->ArchetypeIndex] = true;
		}
	}
	if (ToRemove)
	{
		for (FString ComponentType : *ToRemove)
		{
			FMantleComponentInfo* ComponentInfo = MasterRecord.ComponentInfoMap.Find(ComponentType);
			if (!ComponentInfo)
			{
				UE_LOG(LogMantle, Error, TEXT("Attempted to remove unknown component type: %s"), *ComponentType);
				continue;
			}

			Archetype[ComponentInfo->ArchetypeIndex] = false;
		}
	}
}

TSharedPtr<FMantleDBEntry> UMantleDB::GetEntry(TBitArray<>& Archetype)
{
	TSharedPtr<FMantleDBEntry>* ExistingEntry = EntriesByArchetype.Find(Archetype);
	
	if (!ExistingEntry || !ExistingEntry->IsValid())
	{
		return nullptr;
	}

	return *ExistingEntry;
}

TSharedPtr<FMantleDBEntry> UMantleDB::GetOrCreateEntry(TBitArray<>& Archetype)
{
	TSharedPtr<FMantleDBEntry>* ExistingEntry = EntriesByArchetype.Find(Archetype);
	
	if (ExistingEntry && ExistingEntry->IsValid())
	{
		return *ExistingEntry;
	}

	TSharedPtr<FMantleDBEntry> NewEntry = MakeShareable(new FMantleDBEntry(Archetype, MasterRecord));
	EntriesByArchetype.Add(Archetype, NewEntry);
	ActiveArchetypes.Add(Archetype);

	FMantleCachedEntry& CachedEntry = MasterRecord.FindOrAddCachedEntry(Archetype);
	
	for (auto Iterator = MasterRecord.CachedQueries.CreateIterator(); Iterator; ++Iterator)
	{
		TBitArray<> QueryArchetype = Iterator.Key();
		
		if (TBitArray<>::BitwiseAND(Archetype, QueryArchetype, EBitwiseOperatorFlags::MinSize) != QueryArchetype)
		{
			continue;
		}

		CachedEntry.MatchingQueries.Add(QueryArchetype);
		Iterator.Value().Version.Invalidate();
	}
	
	return NewEntry;
}

FMantleIterator UMantleDB::RunQueryInternal(TBitArray<>& QueryArchetype)
{
	FMantleCachedQuery* CachedQuery = MasterRecord.CachedQueries.Find(QueryArchetype);
	if (CachedQuery && CachedQuery->Version.IsValid())
	{
		return FMantleIterator(*CachedQuery, &MasterRecord);
	}
	if (!CachedQuery)
	{
		CachedQuery = &MasterRecord.CachedQueries.Add(QueryArchetype, FMantleCachedQuery(QueryArchetype));
	}
	
	CachedQuery->ClearData();
	
	for (TBitArray<> Archetype : ActiveArchetypes)
	{
		if (TBitArray<>::BitwiseAND(Archetype, QueryArchetype, EBitwiseOperatorFlags::MinSize) != QueryArchetype)
		{
			continue;
		}

		FMantleCachedEntry& CachedEntry = MasterRecord.FindOrAddCachedEntry(Archetype);
		if (!CachedEntry.bIsValid && !RefreshCachedEntry(CachedEntry))
		{
			return FMantleIterator();
		}

		CachedQuery->MatchingEntries.Add(CachedEntry);
		CachedEntry.MatchingQueries.Add(CachedQuery->QueryArchetype);
	}
	
	CachedQuery->Version.Update();
	return FMantleIterator(*CachedQuery, &MasterRecord);
}

bool UMantleDB::RefreshCachedEntry(FMantleCachedEntry& CachedEntry)
{
	// In the future we may refresh the CachedEntry on a per-chunk basis, but for now, we just clear everything
	// and recompute.
	CachedEntry.ChunkedComponents.Empty();
	CachedEntry.ChunkedEntityIds.Empty();
			
	TSharedPtr<FMantleDBEntry>* EntryPtr = EntriesByArchetype.Find(CachedEntry.Archetype);
	if (!EntryPtr || !(EntryPtr)->IsValid())
	{
		// TODO(): Implement ToString(Archetype) returns: "[CompName, CompName, ....]"
		UE_LOG(LogMantle, Error, TEXT("Invalid Archetype during ComponentQuery."));
		return false;
	}

	FMantleDBEntry* Entry = EntryPtr->Get();
		
	for (FGuid ChunkId : Entry->AllChunkIds)
	{
		FMantleDBChunk* Chunk = Entry->Chunks.Find(ChunkId);

		if (!Chunk)
		{
			// TODO(): Remove chunk & log error?
			continue;
		}
		if (Chunk->IsEmpty())
		{
			continue;
		}

		CachedEntry.ChunkedEntityIds.Add(TArrayView<FGuid>(Chunk->EntityIds));

		// We cache all component data for a particular entry even if the current query doesn't need it.
		for (FString& ComponentType : Entry->ComponentTypes)
		{
			FMantleComponentInfo* ComponentInfo = Chunk->ComponentTypeInfo.Find(ComponentType);

			if (!ComponentInfo)
			{
				UE_LOG(LogMantle, Error, TEXT("Malformed Chunk: ComponentInfo for %s is missing."), *ComponentType);
				return false;
			}
				
			CachedEntry.ChunkedComponents.FindOrAdd(ComponentType).Add(
			FAnankeUntypedArrayView(ComponentInfo->ChunkLocation, Chunk->EntityIds.Num())
			);
		}
	}

	CachedEntry.bIsValid = true;

	return true;
}

void UMantleDB::EntryWasModified(TBitArray<>& EntryArchetype)
{
	FMantleCachedEntry* CachedEntry = MasterRecord.CachedEntries.Find(EntryArchetype);

	if (!CachedEntry)
	{
		UE_LOG(LogMantle, Error, TEXT("No CachedEntry found during EntryWasModified()"));
		return;
	}

	CachedEntry->bIsValid = false;

	for (TBitArray<> QueryArchetype : CachedEntry->MatchingQueries)
	{
		FMantleCachedQuery* CachedQuery = MasterRecord.CachedQueries.Find(QueryArchetype);

		if (CachedQuery)
		{
			CachedQuery->Version.Invalidate();
		}
	}
}

bool UMantleDB::RefreshCachedQuery(TBitArray<>& Archetype)
{
	FMantleCachedQuery* CachedQuery = MasterRecord.CachedQueries.Find(Archetype);
	if (!CachedQuery || !CachedQuery->Version.IsValid())
	{
		// There must be a valid cached query available for the following reasons:
		//   1) ResultIterator will not be iterable if the cached query for its archetype is invalid.
		//   2) A cached query must exist so that ResultIterator can be invalidated by further changes to the DB.
		RunQueryInternal(Archetype);
	}

	// Sanity check to make sure that the RunQueryInternal was successful.
	return MasterRecord.CachedQueries.Contains(Archetype);
}
// End UMantleDB ------------------------------------------------------------------------------------------------------