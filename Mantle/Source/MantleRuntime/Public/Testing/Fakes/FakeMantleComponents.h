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
#include "Containers/UnrealString.h"
#include "Foundation/MantleTypes.h"
#include "GameFramework/Actor.h"
#include "Math/Transform.h"
#include "UObject/ObjectPtr.h"

#include "FakeMantleComponents.generated.h"

USTRUCT()
struct FFakeTransformComponent : public FMantleTestComponent
{
	GENERATED_BODY()

public:
	FFakeTransformComponent() = default;
	
	FFakeTransformComponent(FTransform& NewTransform)
	{
		Transform = NewTransform;
	}
	
	UPROPERTY()
	FTransform Transform;
};

USTRUCT()
struct FFakeItemComponent : public FMantleTestComponent
{
	GENERATED_BODY()

public:
	FFakeItemComponent() = default;

	FFakeItemComponent(FString NewName, float NewWeight, float NewCost)
	{
		Name = NewName;
		Weight = NewWeight;
		Cost = NewCost;
	}
	
	UPROPERTY()
	FString Name = TEXT("");

	UPROPERTY()
	float Weight = 1.0;
	
	UPROPERTY()
	float Cost = 1.0;
};

USTRUCT()
struct FFakeTargetingComponent : public FMantleTestComponent
{
	GENERATED_BODY()

public:
	FFakeTargetingComponent() = default;
	
	FFakeTargetingComponent(AActor* NewTarget, FString NewName)
	{
		Target = NewTarget;
		TargetName = NewName;
	}

	UPROPERTY()
	TObjectPtr<AActor> Target;

	UPROPERTY()
	FString TargetName;
};

USTRUCT()
struct FFakePlaceholderComponent : public FMantleTestComponent
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float SomeFloatValue = 0.0;
};

USTRUCT()
struct FFakeBigComponent : public FMantleTestComponent
{
	GENERATED_BODY()

public:
	FFakeBigComponent() = default;
	
	FFakeBigComponent(FTransform& NewTransform)
	{
		Transform1 = NewTransform;
		Transform2 = NewTransform;
		Transform3 = NewTransform;
		Transform4 = NewTransform;
		Transform5 = NewTransform;
		Transform6 = NewTransform;
		Transform7 = NewTransform;
		Transform8 = NewTransform;
		Transform9 = NewTransform;
		Transform10 = NewTransform;
	}
	
	UPROPERTY()
	FTransform Transform1;

	UPROPERTY()
	FTransform Transform2;

	UPROPERTY()
	FTransform Transform3;

	UPROPERTY()
	FTransform Transform4;

	UPROPERTY()
	FTransform Transform5;

	UPROPERTY()
	FTransform Transform6;

	UPROPERTY()
	FTransform Transform7;

	UPROPERTY()
	FTransform Transform8;

	UPROPERTY()
	FTransform Transform9;

	UPROPERTY()
	FTransform Transform10;
};

USTRUCT()
struct FFakeEmptyComponent : public FMantleTestComponent
{
	GENERATED_BODY()
};
