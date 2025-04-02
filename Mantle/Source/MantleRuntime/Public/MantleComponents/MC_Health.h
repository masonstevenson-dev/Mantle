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
#include "Containers/AnankeDynamicValue.h"
#include "Foundation/MantleDB.h"
#include "Foundation/MantleTypes.h"
#include "Math/UnrealMathUtility.h"

#include "MC_Health.generated.h"

// TODO(): Consider making this a MULTICAST_DELEGATE since it isn't currently bound to using BP.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMC_Health_HealthChangedEvent, float, OldVal, float, NewVal);

USTRUCT(BlueprintType)
struct MANTLERUNTIME_API FMC_Health : public FMantleComponent
{
	GENERATED_BODY()

public:
	FMC_Health() = default;
	FMC_Health(float NewHealth, float NewMaxHealth)
	{
		Health = NewHealth;
		MaxHealth = FAnankeDynamicValue(NewMaxHealth);
	}

	float GetHealth()
	{
		return Health;
	}
	FAnankeDynamicValue GetMaxHealth()
	{
		return MaxHealth;
	}
	
	void ApplyHealing(float Amount)
	{
		UpdateHealth(FMath::Abs(Amount));
	}
	void ApplyDamage(float Amount)
	{
		UpdateHealth(FMath::Abs(Amount) * -1);
	}
	void SetHealth(float NewHealth)
	{
		if (Health == NewHealth)
		{
			return;	
		}

		float OldHealth = Health;
		Health = NewHealth;
		OnHealthChanged.Broadcast(OldHealth, Health);
	}
	void SetMaxHealth(FAnankeDynamicValue NewMaxHealth)
	{
		if (NewMaxHealth == MaxHealth)
		{
			return;
		}

		float OldValue = MaxHealth.Value();
		MaxHealth = NewMaxHealth;
		float NewValue = MaxHealth.Value();

		// We still have to check the values here because the dynamic value could have updated, but the result value
		// could stay the same. For example, if the old dynamic value is {(10 * 1) + 5} and the new value is
		// {(10 * 2) - 5}.
		if (OldValue != NewValue)
		{
			OnMaxHealthChanged.Broadcast(OldValue, NewValue);	
		}
	}

	FMC_Health_HealthChangedEvent OnHealthChanged;
	FMC_Health_HealthChangedEvent OnMaxHealthChanged;

private:
	float Health = 0.0f;
	FAnankeDynamicValue MaxHealth;
	
	void UpdateHealth(float Amount)
	{
		float OldHealth = Health;
		Health = FMath::Clamp(Health + Amount, 0, MaxHealth.Value());

		if (Health != OldHealth)
		{
			OnHealthChanged.Broadcast(OldHealth, Health);
		}
	}
};
