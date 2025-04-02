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

#include "Foundation/MantleEffectExecutor.h"

#include "MantleComponents/EffectPayloads/EP_EffectMetadata.h"

UMantleEffectExecutor::UMantleEffectExecutor(const FObjectInitializer& Initializer): Super(Initializer)
{
	Query.AddRequiredComponent<FEP_EffectMetadata>();
}

void UMantleEffectExecutor::PerformOperation(FMantleOperationContext& Ctx)
{
	FMantleIterator QueryResult = Ctx.MantleDB->RunQuery(Query);

	TArray<FGuid> EffectsToCleanUp;

	while (QueryResult.Next())
	{
		TArrayView<FGuid> EffectIds = QueryResult.GetEntities();
		TArrayView<FEP_EffectMetadata> Effects = QueryResult.GetArrayView<FEP_EffectMetadata>();
		LoadEffectPayloads(QueryResult);

		for (int32 EffectIndex = 0; EffectIndex < EffectIds.Num(); ++EffectIndex)
		{
			FGuid EffectId = EffectIds[EffectIndex];
			FEP_EffectMetadata& EffectMetadata = Effects[EffectIndex];
			
			const double TimeSinceLastTrigger = FPlatformTime::Seconds() - EffectMetadata.LastTimeTriggered;
			if (TimeSinceLastTrigger < EffectMetadata.TriggerRateSec)
			{
				continue;
			}

			if (EffectMetadata.EffectType == EMantleEffectType::Limited && EffectMetadata.RemainingTriggers <= 0)
			{
				continue;
			}
			
			FMantleEffectExecutionResult ExecutionResult = Execute(Ctx, EffectIndex, EffectMetadata.CancelRequested);

			switch (ExecutionResult.ExecutionStatus)
			{
			case EMantleEffectExecutionStatus::Succeeded:
				EffectMetadata.OnExecuted.Broadcast(Ctx.MantleDB, EffectId);
				break;
			case EMantleEffectExecutionStatus::Failed:
				EffectMetadata.NumFailures++;
				if (EffectMetadata.NumFailures > EffectMetadata.MaxFailures)
				{
					// Can maybe fix by using FWeakObjectPtr instead?
					// Can also maybe fix by switching from DynamicMulticastDelegate to MulticastDelegate. Reminder: Dynamic just lets you serialize the delegate. this means that we wouldn't be allowed to serialize any effects.
					EffectMetadata.OnCanceled.Broadcast(Ctx.MantleDB, EffectId);
					EffectsToCleanUp.Add(EffectId);
					continue;
				}
				break;
			case EMantleEffectExecutionStatus::Cancel:
				EffectMetadata.OnCanceled.Broadcast(Ctx.MantleDB, EffectId);
				EffectsToCleanUp.Add(EffectId);
				continue;
			default:
				break;
			}
			
			if (EffectMetadata.EffectType == EMantleEffectType::Limited)
			{
				EffectMetadata.RemainingTriggers--;

				if (EffectMetadata.RemainingTriggers <= 0 || ExecutionResult.ExecutionStatus == EMantleEffectExecutionStatus::Succeeded)
				{
					EffectMetadata.OnFinished.Broadcast(Ctx.MantleDB, EffectId);
					EffectsToCleanUp.Add(EffectId);
					continue;
				}
			}
			
			EffectMetadata.LastTimeTriggered = FPlatformTime::Seconds();
		}
	}

	if (EffectsToCleanUp.Num() > 0)
	{
		Ctx.MantleDB->RemoveEntities(EffectsToCleanUp);
	}
}
