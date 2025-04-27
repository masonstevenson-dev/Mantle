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

#include "MantleTypes.generated.h"

// Is it safe to add a container type (TArray, TMap, etc) to FMantleComponent?
//   -> Currently, I think NO, it would result in a memory leak. Basically what I think happens is when you add an entity
//      to the DB (see FMantleDBChunk::AddEntities), it creates a deep copy of the struct, including any TArray, TMap,
//      etc (see ComponentInstance.GetScriptStruct()->CopyScriptStruct, FArrayProperty::CopyValuesInternal). However,
//      when the entity is destroyed, we currently do not call the destructor on each component struct. So any memory
//      that got allocated during the copy (array data for example), get leaked.
//
//		TODO(): To fix, need to fetch the script struct and call ScriptStruct->DestroyStruct(DataPtr) (see FMassArchetypeData::RemoveEntity for example)
//
// Do mantle components respect GC?
//   -> No. So basically there is no point in using the UPROPERTY() tag on an FMantleComponent, and furthermore it is
//      not safe to store a TObjectPtr<SomeUObject> and expect that object to not get GC'd.
USTRUCT()
struct MANTLERUNTIME_API FMantleComponent
{
	GENERATED_BODY()
};

// Used for testing only.
USTRUCT()
struct FMantleTestComponent : public FMantleComponent
{
	GENERATED_BODY()
};
