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
#include "Containers/ArrayView.h"
#include "Containers/BitArray.h"
#include "Containers/Map.h"
#include "Containers/UnrealString.h"
#include "MantleDB.h"
#include "MantleRuntimeLoggingDefs.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "Misc/Guid.h"
#include "UObject/Class.h"

#include "MantleQueries.generated.h"

USTRUCT()
struct FMantleComponentQuery
{
	GENERATED_BODY()

public:
	template<typename TComponentType>
	void AddRequiredComponent()
	{
		const UScriptStruct* ComponentType = TComponentType::StaticStruct();
		const FString ComponentName = ComponentType->GetName();

		if (RequiredComponents.Contains(ComponentName))
		{
			return;
		}
		
		RequiredComponents.Add(ComponentName);

		if (!CachedArchetype.IsEmpty())
		{
			CachedArchetype.Empty();
		}
	}

protected:
	friend UMantleDB;
	
	TArray<FString> RequiredComponents;
	TBitArray<> CachedArchetype;
};

USTRUCT()
struct MANTLERUNTIME_API FMantleIterator
{
	GENERATED_BODY()
	
public:
	FMantleIterator() = default;
	
	FMantleIterator(FMantleCachedQuery& DBCache, FMantleDBMasterRecord* DBMasterRecord)
	{
		LocalCache = DBCache;
		MasterRecord = DBMasterRecord;
	}
	
	template <typename ViewType>
	TArrayView<ViewType> GetArrayView()
	{
		return GetArrayViewInternal<ViewType>(EntryIndex, ChunkIndex);
	}

	// NOTE: This gets the entities at the CURRENT INDEX.
	TArrayView<FGuid> GetEntities();
	bool Next();
	void Reset();
	bool IsValid();
	
private:
	friend FMantleDBChunk;
	friend TestSuite;
	friend UMantleDB;

	template <typename ViewType>
	TArrayView<ViewType> GetArrayViewInternal(int32 TargetEntryIndex, int32 TargetChunkIndex)
	{
		if (!IsValid())
		{
			ANANKE_LOG_PERIODIC(Error, TEXT("Invalid Iterator."), 1.0);
			return TArrayView<ViewType>();
		}
		if (TargetEntryIndex < 0 || TargetEntryIndex >= LocalCache.MatchingEntries.Num() || TargetChunkIndex < 0)
		{
			ANANKE_LOG_PERIODIC(Error, TEXT("Invalid ArrayView index."), 1.0);
			return TArrayView<ViewType>();
		}
		if (LocalCache.MatchingEntries[TargetEntryIndex].ChunkedEntityIds.Num() == 0)
		{
			return TArrayView<ViewType>();
		}

		const UScriptStruct* ComponentType = ViewType::StaticStruct();
		const FString ComponentName = ComponentType->GetName();
		
		TArray<FAnankeUntypedArrayView>* Chunks = LocalCache.MatchingEntries[TargetEntryIndex].ChunkedComponents.Find(ComponentName);
		if (!Chunks)
		{
			UE_LOG(LogMantle, Error, TEXT("Chunks for component %s are missing."), *ComponentName);
			return TArrayView<ViewType>();
		}
		if (Chunks->Num() == 0)
		{
			return TArrayView<ViewType>();
		}
		if (TargetChunkIndex >= Chunks->Num())
		{
			UE_LOG(LogMantle, Error, TEXT("Invalid chunk index."));
			return TArrayView<ViewType>();
		}

		return (*Chunks)[TargetChunkIndex].GetArrayView<ViewType>();
	}
	
	int32 EntryIndex = -1;
	int32 ChunkIndex = 0;

	FMantleCachedQuery LocalCache;

	// TODO(): Consider storing a weakptr to the MantleDB instead.
	FMantleDBMasterRecord* MasterRecord = nullptr;
};