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

#include "Operations/MO_ViewpointCollector.h"

#include "MantleComponents/MC_Viewpoint.h"
#include "MantleRuntimeLoggingDefs.h"

UMO_ViewpointCollector::UMO_ViewpointCollector(const FObjectInitializer& Initializer)
{
	Query.AddRequiredComponent<FMC_Viewpoint>();
}

void UMO_ViewpointCollector::PerformOperation(FMantleOperationContext& Ctx)
{
	FMantleIterator QueryIterator = Ctx.MantleDB->RunQuery(Query);

	while (QueryIterator.Next())
	{
		TArrayView<FMC_Viewpoint> Viewpoints = QueryIterator.GetArrayView<FMC_Viewpoint>();

		for (FMC_Viewpoint& Viewpoint : Viewpoints)
		{
			AController* SourceController = Viewpoint.GetViewpointSourceController();
			
			if (!SourceController)
			{
				ANANKE_LOG_PERIODIC(Error, TEXT("Invalid SourceController."), 1.0);
				continue;
			}

			// Copy data from the source into the Viewpoint component.
			SourceController->GetPlayerViewPoint(Viewpoint.Location, Viewpoint.Rotation);
			Viewpoint.LastTimeProcessedSec = FPlatformTime::Seconds();
		}
	}
}
