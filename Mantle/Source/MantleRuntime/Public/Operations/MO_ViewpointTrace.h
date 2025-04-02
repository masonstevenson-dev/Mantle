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
#include "Containers/Array.h"
#include "Foundation/MantleOperation.h"
#include "Foundation/MantleQueries.h"
#include "InstancedStruct.h"
#include "Math/Color.h"
#include "Misc/Guid.h"

#include "MO_ViewpointTrace.generated.h"

struct FMC_PerceptionEvent;
struct FMC_ViewpointTrace;
struct FMC_Viewpoint;
struct FMC_AvatarActor;

/**
 * Args to pass to DrawDebugSphere().
 */
USTRUCT()
struct FVPTDebugSphereData
{
	GENERATED_BODY()
	
	FVector Center = FVector::ZeroVector;
	float Radius = 5.0f;
	int32 Segments = 16;
	FColor Color = FColor::Black;
	bool bPersistentLines = false;
	float LifeTime = -1.0; 
	uint8 DepthPriority = 0; 
	float Thickness = 0.0;
};

/**
 * ViewpointTrace Mantle operation. For each entity, performs a line trace from that entity's "viewpoint".
 *
 * Options such as the scan rate can be configured on a per-entity basis using the MC_ViewpointTrace component.
 *
 * This operation outputs temporary perception event entities that can be consumed by downstream systems. These events
 * are produced when the line trace detects an actor in the world with a MantleAvatarComponent, which indicates that the
 * Actor in question represents some entity in the Mantle engine.
 *
 * Input Entity Composition:
 *   + MC_Avatar
 *   + MC_Viewpoint
 *   + MC_ViewpointTrace
 *   
 * Output Entity Composition:
 *   + MC_PerceptionEvent
 *   + MC_ViewpointTraceEvent
 *   + MC_TemporaryEntity
 *   + MC_PlayerPerceptionEvent OR MC_AIPerceptionEvent
 */
UCLASS()
class MANTLERUNTIME_API UMO_ViewpointTrace : public UMantleOperation
{
	GENERATED_BODY()
	
protected:
	//~UMantleOperation Interface
	virtual void Initialize() override;
	virtual void PerformOperation(FMantleOperationContext& Ctx) override;
	//~End UMantleOperation Interface
	
	void PerformLineTrace(
		FMantleOperationContext& Ctx,
		FGuid& SourceEntity,
		FMC_AvatarActor& Avatar,
		FMC_Viewpoint& Viewpoint,
		FMC_ViewpointTrace& TraceOptions,
		FVPTDebugSphereData& DebugSphereData,
		TArray<FMC_PerceptionEvent>* OutEvents
	);
	void EmitPerceptionEvents(
		FMantleOperationContext& Ctx,
		TArray<FMC_PerceptionEvent>& EventsToEmit,
		FInstancedStruct& SourceFilter
	);
	void DrawDebugGeometry(FMantleOperationContext& Ctx, FVPTDebugSphereData& DebugSphereData);

	//~UMO_ViewpointTrace Interface
	virtual void AddRequiredTraceComponent(FMantleComponentQuery& Query);
	virtual void LoadTraceData(FMantleIterator& Iterator);
	virtual FMC_ViewpointTrace& GetTraceOptions(int32 EntityIndex);
	virtual bool EntityIsValidTarget(FMantleOperationContext& Ctx, FGuid& TargetEntity) { return true; }
	virtual void AddTraceEventTags(TArray<FInstancedStruct>& EventComponents) {}
	//~End EMO_ViewpointTrace Interface
	
private:
	FMantleComponentQuery TraceQuery;
	TArrayView<FMC_ViewpointTrace> ViewpointTraceConfigs;
};
