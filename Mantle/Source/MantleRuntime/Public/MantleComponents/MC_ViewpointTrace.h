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
#include "Containers/EnumAsByte.h"
#include "Engine/EngineTypes.h"
#include "Foundation/MantleTypes.h"
#include "MantleComponents/MC_PerceptionEvent.h"

#include "MC_ViewpointTrace.generated.h"

UENUM()
enum class EOverlapEmission : uint8
{
	// Emit all overlap events.
	All,

	// Emit no overlap events.
	None
};

UENUM()
enum class EBlockingHitEmission : uint8
{
	// Emit all blocking hit events.
	All,

	// Emit blocking hit events only when what we are looking at changed.
	Delta,

	// Emit no blocking hit events.
	None
};

/**
 *  Allows for filtering perception events that were produced using the ViewpointTrace method.
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_ViewpointTraceEvent : public FMantleComponent { GENERATED_BODY() };

/**
 *  Configuration for performing a viewpoint trace.
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_ViewpointTrace : public FMantleComponent
{
	GENERATED_BODY()

public:
	bool bEnabled = true;
	double LastScanTimeSec = 0.0;
	FMC_PerceptionEvent LastBlockingHit;
	
	// Trace options
	float ScanRange = 200.0f;
	double ScanRateSec = 0.01;
	ECollisionChannel TraceChannel = ECC_Visibility;
	EOverlapEmission OverlapRule = EOverlapEmission::None;
	EBlockingHitEmission BlockingHitRule = EBlockingHitEmission::Delta;
	bool bDrawDebugGeometry = false;
	double MaxViewpointDataAgeSec = 0.5; // 500 ms
};
