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

#include "Operations/EffectExecutions/EE_SimpleDamageEffect.h"

#include "MantleComponents/MC_Health.h"
#include "MantleComponents/EffectPayloads/EP_SimpleDamageEffect.h"

UEE_SimpleDamageEffect::UEE_SimpleDamageEffect(const FObjectInitializer& Initializer): Super(Initializer)
{
	Query.AddRequiredComponent<FEP_SimpleDamageEffect>();
}

void UEE_SimpleDamageEffect::LoadEffectPayloads(FMantleIterator& Iterator)
{
	EffectData = Iterator.GetArrayView<FEP_SimpleDamageEffect>();
}

FMantleEffectExecutionResult UEE_SimpleDamageEffect::Execute(FMantleOperationContext& Ctx, int32 EffectIndex, bool CancelRequested)
{
	FMantleEffectExecutionResult Result;

	if (CancelRequested)
	{
		Result.ExecutionStatus = EMantleEffectExecutionStatus::Cancel;
		return Result;
	}
	
	if (EffectIndex < 0 || EffectIndex >= EffectData.Num())
	{
		ANANKE_LOG_PERIODIC(Error, TEXT("Invalid EffectIndex"), 1.0);
		Result.ExecutionStatus = EMantleEffectExecutionStatus::Cancel;
		return Result;
	}

	FEP_SimpleDamageEffect& DamageInfo = EffectData[EffectIndex];

	if (!Ctx.MantleDB->HasComponent<FMC_Health>(DamageInfo.TargetEntity))
	{
		Result.ExecutionStatus = EMantleEffectExecutionStatus::Cancel;
		return Result;
	}
	
	FMC_Health* TargetHealth = Ctx.MantleDB->GetComponent<FMC_Health>(DamageInfo.TargetEntity);
	if (!TargetHealth)
	{
		ANANKE_LOG_PERIODIC(Error, TEXT("TargetHeath is missing."), 1.0);
		Result.ExecutionStatus = EMantleEffectExecutionStatus::Cancel;
		return Result;
	}

	TargetHealth->ApplyDamage(DamageInfo.DamageAmount);
	Result.ExecutionStatus = EMantleEffectExecutionStatus::Succeeded;
	return Result;
}
