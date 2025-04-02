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

#include "Engine/MantleGameViewportClient.h"

/*
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Stats/StatsData.h"

void UMantleGameViewportClient::Draw(FViewport* InViewport, FCanvas* SceneCanvas)
{
	Super::Draw(InViewport, SceneCanvas);
	DrawMantleStatsHUD(InViewport, SceneCanvas);	
}

void UMantleGameViewportClient::DrawMantleStatsHUD(FViewport* InViewport, FCanvas* SceneCanvas)
{
	FCanvas* DebugCanvas = Viewport->GetDebugCanvas();
	if (!DebugCanvas)
	{
		return;
	}

	UWorld* CurrentWorld = GetWorld();
	if (!CurrentWorld)
	{
		return;
	}

#if STATS
	FGameThreadStatsData* ViewData = FLatestGameThreadStatsData::Get().Latest;
	if (!ViewData)
	{
		return;
	}

	if (HasIncrementedErrorStat(*ViewData))
	{
		FCanvasTextItem CanvasText = FCanvasTextItem(FVector2D(0, 0), FText::GetEmpty(), GEngine->GetSmallFont(), FLinearColor::Red);
		DebugCanvas->DrawItem(CanvasText, FVector2D(40, GIsEditor ? 45 : 100));
	}

	
#endif
}

bool UMantleGameViewportClient::HasIncrementedErrorStat(FGameThreadStatsData& ViewData)
{
	for (int32 GroupIndex = 0; GroupIndex < ViewData.ActiveStatGroups.Num(); ++GroupIndex)
	{
		FString StatGroupName = ViewData.GroupNames[GroupIndex].ToString();

		if (!StatGroupName.Equals(TEXT("STATGROUP_MantleErrors")))
		{
			continue;
		}

		const FActiveStatGroupInfo& ErrorStats = ViewData.ActiveStatGroups[GroupIndex];
		for (const FComplexStatMessage& ErrorStat : ErrorStats.CountersAggregate)
		{
			if (ErrorStat.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_double)
			{
				double StatValue = ErrorStat.GetValue_double(EComplexStatField::IncMax);

				if (StatValue > 0)
				{
					return true;
				}
			}
			else if (ErrorStat.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64)
			{
				int64 StatValue = ErrorStat.GetValue_int64(EComplexStatField::IncMax);

				if (StatValue > 0)
				{
					return true;
				}
			}
		}
	}

	return false;
}*/