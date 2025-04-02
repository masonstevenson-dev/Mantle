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
#include "GameFramework/Controller.h"
#include "UObject/WeakObjectPtrTemplates.h"

#include "MC_Viewpoint.generated.h"

/**
 *  Holds data about the location and direction of an entity's "eyes".
 */
USTRUCT()
struct MANTLERUNTIME_API FMC_Viewpoint : public FMantleComponent
{
	GENERATED_BODY()

public:
	FMC_Viewpoint() = default;
	FMC_Viewpoint(AController* NewSource)
	{
		SetViewpointSourceController(NewSource);
	}
	
	void SetViewpointSourceController(AController* NewSource) { ViewpointSource = NewSource; }
	AController* GetViewpointSourceController() { return ViewpointSource.IsValid() ? ViewpointSource.Get() : nullptr; }

	bool IsValid()
	{
		return ViewpointSource.IsValid();
	}
	
	bool IsPlayerViewpoint()
	{
		return ViewpointSource.IsValid() && ViewpointSource->IsA<APlayerController>();
	}
	
	FVector Location = FVector::ZeroVector;
	FRotator Rotation = FRotator::ZeroRotator;
	double LastTimeProcessedSec = 0.0;

protected:
	// The controller to retrieve data from.
	UPROPERTY()
	TWeakObjectPtr<AController> ViewpointSource;
};
