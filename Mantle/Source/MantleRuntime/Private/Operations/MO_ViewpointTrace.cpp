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

#include "Operations\MO_ViewpointTrace.h"

#include "ActorComponents/MantleAvatarComponent.h"
#include "MantleComponents/MC_Avatar.h"
#include "MantleComponents/MC_PerceptionEvent.h"
#include "MantleComponents/MC_TemporaryEntity.h"
#include "MantleComponents/MC_Viewpoint.h"
#include "MantleComponents/MC_ViewpointTrace.h"
#include "MantleRuntimeLoggingDefs.h"
#include "Macros/AnankeCoreLoggingMacros.h"

void UMO_ViewpointTrace::Initialize()
{
	TraceQuery.AddRequiredComponent<FMC_AvatarActor>();
	TraceQuery.AddRequiredComponent<FMC_Viewpoint>();
	AddRequiredTraceComponent(TraceQuery);
}

void UMO_ViewpointTrace::PerformOperation(FMantleOperationContext& Ctx)
{
	// Note: We don't know how many events will be emitted per entity, and therefore cannot reserve memory for this
	//       array on each chunk iteration. As a potential optimization, we could create a 'max events' limit and then
	//       always reserve that number of slots in this array per entity.
	TArray<FMC_PerceptionEvent> PlayerEventsToEmit;
	TArray<FMC_PerceptionEvent> AIEventsToEmit;

	FMantleIterator QueryIterator = Ctx.MantleDB->RunQuery(TraceQuery);

	while (QueryIterator.Next())
	{
		TArrayView<FGuid> Entities = QueryIterator.GetEntities();
		TArrayView<FMC_AvatarActor> Avatars = QueryIterator.GetArrayView<FMC_AvatarActor>();
		TArrayView<FMC_Viewpoint> Viewpoints = QueryIterator.GetArrayView<FMC_Viewpoint>();
		LoadTraceData(QueryIterator);

		for (int32 EntityIndex = 0; EntityIndex < Entities.Num(); ++EntityIndex)
		{
			FGuid& SourceEntity = Entities[EntityIndex];
			FMC_AvatarActor& Avatar = Avatars[EntityIndex];
			FMC_Viewpoint& Viewpoint = Viewpoints[EntityIndex];
			FMC_ViewpointTrace& TraceOptions = GetTraceOptions(EntityIndex);

			if (FPlatformTime::Seconds() - TraceOptions.LastScanTimeSec < TraceOptions.ScanRateSec)
			{
				continue;
			}
			if (!Viewpoint.IsValid())
			{
				ANANKE_LOG_PERIODIC(Error, TEXT("Invalid viewpoint."), 1.0);
				continue;
			}
			if (FPlatformTime::Seconds() - Viewpoint.LastTimeProcessedSec > TraceOptions.MaxViewpointDataAgeSec)
			{
				ANANKE_LOG_PERIODIC(Error, TEXT("Stale viewpoint data."), 1.0);
				continue;
			}

			FVPTDebugSphereData DebugSphereData;
			DebugSphereData.LifeTime = static_cast<float>(TraceOptions.ScanRateSec * 2);

			TArray<FMC_PerceptionEvent>* EventsToEmit;
			if (Viewpoint.IsPlayerViewpoint())
			{
				EventsToEmit = &PlayerEventsToEmit;
			}
			else
			{
				EventsToEmit = &AIEventsToEmit;
			}

			PerformLineTrace(Ctx, SourceEntity, Avatar, Viewpoint, TraceOptions, DebugSphereData, EventsToEmit);

			if (TraceOptions.bDrawDebugGeometry)
			{
				DrawDebugGeometry(Ctx, DebugSphereData);
			}
		}
	}

	if (PlayerEventsToEmit.Num() > 0)
	{
		auto FilterComponent = FInstancedStruct::Make(FMC_PlayerPerceptionEvent());
		EmitPerceptionEvents(Ctx, PlayerEventsToEmit, FilterComponent);
	}
	if (AIEventsToEmit.Num() > 0)
	{
		auto FilterComponent = FInstancedStruct::Make(FMC_AIPerceptionEvent());
		EmitPerceptionEvents(Ctx, AIEventsToEmit, FilterComponent);
	}
}

void UMO_ViewpointTrace::PerformLineTrace(
	FMantleOperationContext& Ctx,
	FGuid& SourceEntity,
	FMC_AvatarActor& Avatar,
	FMC_Viewpoint& Viewpoint,
	FMC_ViewpointTrace& TraceOptions,
	FVPTDebugSphereData& DebugSphereData,
	TArray<FMC_PerceptionEvent>* OutEvents
)
{
	FVector TraceStartPoint = Viewpoint.Location;
	FRotator TraceStartRotator = Viewpoint.Rotation;
	FVector TraceDirection = TraceStartRotator.Vector();
	FVector TraceEndPoint = TraceStartPoint + (TraceDirection * TraceOptions.ScanRange);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UAnankeFirstPersonCameraTraceComponent_CollectPerceptionData), false);

	if (AActor* AvatarActor = Avatar.GetAvatarActor())
	{
		QueryParams.AddIgnoredActor(AvatarActor);
	}
	
	TArray<FHitResult> TraceResults;
	Ctx.World->LineTraceMultiByChannel(TraceResults, TraceStartPoint, TraceEndPoint, TraceOptions.TraceChannel, QueryParams);
	TraceOptions.LastScanTimeSec = FPlatformTime::Seconds();

	if (TraceResults.Num() > 0)
	{
		DebugSphereData.Center = TraceResults[TraceResults.Num() - 1].Location;
		DebugSphereData.Color = FColor::Yellow;
	}
	else
	{
		DebugSphereData.Center = TraceEndPoint;
		DebugSphereData.Color = FColor::Red;
	}

	bool bFoundValidBlockingHit = false;

	for (FHitResult TraceResult : TraceResults)
	{
		AActor* ResultActor = TraceResult.GetActor();

		if (!ResultActor)
		{
			continue;
		}

		auto* AvatarComponent = Cast<UMantleAvatarComponent>(
			ResultActor->GetComponentByClass(UMantleAvatarComponent::StaticClass()));
		
		if (AvatarComponent && Ctx.MantleDB->HasEntity(AvatarComponent->GetEntityId()))
		{
			FGuid TargetEntity = AvatarComponent->GetEntityId();

			if (!EntityIsValidTarget(Ctx,TargetEntity))
			{
				continue;
			}
			
			if (TraceResult.bBlockingHit)
			{
				DebugSphereData.Color = FColor::Green;
				bFoundValidBlockingHit = true;

				if (
					TraceOptions.BlockingHitRule == EBlockingHitEmission::All ||
					(
						TraceOptions.BlockingHitRule == EBlockingHitEmission::Delta &&
						TargetEntity != TraceOptions.LastBlockingHit.TargetEntity
					)
				)
				{
					TraceOptions.LastBlockingHit = FMC_PerceptionEvent(SourceEntity, TargetEntity, true);
					OutEvents->Add(TraceOptions.LastBlockingHit);
				}
			}
			else if (TraceOptions.OverlapRule == EOverlapEmission::All)
			{
				OutEvents->Add(FMC_PerceptionEvent(SourceEntity, TargetEntity, false));
			}
		}	
	}

	if(!bFoundValidBlockingHit && TraceOptions.LastBlockingHit.HasTarget())
	{
		// We emit a special "no targets" event here. This can be useful for signaling to downstream systems
		// that an entity used to be looking at something but is not anymore.
		TraceOptions.LastBlockingHit = FMC_PerceptionEvent(SourceEntity);
		OutEvents->Add(FMC_PerceptionEvent(TraceOptions.LastBlockingHit));
	}
}

void UMO_ViewpointTrace::EmitPerceptionEvents(
	FMantleOperationContext& Ctx,
	TArray<FMC_PerceptionEvent>& EventsToEmit,
	FInstancedStruct& SourceFilter
)
{
	TArray<FInstancedStruct> EventComponents;
	EventComponents.Add(FInstancedStruct::Make(FMC_PerceptionEvent()));
	EventComponents.Add(FInstancedStruct::Make(FMC_ViewpointTraceEvent()));
	EventComponents.Add(FInstancedStruct::Make(FMC_TemporaryEntity()));
	AddTraceEventTags(EventComponents);
	EventComponents.Add(SourceFilter);

	FMantleIterator ResultIterator = Ctx.MantleDB->AddEntities(EventComponents, EventsToEmit.Num());
	auto DataIterator = EventsToEmit.CreateIterator();
	
	while (ResultIterator.Next() && DataIterator)
	{
		TArrayView<FGuid> Entities = ResultIterator.GetEntities();
		TArrayView<FMC_PerceptionEvent> PerceptionEvents = ResultIterator.GetArrayView<FMC_PerceptionEvent>();

		for (int32 EventIndex = 0; EventIndex < Entities.Num() && DataIterator; ++EventIndex, ++DataIterator)
		{
			PerceptionEvents[EventIndex] = *DataIterator;
		}
	}

	if (DataIterator)
	{
		// sanity check. There should never be any data left in this iterator.
		UE_LOG(LogMantle, Error, TEXT("UMO_ViewpointTrace: DataIterator was not completely consumed."));
	}
}

void UMO_ViewpointTrace::DrawDebugGeometry(FMantleOperationContext& Ctx, FVPTDebugSphereData& DebugSphereData)
{
	DrawDebugSphere(
		Ctx.World.Get(),
		DebugSphereData.Center,
		DebugSphereData.Radius,
		DebugSphereData.Segments,
		DebugSphereData.Color,
		DebugSphereData.bPersistentLines,
		DebugSphereData.LifeTime,
		DebugSphereData.DepthPriority,
		DebugSphereData.Thickness
	);
}

void UMO_ViewpointTrace::AddRequiredTraceComponent(FMantleComponentQuery& Query)
{
	Query.AddRequiredComponent<FMC_ViewpointTrace>();
}

void UMO_ViewpointTrace::LoadTraceData(FMantleIterator& Iterator)
{
	ViewpointTraceConfigs = Iterator.GetArrayView<FMC_ViewpointTrace>();
}

FMC_ViewpointTrace& UMO_ViewpointTrace::GetTraceOptions(int32 EntityIndex)
{
	return ViewpointTraceConfigs[EntityIndex];
}