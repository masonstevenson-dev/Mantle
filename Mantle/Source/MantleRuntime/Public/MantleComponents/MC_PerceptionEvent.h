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
#include "Foundation/MantleTypes.h"
#include "Misc/Guid.h"

#include "MC_PerceptionEvent.generated.h"

/**
 *  Indicates that an entity is perceiving another entity in some way. Additional components add context. For example,
 *  an entity with this component + the MC_ViewpointTraceEvent component is produced when the ViewpointTrace operation
 *  performs a line trace from an entity's "viewpoint".
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_PerceptionEvent : public FMantleComponent
{
	GENERATED_BODY()

public:
	FMC_PerceptionEvent() = default;
	
	// Constructor for regular events.
	FMC_PerceptionEvent(FGuid& NewSource, FGuid& NewTarget, bool bNewBlockingHit):
		SourceEntity(NewSource), TargetEntity(NewTarget), bBlockingHit(bNewBlockingHit) { }

	// Constructor for 'no targets' events.
	FMC_PerceptionEvent(FGuid& NewSource): SourceEntity(NewSource) { }

	bool HasTarget()
	{
		return TargetEntity.IsValid();
	}
	
	FGuid SourceEntity;
	FGuid TargetEntity;

	// If true, this was a 'blocking' hit. Otherwise this was an overlap.
	bool bBlockingHit = false;
};

/**
 *  Used to filter events that are specific to players.
 */ 
USTRUCT()
struct MANTLERUNTIME_API FMC_PlayerPerceptionEvent : public FMantleComponent { GENERATED_BODY() };

/**
 *  Used to filter events that are specific to AI.
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_AIPerceptionEvent : public FMantleComponent { GENERATED_BODY() };
