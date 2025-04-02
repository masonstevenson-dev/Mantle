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

#include "Foundation/MantleQueries.h"

#include "MantleRuntimeLoggingDefs.h"

TArrayView<FGuid> FMantleIterator::GetEntities()
{
	if (!IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("Attempted to call NumEntities() on invalid Iterator. [0]"));
		return TArrayView<FGuid>();
	}

	if (EntryIndex < 0 || EntryIndex >= LocalCache.MatchingEntries.Num())
	{
		UE_LOG(LogMantle, Error, TEXT("Attempted to call GetArrayView() on invalid Iterator. [1]"));
		return TArrayView<FGuid>();
	}

	if (LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds.Num() == 0)
	{
		return TArrayView<FGuid>();
	}
	
	if (ChunkIndex < 0 || ChunkIndex >= LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds.Num())
	{
		UE_LOG(LogMantle, Error, TEXT("Attempted to call GetArrayView() on invalid Iterator. [2]"));
		return TArrayView<FGuid>();
	}

	return LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds[ChunkIndex];
}

bool FMantleIterator::Next()
{
	if (!IsValid())
	{
		UE_LOG(LogMantle, Error, TEXT("Attempted to call Next() on invalid Iterator."));
		return false;
	}
	
	int32 NumEntries = LocalCache.MatchingEntries.Num();

	if (EntryIndex >= NumEntries)
	{
		return false;
	}

	if (EntryIndex >= 0)
	{
		if (ChunkIndex < LocalCache.MatchingEntries[EntryIndex].NumChunks() - 1)
		{
			ChunkIndex++;
		}
		else
		{
			EntryIndex++;
			ChunkIndex = 0;
		}
	}
	else
	{
		// First time only: Set the entry index to the first valid location instead of incrementing. This allows us to
		// call while(Iterator.Next()) without missing the first chunk of data.
		EntryIndex = 0;
	}

	// TODO(): Currently this returns true if a entry for this exists but is empty.
	return EntryIndex < NumEntries;
}

void FMantleIterator::Reset()
{
	EntryIndex = -1;
	ChunkIndex = 0;
}

bool FMantleIterator::IsValid()
{
	// Our local cache may have gone out of date. Check the DB to see.
	
	if (!MasterRecord)
	{
		return false;
	}

	FMantleCachedQuery* DBCachedQuery = MasterRecord->CachedQueries.Find(LocalCache.QueryArchetype);

	if (!DBCachedQuery)
	{
		return false;
	}

	return DBCachedQuery->Version.IsValid() && DBCachedQuery->Version == LocalCache.Version;
}