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
#include "Foundation/MantleOperation.h"
#include "Foundation/MantleQueries.h"

#include "MantleEffectExecutor.generated.h"

UENUM()
enum class EMantleEffectExecutionStatus : uint8
{
	Succeeded,
	Failed,
	Cancel
};

USTRUCT()
struct FMantleEffectExecutionResult
{
	GENERATED_BODY()

public:
	FMantleEffectExecutionResult() = default;
	FMantleEffectExecutionResult(EMantleEffectExecutionStatus NewExecutionStatus): ExecutionStatus(NewExecutionStatus) {}
	
	EMantleEffectExecutionStatus ExecutionStatus = EMantleEffectExecutionStatus::Failed;
};

UCLASS()
class MANTLERUNTIME_API UMantleEffectExecutor : public UMantleOperation
{
	GENERATED_BODY()

public:
	UMantleEffectExecutor(const FObjectInitializer& Initializer);

protected:
	virtual void PerformOperation(FMantleOperationContext& Ctx) override;

	// Override this fn to grab whatever payload components are attached to your specific effect entity type. While you
	// technically *can* also grab FEP_EffectMetadata, modifying it could cause this base Executor class to stop
	// functioning properly. Ideally, any changes to the EffectMetadata should be communicated from the child executor
	// to this base class via the FMantleEffectExecutionResult struct.
	virtual void LoadEffectPayloads(FMantleIterator& Iterator) {} 
	virtual FMantleEffectExecutionResult Execute(FMantleOperationContext& Ctx, int32 EffectIndex, bool CancelRequested)
	{
		return FMantleEffectExecutionResult();
	}
	
	FMantleComponentQuery Query;
};
