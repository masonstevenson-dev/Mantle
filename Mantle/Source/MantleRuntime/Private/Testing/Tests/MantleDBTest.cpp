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

#include "MantleRuntimeLoggingDefs.h"
#include "Containers/AnankeUntypedArrayView.h"
#include "Containers/Array.h"
#include "Containers/ArrayView.h"
#include "Containers/UnrealString.h"
#include "Foundation/MantleDB.h"
#include "Foundation/MantleQueries.h"
#include "Logging/LogVerbosity.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "Misc/AutomationTest.h"
#include "Testing/Fakes/AnankeTestActor.h"
#include "Testing/Fakes/FakeMantleComponents.h"
#include "Testing/Macros/AnankeTestMacros.h"

#if WITH_EDITOR

class TestSuite
{
public:
	TestSuite(FAutomationTestBase* NewTestFramework): TestFramework(NewTestFramework)
	{
		// This constructor is run before each test.
		ANANKE_LOG(LogMantleTest, Log, TEXT("Setting up test suite."));

		ANANKE_LOG(LogMantleTest, Log, TEXT("Creating test world."));
		TestWorld = TStrongObjectPtr<UWorld>(UWorld::CreateWorld(EWorldType::Game, false));
		UWorld* World = TestWorld.Get();
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
		ANANKE_LOG(LogMantleTest, Log, TEXT("Test world initialized."));

		ANANKE_LOG(LogMantleTest, Log, TEXT("Creating Mantle database."));
		MantleDB = TStrongObjectPtr<UMantleDB>(NewObject<UMantleDB>());
		ANANKE_LOG(LogMantleTest, Log, TEXT("Mantle database created."));
		
	}

	~TestSuite()
	{
		// This destructor is run after each test.
		if (TestWorld.IsValid())
		{
			ANANKE_LOG(LogMantle, Log, TEXT("Destroying test world."));
			UWorld* WorldPtr = TestWorld.Get();
			GEngine->DestroyWorldContext(WorldPtr);
			WorldPtr->DestroyWorld(true);

			MantleDB.Reset();
			TestWorld.Reset();
			
			WorldPtr->MarkAsGarbage();
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			ANANKE_LOG(LogMantle, Log, TEXT("Test world destroyed."));	
		}
		else
		{
			MantleDB.Reset();
			TestWorld.Reset();
		}
	}

	// UTIL FNs -------------------------------------------------------------------------------------------------------
	void InitDB(int32 ChunkSizeBytes = Ananke::Mantle::kDefaultChunkSize)
	{
		TArray<UScriptStruct*> ComponentTypes;
		ComponentTypes.Add(FFakeTransformComponent::StaticStruct());
		TransformComponentName = FFakeTransformComponent::StaticStruct()->GetName();
		TransformComponentBitIndex = 0;
		
		ComponentTypes.Add(FFakeItemComponent::StaticStruct());
		ItemComponentName = FFakeItemComponent::StaticStruct()->GetName();
		ItemComponentBitIndex = 1;
		
		ComponentTypes.Add(FFakeTargetingComponent::StaticStruct());
		TargetingComponentName = FFakeTargetingComponent::StaticStruct()->GetName();
		TargetingComponentBitIndex = 2;

		ComponentTypes.Add(FFakeBigComponent::StaticStruct());
		BigComponentName = FFakeBigComponent::StaticStruct()->GetName();
		BigComponentBitIndex = 3;

		ComponentTypes.Add(FFakeEmptyComponent::StaticStruct());
		EmptyComponentName = FFakeEmptyComponent::StaticStruct()->GetName();
		EmptyComponentBitIndex = 4;

		NumComponents = ComponentTypes.Num();

		MantleDB->Initialize(ComponentTypes, ChunkSizeBytes);
	}

	void InitDBWithEntities(AAnankeTestActor* TargetActor_Archetype2, AAnankeTestActor* TargetActor_Archetype4)
	{
		InitDB(1*1024); // 1kb per chunk.

		int32 NumEntities_Archetype1 = 10; // size = 96*10 ~= .94kb
		int32 NumEntities_Archetype2 = 20; // size = (120*20) ~= 2.34kb
		int32 NumEntities_Archetype3 = 30; // size = (120*30) ~= 3.54kb
		int32 NumEntities_Archetype4 = 40; // size = (144*40) ~= 5.66kb

		// Archetype 1: Transform only
		TArray<FInstancedStruct> Components_Archetype1;
		auto Transform_Archetype1 = FTransform(FVector(1.0f, 1.0f, 1.0f));
		Components_Archetype1.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype1)));
		MantleDB->AddEntities(Components_Archetype1, NumEntities_Archetype1);

		// Archetype 2: Transform + Targeting
		TArray<FInstancedStruct> Components_Archetype2;
		auto Transform_Archetype2 = FTransform(FVector(2.0f, 2.0f, 2.0f));
		Components_Archetype2.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype2)));
		TargetActor_Archetype2->bSomeBoolProperty = true;
		TargetActor_Archetype2->SomeFloatProperty = 2.0f;
		Components_Archetype2.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_Archetype2, TEXT("TheTarget_Archetype2"))));
		MantleDB->AddEntities(Components_Archetype2, NumEntities_Archetype2);

		// Archetype 3: Transform + Item
		TArray<FInstancedStruct> Components_Archetype3;
		auto Transform_Archetype3 = FTransform(FVector(3.0f, 3.0f, 3.0f));
		Components_Archetype3.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype3)));
		Components_Archetype3.Add(FInstancedStruct::Make(FFakeItemComponent(TEXT("ItemName_Archetype3"), 30.0f, 3.0f)));
		MantleDB->AddEntities(Components_Archetype3, NumEntities_Archetype3);
		
		// Archetype 4: Transform + Item + Targeting
		TArray<FInstancedStruct> Components_Archetype4;
		auto Transform_Archetype4 = FTransform(FVector(4.0f, 4.0f, 4.0f));
		Components_Archetype4.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype4)));
		Components_Archetype4.Add(FInstancedStruct::Make(FFakeItemComponent(TEXT("ItemName_Archetype4"), 40.0f, 4.0f)));
		TargetActor_Archetype4->bSomeBoolProperty = false;
		TargetActor_Archetype4->SomeFloatProperty = 4.0f;
		Components_Archetype4.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_Archetype4, TEXT("TheTarget_Archetype4"))));
		MantleDB->AddEntities(Components_Archetype4, NumEntities_Archetype4);
	}

	// Smaller version of DB. Does not include Archetype4.
	void InitDBWithEntities2(AAnankeTestActor* TargetActor_Archetype2)
	{
		InitDB(1*1024); // 1kb per chunk.

		int32 NumEntities_Archetype1 = 10; // size = 96*10 ~= .94kb
		int32 NumEntities_Archetype2 = 20; // size = (120*20) ~= 2.34kb
		int32 NumEntities_Archetype3 = 30; // size = (120*30) ~= 3.54kb

		// Archetype 1: Transform only
		TArray<FInstancedStruct> Components_Archetype1;
		auto Transform_Archetype1 = FTransform(FVector(1.0f, 1.0f, 1.0f));
		Components_Archetype1.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype1)));
		MantleDB->AddEntities(Components_Archetype1, NumEntities_Archetype1);

		// Archetype 2: Transform + Targeting
		TArray<FInstancedStruct> Components_Archetype2;
		auto Transform_Archetype2 = FTransform(FVector(2.0f, 2.0f, 2.0f));
		Components_Archetype2.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype2)));
		TargetActor_Archetype2->bSomeBoolProperty = true;
		TargetActor_Archetype2->SomeFloatProperty = 2.0f;
		Components_Archetype2.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_Archetype2, TEXT("TheTarget_Archetype2"))));
		MantleDB->AddEntities(Components_Archetype2, NumEntities_Archetype2);

		// Archetype 3: Transform + Item
		TArray<FInstancedStruct> Components_Archetype3;
		auto Transform_Archetype3 = FTransform(FVector(3.0f, 3.0f, 3.0f));
		Components_Archetype3.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_Archetype3)));
		Components_Archetype3.Add(FInstancedStruct::Make(FFakeItemComponent(TEXT("ItemName_Archetype3"), 30.0f, 3.0f)));
		MantleDB->AddEntities(Components_Archetype3, NumEntities_Archetype3);
	}

	void ValidateComponentInfo(UScriptStruct* ComponentType, int32 ExpectedArchetypeIndex)
	{
		FString ComponentTypeName = ComponentType->GetName();
		FMantleComponentInfo* StructInfo = MantleDB->MasterRecord.ComponentInfoMap.Find(ComponentTypeName);

		FString ExpectedComponentInfoMsg = FString::Printf(TEXT("ComponentInfo for %s"), *ComponentTypeName);
		if(!TestFramework->TestNotNull(ExpectedComponentInfoMsg, StructInfo))
		{
			return;
		}

		FString ExpectedArchetypeIndexMsg = FString::Printf(TEXT("Archetype index for %s"), *ComponentTypeName);
		TestFramework->TestEqual(ExpectedArchetypeIndexMsg, StructInfo->ArchetypeIndex, ExpectedArchetypeIndex);

		FString ExpectedStructSizeMsg = FString::Printf(TEXT("Struct size for %s"), *ComponentTypeName);
		TestFramework->TestEqual(ExpectedStructSizeMsg, StructInfo->StructSize, ComponentType->GetStructureSize());

		FString ExpectedStructAlignmentMsg = FString::Printf(TEXT("Alignment for %s"), *ComponentTypeName);
		TestFramework->TestEqual(ExpectedStructAlignmentMsg, StructInfo->StructAlignment, ComponentType->GetMinAlignment());
	}
	// END UTIL FNs ---------------------------------------------------------------------------------------------------
	
	void Test_SmokeTest()
	{
		ANANKE_LOG(LogMantleTest, Log, TEXT("Running Test_SmokeTest"));
		ANANKE_TEST_TRUE(TestFramework, MantleDB.IsValid());
	}

	void Test_InitializeDB()
	{
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 0);
		
		InitDB(128*1024);
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->MasterRecord.ComponentInfoMap.Num(), NumComponents);
		ValidateComponentInfo(FFakeTransformComponent::StaticStruct(), 0);
		ValidateComponentInfo(FFakeItemComponent::StaticStruct(), 1);
		ValidateComponentInfo(FFakeTargetingComponent::StaticStruct(), 2);
		ValidateComponentInfo(FFakeBigComponent::StaticStruct(), 3);
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->MasterRecord.ChunkComponentBlobSize, 128*1024);

		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 1);
		TBitArray<> BareArchetype = TBitArray<>(false, NumComponents);
		TSharedPtr<FMantleDBEntry>* BareArchetypeEntry = MantleDB->EntriesByArchetype.Find(BareArchetype);
		ANANKE_TEST_NOT_NULL(TestFramework, BareArchetypeEntry);
		TestFramework->TestTrue(TEXT("BareArchetypeEntry.IsValid()"), (*BareArchetypeEntry).IsValid());
	}

	void Test_DBDoesNotAddComponentTypesIfAlreadyInitialized()
	{
		// Expect warning.
		TestFramework->AddExpectedError(
			TEXT("Attempted to initialize MantleDB but it was already initialized."), EAutomationExpectedErrorFlags::Contains, 1);
		
		TArray<UScriptStruct*> ComponentTypes;
		ComponentTypes.Add(FFakePlaceholderComponent::StaticStruct());
		MantleDB->Initialize(ComponentTypes);
		
		InitDB();
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->MasterRecord.ComponentInfoMap.Num(), 1);
		ValidateComponentInfo(FFakePlaceholderComponent::StaticStruct(), 0);
	}

	void Test_AddEntity()
	{
		InitDB();

		TArray<FInstancedStruct> ComponentsToAdd;

		auto Transform = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ComponentsToAdd.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform)));

		AActor* TargetActor = TestWorld->SpawnActor<AAnankeTestActor>();
		ComponentsToAdd.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor, TEXT("TheTarget"))));

		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 1);
		MantleDB->AddEntity(ComponentsToAdd);
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 2);

		FMantleIterator Result = MantleDB->AddEntityAndIterate(ComponentsToAdd);

		if (!ANANKE_TEST_TRUE(TestFramework, Result.Next()))
		{
			return;
		}
		if (!ANANKE_TEST_EQUAL(TestFramework, Result.GetEntities().Num(), 1))
		{
			return;
		}

		TArrayView<FFakeTransformComponent> TransformView = Result.GetArrayView<FFakeTransformComponent>();
		if (!ANANKE_TEST_EQUAL(TestFramework, TransformView.Num(), 1))
		{
			return;
		}
		TestFramework->TestEqual(
			TEXT("TransformView[0].Transform.Location"),
			TransformView[0].Transform.GetLocation(),
			FVector(10.0f, 20.0f, 30.0f)
		);

		TArrayView<FFakeTargetingComponent> TargetingView = Result.GetArrayView<FFakeTargetingComponent>();
		if (!ANANKE_TEST_EQUAL(TestFramework, TargetingView.Num(), 1))
		{
			return;
		}
		FFakeTargetingComponent ResultTargeting = TargetingView[0];
		ANANKE_TEST_EQUAL(TestFramework, ResultTargeting.Target.Get(), TargetActor);
		ANANKE_TEST_EQUAL(TestFramework, ResultTargeting.TargetName, TEXT("TheTarget"));

		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 2);

		TBitArray<> BareArchetype = TBitArray<>(false, NumComponents);
		TBitArray<> TestArchetype = TBitArray<>(false, NumComponents);
		TestArchetype[TransformComponentBitIndex] = true;
		TestArchetype[TargetingComponentBitIndex] = true;

		TSharedPtr<FMantleDBEntry>* BareArchetypeEntry = MantleDB->EntriesByArchetype.Find(BareArchetype);
		ANANKE_TEST_NOT_NULL(TestFramework, BareArchetypeEntry);
		TestFramework->TestTrue(TEXT("BareArchetypeEntry.IsValid()"), (*BareArchetypeEntry).IsValid());
		ANANKE_TEST_EQUAL(TestFramework, BareArchetypeEntry->Get()->Chunks.Num(), 0);

		TSharedPtr<FMantleDBEntry>* TestArchetypeEntry = MantleDB->EntriesByArchetype.Find(TestArchetype);
		ANANKE_TEST_NOT_NULL(TestFramework, TestArchetypeEntry);
		TestFramework->TestTrue(TEXT("TestArchetypeEntry.IsValid()"), (*TestArchetypeEntry).IsValid());
		ANANKE_TEST_EQUAL(TestFramework, TestArchetypeEntry->Get()->Chunks.Num(), 1);
		ANANKE_TEST_EQUAL(TestFramework, TestArchetypeEntry->Get()->AvailableChunkIds.Num(), 1);
		ANANKE_TEST_EQUAL(TestFramework, TestArchetypeEntry->Get()->AllChunkIds.Num(), 1);

		FMantleDBChunk* TestArchetypeChunk = TestArchetypeEntry->Get()->Chunks.Find(TestArchetypeEntry->Get()->AllChunkIds[0]);
		ANANKE_TEST_NOT_NULL(TestFramework, TestArchetypeChunk);
		ANANKE_TEST_NOT_NULL(TestFramework, TestArchetypeChunk->ComponentBlob);

		// Expected = (total bytes - alignment bytes) / (transform component bytes + targeting component bytes)
		//          = (128*1024 - 16 - 8) / (96 + 24) = 1092
		ANANKE_TEST_EQUAL(TestFramework, TestArchetypeChunk->TotalCapacity, 1092);
		ANANKE_TEST_EQUAL(TestFramework, TestArchetypeChunk->EntityIds.Num(), 2);
		{
			auto* ExtractedTransformComponent = (FFakeTransformComponent*)TestArchetypeChunk->GetComponentInternal(TransformComponentName, 0);
			ANANKE_TEST_NOT_NULL(TestFramework, ExtractedTransformComponent);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTransformComponent->Transform.GetLocation(), FVector(10.0f, 20.0f, 30.0f));
			auto* ExtractedTargetingComponent = (FFakeTargetingComponent*)TestArchetypeChunk->GetComponentInternal(TargetingComponentName, 0);
			ANANKE_TEST_NOT_NULL(TestFramework, ExtractedTargetingComponent);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTargetingComponent->Target.Get(), TargetActor);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTargetingComponent->TargetName, TEXT("TheTarget"));
		}
		{
			auto* ExtractedTransformComponent = (FFakeTransformComponent*)TestArchetypeChunk->GetComponentInternal(TransformComponentName, 1);
			ANANKE_TEST_NOT_NULL(TestFramework, ExtractedTransformComponent);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTransformComponent->Transform.GetLocation(), FVector(10.0f, 20.0f, 30.0f));
			auto* ExtractedTargetingComponent = (FFakeTargetingComponent*)TestArchetypeChunk->GetComponentInternal(TargetingComponentName, 1);
			ANANKE_TEST_NOT_NULL(TestFramework, ExtractedTargetingComponent);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTargetingComponent->Target.Get(), TargetActor);
			ANANKE_TEST_EQUAL(TestFramework, ExtractedTargetingComponent->TargetName, TEXT("TheTarget"));
		}
		
		TestWorld.Get()->EditorDestroyActor(TargetActor, false);
	}
	
	void Test_AddEntities()
	{
		InitDB();

		TArray<FInstancedStruct> ComponentsToAdd;

		auto BigTransform = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ComponentsToAdd.Add(FInstancedStruct::Make(FFakeBigComponent(BigTransform)));

		auto Transform = FTransform(FVector(-5.0f, -10.0f, -20.0f));
		ComponentsToAdd.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform)));

		ComponentsToAdd.Add(FInstancedStruct::Make(FFakeItemComponent(TEXT("ItemName"), 2.5f, 5.99f)));

		// Total archetype size should be = 960 + 96 + 24 = 1080
		// Total alignment bytes should be = 16 + 16 + 8 = 40
		// So capacity = (128*1024 - 40) / (1080) = 121
		//
		// For 200 entities added, we would expect 121 to be in chunk 1 and 79 to be in chunk 2.

		TestFramework->TestEqual(TEXT("EntriesByArchetype.Num()"), MantleDB->EntriesByArchetype.Num(), 1);
		FMantleIterator Result = MantleDB->AddEntities(ComponentsToAdd, 200);
		TestFramework->TestEqual(TEXT("EntriesByArchetype.Num()"), MantleDB->EntriesByArchetype.Num(), 2);

		if(!ANANKE_TEST_EQUAL(TestFramework, Result.LocalCache.MatchingEntries.Num(), 1))
		{
			return;
		}

		FMantleCachedEntry CachedEntry = Result.LocalCache.MatchingEntries[0];

		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedComponents.Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds.Num(), 2);
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[0].Num(), 121);
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[1].Num(), 79);

		// Check entities at the boundaries and a couple in the middle.
		TArray<TArray<int32>> EntitiesToCheck;
		EntitiesToCheck.AddDefaulted(CachedEntry.ChunkedEntityIds.Num());
		EntitiesToCheck[0].Append({0, 40, 80, 120});
		EntitiesToCheck[1].Append({0, 26, 52, 78});

		int32 ExpectedNumChunks = EntitiesToCheck.Num();

		// Have to call once in order make the result indices valid.
		Result.Next();
		
		for (int32 ChunkIndex = 0; ChunkIndex < ExpectedNumChunks; ++ChunkIndex)
		{
			TArrayView<FFakeBigComponent> BCView = Result.GetArrayViewInternal<FFakeBigComponent>(0, ChunkIndex);
			TArrayView<FFakeTransformComponent> TCView = Result.GetArrayViewInternal<FFakeTransformComponent>(0, ChunkIndex);
			TArrayView<FFakeItemComponent> ICView = Result.GetArrayViewInternal<FFakeItemComponent>(0, ChunkIndex);

			for (int32 EntityIndex : EntitiesToCheck[ChunkIndex])
			{
				if (
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < BCView.Num()"), ChunkIndex, EntityIndex), EntityIndex < BCView.Num()) ||
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < TCView.Num()"), ChunkIndex, EntityIndex), EntityIndex < TCView.Num()) ||
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < ICView.Num()"), ChunkIndex, EntityIndex), EntityIndex < ICView.Num())
				)
				{
					continue;
				}
				
				FFakeBigComponent CurrentBC = BCView[EntityIndex];
				FFakeTransformComponent CurrentTC = TCView[EntityIndex];
				FFakeItemComponent CurrentIC = ICView[EntityIndex];
				
				TestFramework->TestEqual(
					FString::Printf(TEXT("CurrentBC[%d][%d].Transform1.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform1.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);
				TestFramework->TestEqual(
				FString::Printf(TEXT("CurrentBC[%d][%d].Transform5.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform5.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);
				TestFramework->TestEqual(
				FString::Printf(TEXT("CurrentBC[%d][%d].Transform10.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform10.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);

				TestFramework->TestEqual(
					FString::Printf(TEXT("CurrentTC[%d][%d].Transform.Location"), ChunkIndex, EntityIndex),
					CurrentTC.Transform.GetLocation(),
					FVector(-5.0f, -10.0f, -20.0f)
				);
				
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Name"), ChunkIndex, EntityIndex), CurrentIC.Name, TEXT("ItemName"));
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Weight"), ChunkIndex, EntityIndex), CurrentIC.Weight, 2.5f);
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Cost"), ChunkIndex, EntityIndex), CurrentIC.Cost, 5.99f);
			}
		}

		Result.Reset();
		int32 ChunksChecked = 0;

		while (Result.Next())
		{
			TArrayView<FFakeBigComponent> BCView = Result.GetArrayView<FFakeBigComponent>();
			TArrayView<FFakeTransformComponent> TCView = Result.GetArrayView<FFakeTransformComponent>();
			TArrayView<FFakeItemComponent> ICView = Result.GetArrayView<FFakeItemComponent>();

			int32 ChunkIndex = ChunksChecked;

			for (int32 EntityIndex : EntitiesToCheck[ChunkIndex])
			{
				if (
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < BCView.Num()"), ChunkIndex, EntityIndex), EntityIndex < BCView.Num()) ||
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < TCView.Num()"), ChunkIndex, EntityIndex), EntityIndex < TCView.Num()) ||
					!TestFramework->TestTrue(FString::Printf(TEXT("EntityIndex[%d][%d] < ICView.Num()"), ChunkIndex, EntityIndex), EntityIndex < ICView.Num())
				)
				{
					continue;
				}
				
				FFakeBigComponent CurrentBC = BCView[EntityIndex];
				FFakeTransformComponent CurrentTC = TCView[EntityIndex];
				FFakeItemComponent CurrentIC = ICView[EntityIndex];
				
				TestFramework->TestEqual(
					FString::Printf(TEXT("CurrentBC[%d][%d].Transform1.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform1.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);
				TestFramework->TestEqual(
				FString::Printf(TEXT("CurrentBC[%d][%d].Transform5.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform5.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);
				TestFramework->TestEqual(
				FString::Printf(TEXT("CurrentBC[%d][%d].Transform10.Location"), ChunkIndex, EntityIndex),
					CurrentBC.Transform10.GetLocation(),
					FVector(10.0f, 20.0f, 30.0f)
				);

				TestFramework->TestEqual(
					FString::Printf(TEXT("CurrentTC[%d][%d].Transform.Location"), ChunkIndex, EntityIndex),
					CurrentTC.Transform.GetLocation(),
					FVector(-5.0f, -10.0f, -20.0f)
				);
				
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Name"), ChunkIndex, EntityIndex), CurrentIC.Name, TEXT("ItemName"));
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Weight"), ChunkIndex, EntityIndex), CurrentIC.Weight, 2.5f);
				TestFramework->TestEqual(FString::Printf(TEXT("CurrentIC[%d][%d].Cost"), ChunkIndex, EntityIndex), CurrentIC.Cost, 5.99f);
			}

			ChunksChecked++;
		}

		ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, ExpectedNumChunks);
	}

	void Test_AddBareEntities()
	{
		InitDB(1*1024); // 1kb per chunk.

		TArray<FInstancedStruct> Empty;
		FMantleIterator Result = MantleDB->AddEntities(Empty, 1100);

		if (!ANANKE_TEST_EQUAL(TestFramework, Result.LocalCache.MatchingEntries.Num(), 1))
		{
			return;
		}
		FMantleCachedEntry CachedEntry = Result.LocalCache.MatchingEntries[0];
		ANANKE_TEST_TRUE(TestFramework, CachedEntry.ChunkedComponents.IsEmpty());
		
		if (!ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[0].Num(), 1100);
		
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 1);

		TBitArray<> BareArchetype = TBitArray<>(false, NumComponents);
		TSharedPtr<FMantleDBEntry>* BareArchetypeEntryPtr = MantleDB->EntriesByArchetype.Find(BareArchetype);

		if (!ANANKE_TEST_TRUE(TestFramework, BareArchetypeEntryPtr && BareArchetypeEntryPtr->IsValid()))
		{
			return;
		}
		
		FMantleDBEntry* Entry = BareArchetypeEntryPtr->Get();

		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Num(), 1);
		if (!ANANKE_TEST_EQUAL(TestFramework, Entry->AllChunkIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Find(Entry->AllChunkIds[0])->EntityIds.Num(), 1100);
	}
	
	void Test_SingleArchetypeQuery()
	{
		AAnankeTestActor* TargetActor_Archetype2 = TestWorld->SpawnActor<AAnankeTestActor>();
		AAnankeTestActor* TargetActor_Archetype4 = TestWorld->SpawnActor<AAnankeTestActor>();
		InitDBWithEntities(TargetActor_Archetype2, TargetActor_Archetype4);

		FMantleComponentQuery Query;

		Query.AddRequiredComponent<FFakeItemComponent>();
		Query.AddRequiredComponent<FFakeTargetingComponent>();
		Query.AddRequiredComponent<FFakeTransformComponent>();

		FMantleIterator Result = MantleDB->RunQuery(Query);
		int32 ChunksChecked = 0;

		while(Result.Next())
		{
			TArrayView<FFakeItemComponent> ItemComponents = Result.GetArrayView<FFakeItemComponent>();
			TArrayView<FFakeTargetingComponent> TargetingComponents = Result.GetArrayView<FFakeTargetingComponent>();
			TArrayView<FFakeTransformComponent> TransformComponents = Result.GetArrayView<FFakeTransformComponent>();

			TArrayView<FGuid> EntityIds = Result.GetEntities();
			
			bool bAllEqual = (
				EntityIds.Num() == ItemComponents.Num() &&
				ItemComponents.Num() == TargetingComponents.Num() &&
				TargetingComponents.Num() == TransformComponents.Num()
			);
			if(!TestFramework->TestTrue(TEXT("Result.NumEntities() == ItemComponents.Num == TargetingComponents.Num == TransformComponents.Num"), bAllEqual))
			{
				continue;
			}

			for (int EntityIndex = 0; EntityIndex < EntityIds.Num(); ++EntityIndex)
			{
				FFakeItemComponent ItemComponent = ItemComponents[EntityIndex];
				FFakeTargetingComponent TargetingComponent = TargetingComponents[EntityIndex];
				FFakeTransformComponent TransformComponent = TransformComponents[EntityIndex];

				TestFramework->TestEqual(
					FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
					TransformComponent.Transform.GetLocation(),
					FVector(4.0f, 4.0f, 4.0f)
				);

				TestFramework->TestEqual(TEXT("ItemComponent.Name"), ItemComponent.Name, TEXT("ItemName_Archetype4"));
				TestFramework->TestEqual(TEXT("ItemComponent.Cost"), ItemComponent.Cost, 4.0f);
				TestFramework->TestEqual(TEXT("ItemComponent.Weight"), ItemComponent.Weight, 40.0f);
				
				TestFramework->TestEqual(TEXT("TargetingComponent.TargetName"), TargetingComponent.TargetName, TEXT("TheTarget_Archetype4"));
				AAnankeTestActor* TargetActor = Cast<AAnankeTestActor>(TargetingComponent.Target);
				if (!TestFramework->TestNotNull(TEXT("TargetActor"), TargetActor))
				{
					continue;
				}
				TestFramework->TestEqual(TEXT("TargetActor->bSomeBoolProperty"), TargetActor->bSomeBoolProperty, false);
				TestFramework->TestEqual(TEXT("TargetActor->SomeFloatProperty"), TargetActor->SomeFloatProperty, 4.0f);
			}

			ChunksChecked++;
		}
		
		ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, 7);

		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype4, false);
	}

	void Test_SingleArchetypeQueryNoResults()
	{
		// Sanity check to make sure that running a query for an archetype with no associated entities doesn't blow
		// anything up.
		
		AAnankeTestActor* TargetActor_Archetype2 = TestWorld->SpawnActor<AAnankeTestActor>();
		AAnankeTestActor* TargetActor_Archetype4 = TestWorld->SpawnActor<AAnankeTestActor>();
		InitDBWithEntities(TargetActor_Archetype2, TargetActor_Archetype4);

		FMantleComponentQuery Query;
		Query.AddRequiredComponent<FFakeEmptyComponent>();

		FMantleIterator Result = MantleDB->RunQuery(Query);
		if (!ANANKE_TEST_TRUE(TestFramework, Result.IsValid()))
		{
			return;
		}
		ANANKE_TEST_FALSE(TestFramework, Result.Next());
		
		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype4, false);
	}

	void Test_MultiArchetypeQuery()
	{
		AAnankeTestActor* TargetActor_Archetype2 = TestWorld->SpawnActor<AAnankeTestActor>();
		AAnankeTestActor* TargetActor_Archetype4 = TestWorld->SpawnActor<AAnankeTestActor>();
		InitDBWithEntities(TargetActor_Archetype2, TargetActor_Archetype4);

		FMantleComponentQuery Query;
		Query.AddRequiredComponent<FFakeTransformComponent>();
		Query.AddRequiredComponent<FFakeItemComponent>();

		FMantleIterator Result = MantleDB->RunQuery(Query);
		TArray<TArray<int32>> ExpectedCounts;
		ExpectedCounts.AddDefaulted(2);
		ExpectedCounts[0].Append({
			8, // chunk 0: Archetype3
			8, // chunk 1: Archetype3
			8, // chunk 2: Archetype3
			6  // chunk 3: Archetype3
		});
		ExpectedCounts[1].Append({
			6, // chunk 4: Archetype4
			6, // chunk 5: Archetype4
			6, // chunk 6: Archetype4
			6, // chunk 7: Archetype4
			6, // chunk 8: Archetype4
			6, // chunk 9: Archetype4
			4  // chunk 10: Archetype4
		});

		if(!ANANKE_TEST_EQUAL(TestFramework, Result.LocalCache.MatchingEntries.Num(), ExpectedCounts.Num()))
		{
			return;
		}

		for (int32 EntryIndex = 0; EntryIndex < ExpectedCounts.Num(); ++EntryIndex)
		{
			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedCounts[EntryIndex].Num(); ++ChunkIndex)
			{
				TestFramework->TestEqual(
					FString::Printf(TEXT("Result.NumEntities[%d][%d]"), EntryIndex, ChunkIndex),
					Result.LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds[ChunkIndex].Num(),
					ExpectedCounts[EntryIndex][ChunkIndex]
				);
			}
		}

		int32 ChunksChecked = 0;
		
		while (Result.Next())
		{
			TArrayView<FFakeTransformComponent> TransformComponents = Result.GetArrayView<FFakeTransformComponent>();
			TArrayView<FFakeItemComponent> ItemComponents = Result.GetArrayView<FFakeItemComponent>();
			
			if(!TestFramework->TestTrue(TEXT("TransformComponents.Num() == ItemComponents.Num()"), TransformComponents.Num() == ItemComponents.Num()))
			{
				continue;
			}

			for (int EntityIndex = 0; EntityIndex < TransformComponents.Num(); ++EntityIndex)
			{
				FFakeItemComponent ItemComponent = ItemComponents[EntityIndex];
				FFakeTransformComponent TransformComponent = TransformComponents[EntityIndex];
				
				if (ChunksChecked < 4) // The first 4 chunks should be Archetype3
				{
					TestFramework->TestEqual(
						FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
						TransformComponent.Transform.GetLocation(),
						FVector(3.0f, 3.0f, 3.0f)
					);

					TestFramework->TestEqual(TEXT("ItemComponent.Name"), ItemComponent.Name, TEXT("ItemName_Archetype3"));
					TestFramework->TestEqual(TEXT("ItemComponent.Cost"), ItemComponent.Cost, 3.0f);
					TestFramework->TestEqual(TEXT("ItemComponent.Weight"), ItemComponent.Weight, 30.0f);
				}
				else
				{
					TestFramework->TestEqual(
						FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
						TransformComponent.Transform.GetLocation(),
						FVector(4.0f, 4.0f, 4.0f)
					);

					TestFramework->TestEqual(TEXT("ItemComponent.Name"), ItemComponent.Name, TEXT("ItemName_Archetype4"));
					TestFramework->TestEqual(TEXT("ItemComponent.Cost"), ItemComponent.Cost, 4.0f);
					TestFramework->TestEqual(TEXT("ItemComponent.Weight"), ItemComponent.Weight, 40.0f);
				}
			}

			ChunksChecked++;
		}

		ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, 11);

		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype4, false);
	}

	void Test_EmptyComponentQuery()
	{
		// Sanity check to make sure that component structs with no members work.

		InitDB();

		TArray<FInstancedStruct> Archetype_1;
		Archetype_1.Add(FInstancedStruct::Make(FFakeEmptyComponent()));

		TArray<FInstancedStruct> Archetype_2;
		auto Transform = FTransform(FVector(2.0f, 2.0f, 2.0f));
		Archetype_2.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform)));
		Archetype_2.Add(FInstancedStruct::Make(FFakeEmptyComponent()));

		MantleDB->AddEntities(Archetype_1, 5);
		MantleDB->AddEntities(Archetype_2, 5);

		FMantleComponentQuery Query;
		Query.AddRequiredComponent<FFakeEmptyComponent>();

		FMantleIterator Result = MantleDB->RunQuery(Query);

		if (!ANANKE_TEST_TRUE(TestFramework, Result.IsValid()))
		{
			return;
		}

		int32 EntitiesDiscovered = 0;
		int32 EmptyComponentsProcessed = 0;

		while (Result.Next())
		{
			TArrayView<FGuid> Entities = Result.GetEntities();
			EntitiesDiscovered += Entities.Num();

			TArrayView<FFakeEmptyComponent> EmptyComponents = Result.GetArrayView<FFakeEmptyComponent>();
			EmptyComponentsProcessed += EmptyComponents.Num();
		}

		ANANKE_TEST_EQUAL(TestFramework, EntitiesDiscovered, 10);
		ANANKE_TEST_EQUAL(TestFramework, EmptyComponentsProcessed, 10);
	}
	
	void Test_GetComponent()
	{
		InitDB();

		TArray<FInstancedStruct> ComponentsToAdd_1;
		auto Transform_1 = FTransform(FVector(1.0f, 1.0f, 1.0f));
		ComponentsToAdd_1.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_1)));
		AActor* TargetActor_1 = TestWorld->SpawnActor<AAnankeTestActor>();
		ComponentsToAdd_1.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_1, TEXT("TheTarget_1"))));

		TArray<FInstancedStruct> ComponentsToAdd_2;
		auto Transform_2 = FTransform(FVector(2.0f, 2.0f, 2.0f));
		ComponentsToAdd_2.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_2)));
		AActor* TargetActor_2 = TestWorld->SpawnActor<AAnankeTestActor>();
		ComponentsToAdd_2.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_2, TEXT("TheTarget_2"))));

		TArray<FInstancedStruct> ComponentsToAdd_3;
		auto Transform_3 = FTransform(FVector(3.0f, 3.0f, 3.0f));
		ComponentsToAdd_3.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_3)));
		AActor* TargetActor_3 = TestWorld->SpawnActor<AAnankeTestActor>();
		ComponentsToAdd_3.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_3, TEXT("TheTarget_3"))));

		MantleDB->AddEntity(ComponentsToAdd_1);
		MantleDB->AddEntity(ComponentsToAdd_2);
		FMantleIterator Result = MantleDB->AddEntityAndIterate(ComponentsToAdd_3);
		if (!ANANKE_TEST_TRUE(TestFramework, Result.Next()))
		{
			return;
		}
		if (!ANANKE_TEST_EQUAL(TestFramework, Result.GetEntities().Num(), 1))
		{
			return;
		}

		FGuid EntityId = Result.GetEntities()[0];

		// TODO(): Make a new test to make sure that the result is properly invalidated.
		// MantleDB->AddEntity(ComponentsToAdd_1);
		// MantleDB->AddEntity(ComponentsToAdd_1);

		auto* ResultTransform = MantleDB->GetComponent<FFakeTransformComponent>(EntityId);
		auto* ResultTargeting = MantleDB->GetComponent<FFakeTargetingComponent>(EntityId);
		
		ANANKE_TEST_NOT_NULL(TestFramework, ResultTransform);
		TestFramework->TestEqual(
			TEXT("ResultTransform.Transform.Location"),
			ResultTransform->Transform.GetLocation(),
			FVector(3.0f, 3.0f, 3.0f)
		);
		ANANKE_TEST_NOT_NULL(TestFramework, ResultTargeting);
		TestFramework->TestEqual(TEXT("ResultTargeting.Target"), ResultTargeting->Target.Get(), TargetActor_3);
		TestFramework->TestEqual(TEXT("ResultTargeting.TargetName"), ResultTargeting->TargetName, TEXT("TheTarget_3"));

		TestWorld.Get()->EditorDestroyActor(TargetActor_1, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_3, false);
	}

	void Test_RemoveEntities()
	{
		AAnankeTestActor* TargetActor_Archetype2 = TestWorld->SpawnActor<AAnankeTestActor>();
		AAnankeTestActor* TargetActor_Archetype4 = TestWorld->SpawnActor<AAnankeTestActor>();
		InitDBWithEntities(TargetActor_Archetype2, TargetActor_Archetype4);

		FMantleComponentQuery Query;
		Query.AddRequiredComponent<FFakeTransformComponent>();
		Query.AddRequiredComponent<FFakeItemComponent>();

		FMantleIterator ResultBefore = MantleDB->RunQuery(Query);
		TArray<TArray<int32>> ExpectedCountsBeforeRemoval;
		ExpectedCountsBeforeRemoval.AddDefaulted(2);
		ExpectedCountsBeforeRemoval[0].Append({
			8, // chunk 0: Archetype3
			8, // chunk 1: Archetype3
			8, // chunk 2: Archetype3
			6  // chunk 3: Archetype3
		});
		ExpectedCountsBeforeRemoval[1].Append({
			6, // chunk 0: Archetype4
			6, // chunk 1: Archetype4
			6, // chunk 2: Archetype4
			6, // chunk 3: Archetype4
			6, // chunk 4: Archetype4
			6, // chunk 5: Archetype4
			4  // chunk 6: Archetype4
		});

		if(!ANANKE_TEST_EQUAL(TestFramework, ResultBefore.LocalCache.MatchingEntries.Num(), ExpectedCountsBeforeRemoval.Num()))
		{
			return;
		}

		for (int32 EntryIndex = 0; EntryIndex < ExpectedCountsBeforeRemoval.Num(); ++EntryIndex)
		{
			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedCountsBeforeRemoval[EntryIndex].Num(); ++ChunkIndex)
			{
				TestFramework->TestEqual(
					FString::Printf(TEXT("Result.NumEntities[%d][%d]"), EntryIndex, ChunkIndex),
					ResultBefore.LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds[ChunkIndex].Num(),
					ExpectedCountsBeforeRemoval[EntryIndex][ChunkIndex]
				);
			}
		}

		TArray<FGuid> EntitiesToRemove;

		// Remove half of all the entities for archetype 1.
		for (int32 ChunkIndex = 0; ChunkIndex < ExpectedCountsBeforeRemoval[0].Num(); ++ChunkIndex)
		{
			TArrayView<FGuid> EntityIdChunk = ResultBefore.LocalCache.MatchingEntries[0].ChunkedEntityIds[ChunkIndex];
			
			for (int32 EntityIndex = 0; EntityIndex < EntityIdChunk.Num() / 2; ++EntityIndex)
			{
				EntitiesToRemove.Add(EntityIdChunk[EntityIndex]);
			}
		}

		{
			TArrayView<FGuid> EntityIdChunk = ResultBefore.LocalCache.MatchingEntries[1].ChunkedEntityIds[2];
			// Remove all the entities in entry[1]chunk[2]
			for (FGuid EntityId : EntityIdChunk)
			{
				EntitiesToRemove.Add(EntityId);
			}
		}

		MantleDB->RemoveEntities(EntitiesToRemove);

		FMantleIterator ResultAfter = MantleDB->RunQuery(Query);
		TArray<TArray<int32>> ExpectedCountsAfterRemoval;
		ExpectedCountsAfterRemoval.AddDefaulted(2);
		ExpectedCountsAfterRemoval[0].Append({
			4, // chunk 0: Archetype3
			4, // chunk 1: Archetype3
			4, // chunk 2: Archetype3
			3  // chunk 3: Archetype3
		});
		ExpectedCountsAfterRemoval[1].Append({
			6, // chunk 0: Archetype4
			6, // chunk 1: Archetype4
			// 6, // chunk 2: Archetype4 (deleted)
			6, // chunk 3: Archetype4
			6, // chunk 4: Archetype4
			6, // chunk 5: Archetype4
			4  // chunk 6: Archetype4
		});

		if(!ANANKE_TEST_EQUAL(TestFramework, ResultAfter.LocalCache.MatchingEntries.Num(), ExpectedCountsAfterRemoval.Num()))
		{
			return;
		}

		for (int32 EntryIndex = 0; EntryIndex < ExpectedCountsAfterRemoval.Num(); ++EntryIndex)
		{
			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedCountsAfterRemoval[EntryIndex].Num(); ++ChunkIndex)
			{
				TestFramework->TestEqual(
					FString::Printf(TEXT("Result.NumEntities[%d][%d]"), EntryIndex, ChunkIndex),
					ResultAfter.LocalCache.MatchingEntries[EntryIndex].ChunkedEntityIds[ChunkIndex].Num(),
					ExpectedCountsAfterRemoval[EntryIndex][ChunkIndex]
				);
			}
		}

		int32 ChunksChecked = 0;
		
		while (ResultAfter.Next())
		{
			TArrayView<FFakeTransformComponent> TransformComponents = ResultAfter.GetArrayView<FFakeTransformComponent>();
			TArrayView<FFakeItemComponent> ItemComponents = ResultAfter.GetArrayView<FFakeItemComponent>();
			
			if(!TestFramework->TestTrue(TEXT("TransformComponents.Num() == ItemComponents.Num()"), TransformComponents.Num() == ItemComponents.Num()))
			{
				continue;
			}

			for (int EntityIndex = 0; EntityIndex < TransformComponents.Num(); ++EntityIndex)
			{
				FFakeItemComponent ItemComponent = ItemComponents[EntityIndex];
				FFakeTransformComponent TransformComponent = TransformComponents[EntityIndex];
				
				if (ChunksChecked < 4) // The first 4 chunks should be Archetype3
					{
					TestFramework->TestEqual(
						FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
						TransformComponent.Transform.GetLocation(),
						FVector(3.0f, 3.0f, 3.0f)
					);

					TestFramework->TestEqual(TEXT("ItemComponent.Name"), ItemComponent.Name, TEXT("ItemName_Archetype3"));
					TestFramework->TestEqual(TEXT("ItemComponent.Cost"), ItemComponent.Cost, 3.0f);
					TestFramework->TestEqual(TEXT("ItemComponent.Weight"), ItemComponent.Weight, 30.0f);
					}
				else
				{
					TestFramework->TestEqual(
						FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
						TransformComponent.Transform.GetLocation(),
						FVector(4.0f, 4.0f, 4.0f)
					);

					TestFramework->TestEqual(TEXT("ItemComponent.Name"), ItemComponent.Name, TEXT("ItemName_Archetype4"));
					TestFramework->TestEqual(TEXT("ItemComponent.Cost"), ItemComponent.Cost, 4.0f);
					TestFramework->TestEqual(TEXT("ItemComponent.Weight"), ItemComponent.Weight, 40.0f);
				}
			}

			ChunksChecked++;
		}

		ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, 10);

		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype4, false);
	}

	void Test_DBModificationInvalidatesIterator()
	{
		InitDB();

		TArray<FInstancedStruct> ComponentsToAdd_1;
		auto Transform_1 = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ComponentsToAdd_1.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_1)));

		MantleDB->AddEntity(ComponentsToAdd_1);
		MantleDB->AddEntity(ComponentsToAdd_1);
		FMantleIterator IteratorResult = MantleDB->AddEntityAndIterate(ComponentsToAdd_1);
		ANANKE_TEST_TRUE(TestFramework, IteratorResult.IsValid());
		
		FMantleComponentQuery Query;
		Query.AddRequiredComponent<FFakeTransformComponent>();
		FMantleIterator QueryResult = MantleDB->RunQuery(Query);
		ANANKE_TEST_TRUE(TestFramework, QueryResult.IsValid());

		// Sanity check: running a query should not invalidate IteratorResult
		ANANKE_TEST_TRUE(TestFramework, IteratorResult.IsValid());
		
		MantleDB->AddEntity(ComponentsToAdd_1);
		ANANKE_TEST_FALSE(TestFramework, QueryResult.IsValid());
		ANANKE_TEST_FALSE(TestFramework, IteratorResult.IsValid());
	}

	void Test_UpdateEntities()
	{
		AAnankeTestActor* TargetActor_Archetype2 = TestWorld->SpawnActor<AAnankeTestActor>();

		AAnankeTestActor* TargetActor_UpdatedArchetype = TestWorld->SpawnActor<AAnankeTestActor>();
		TargetActor_UpdatedArchetype->bSomeBoolProperty = false;
		TargetActor_UpdatedArchetype->SomeFloatProperty = 999.9f;
		
		InitDBWithEntities2(TargetActor_Archetype2);

		FMantleComponentQuery Archetype2Query;
		Archetype2Query.AddRequiredComponent<FFakeTransformComponent>();
		Archetype2Query.AddRequiredComponent<FFakeTargetingComponent>();
		
		FMantleComponentQuery Archetype3Query;
		Archetype3Query.AddRequiredComponent<FFakeTransformComponent>();
		Archetype3Query.AddRequiredComponent<FFakeItemComponent>();

		TArray<FGuid> EntitiesToUpdate;

		// PART 1: Check current state of Archetype3 and fill EntitiesToUpdate.
		{
			FMantleIterator Archetype3ResultBefore = MantleDB->RunQuery(Archetype3Query);
			TArray<int32> ExpectedArchetype3CountsBeforeUpdate;
			ExpectedArchetype3CountsBeforeUpdate.Append({
				8, // chunk 0
				8, // chunk 1
				8, // chunk 2
				6  // chunk 3
			});

			if(!ANANKE_TEST_EQUAL(TestFramework, Archetype3ResultBefore.LocalCache.MatchingEntries.Num(), 1))
			{
				return;
			}

			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedArchetype3CountsBeforeUpdate.Num(); ++ChunkIndex)
			{
				TestFramework->TestEqual(
					FString::Printf(TEXT("Result.NumEntities[%d][%d]"), 0, ChunkIndex),
					Archetype3ResultBefore.LocalCache.MatchingEntries[0].ChunkedEntityIds[ChunkIndex].Num(),
					ExpectedArchetype3CountsBeforeUpdate[ChunkIndex]
				);
			}

			// Update every other entity in Archetype 3.
			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedArchetype3CountsBeforeUpdate.Num(); ++ChunkIndex)
			{
				TArrayView<FGuid> EntityIdChunk = Archetype3ResultBefore.LocalCache.MatchingEntries[0].ChunkedEntityIds[ChunkIndex];
			
				for (int32 EntityIndex = 0; EntityIndex < EntityIdChunk.Num(); EntityIndex += 2)
				{
					EntitiesToUpdate.Add(EntityIdChunk[EntityIndex]);
				}
			}
		}

		// PART 2: Update the DB and validate the iterator results.
		{
			TArray<FInstancedStruct> ToAdd;
			TArray<UScriptStruct*> ToRemove;
			ToAdd.Add(FInstancedStruct::Make(FFakeTargetingComponent(TargetActor_UpdatedArchetype, TEXT("TheTarget_UpdatedArchetype"))));
			ToRemove.Add(FFakeItemComponent::StaticStruct());

			// Update from Transform+Item (archetype3) -> Transform+Target (archetype2)
			FMantleIterator UpdateResult = MantleDB->UpdateEntities(EntitiesToUpdate, ToAdd, ToRemove);

			if (!ANANKE_TEST_EQUAL(TestFramework, UpdateResult.LocalCache.MatchingEntries.Num(), 1))
			{
				return;
			}

			FMantleCachedEntry CachedEntry = UpdateResult.LocalCache.MatchingEntries[0];

			ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedComponents.Num(), 2);
			ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds.Num(), 3);
			ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[0].Num(), 4); // 15 total entities
			ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[1].Num(), 8);
			ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[2].Num(), 3);

			int32 ChunksChecked = 0;
			int32 EntitiesChecked = 0;

			while (UpdateResult.Next())
			{
				TArrayView<FFakeTransformComponent> TransformView = UpdateResult.GetArrayView<FFakeTransformComponent>();
				TArrayView<FFakeTargetingComponent> TargetView = UpdateResult.GetArrayView<FFakeTargetingComponent>();

				for (int EntityIndex = 0; EntityIndex < TransformView.Num(); ++EntityIndex)
				{
					FFakeTransformComponent TransformComponent = TransformView[EntityIndex];
					FFakeTargetingComponent TargetingComponent = TargetView[EntityIndex];
				
					TestFramework->TestEqual(
						FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
						TransformComponent.Transform.GetLocation(),
						FVector(3.0f, 3.0f, 3.0f)
					);

					ANANKE_TEST_EQUAL(TestFramework, TargetingComponent.TargetName, TEXT("TheTarget_UpdatedArchetype"));
					AAnankeTestActor* TargetActor = Cast<AAnankeTestActor>(TargetingComponent.Target);
					if (!TestFramework->TestNotNull(TEXT("TargetActor"), TargetActor))
					{
						continue;
					}
					ANANKE_TEST_EQUAL(TestFramework, TargetActor->bSomeBoolProperty, false);
					ANANKE_TEST_EQUAL(TestFramework, TargetActor->SomeFloatProperty, 999.9f);

					EntitiesChecked++;
				}

				ChunksChecked++;
			}

			ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, 3);
			ANANKE_TEST_EQUAL(TestFramework, EntitiesChecked, 15);
		}

		// PART 3: Validate that Archetype2 now contains all its original entities + half of the entities from Archetype3
		{
			FMantleIterator Archetype2Result = MantleDB->RunQuery(Archetype2Query);

			if(!ANANKE_TEST_EQUAL(TestFramework, Archetype2Result.LocalCache.MatchingEntries.Num(), 1))
			{
				return;
			}

			int32 ChunksChecked = 0;
			int32 EntitiesChecked = 0;

			while (Archetype2Result.Next())
			{
				TArrayView<FFakeTransformComponent> TransformView = Archetype2Result.GetArrayView<FFakeTransformComponent>();
				TArrayView<FFakeTargetingComponent> TargetView = Archetype2Result.GetArrayView<FFakeTargetingComponent>();

				for (int EntityIndex = 0; EntityIndex < TransformView.Num(); ++EntityIndex)
				{
					FFakeTransformComponent TransformComponent = TransformView[EntityIndex];
					FFakeTargetingComponent TargetingComponent = TargetView[EntityIndex];

					// The first 20 entities should have the non-updated value.
					if (EntitiesChecked < 20)
					{
						TestFramework->TestEqual(
							FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
							TransformComponent.Transform.GetLocation(),
							FVector(2.0f, 2.0f, 2.0f)
						);
						
						ANANKE_TEST_EQUAL(TestFramework, TargetingComponent.TargetName, TEXT("TheTarget_Archetype2"));
						AAnankeTestActor* TargetActor = Cast<AAnankeTestActor>(TargetingComponent.Target);
						if (!TestFramework->TestNotNull(TEXT("TargetActor"), TargetActor))
						{
							continue;
						}
						ANANKE_TEST_EQUAL(TestFramework, TargetActor->bSomeBoolProperty, true);
						ANANKE_TEST_EQUAL(TestFramework, TargetActor->SomeFloatProperty, 2.0f);
					}
					else // The last 15 should have the updated value
					{
						TestFramework->TestEqual(
							FString::Printf(TEXT("TransformComponent[%d][%d].Transform.Location"), ChunksChecked, EntityIndex),
							TransformComponent.Transform.GetLocation(),
							FVector(3.0f, 3.0f, 3.0f)
						);
						
						ANANKE_TEST_EQUAL(TestFramework, TargetingComponent.TargetName, TEXT("TheTarget_UpdatedArchetype"));
						AAnankeTestActor* TargetActor = Cast<AAnankeTestActor>(TargetingComponent.Target);
						if (!TestFramework->TestNotNull(TEXT("TargetActor"), TargetActor))
						{
							continue;
						}
						ANANKE_TEST_EQUAL(TestFramework, TargetActor->bSomeBoolProperty, false);
						ANANKE_TEST_EQUAL(TestFramework, TargetActor->SomeFloatProperty, 999.9f);
					}
					EntitiesChecked++;
				}
				ChunksChecked++;
			}
			
			ANANKE_TEST_EQUAL(TestFramework, ChunksChecked, 5);
			ANANKE_TEST_EQUAL(TestFramework, EntitiesChecked, 35);
		}

		// PART 4: Validate that entities were correctly removed from Archetype 3.
		{
			FMantleIterator Archetype3ResultAfter = MantleDB->RunQuery(Archetype3Query);

			if(!ANANKE_TEST_EQUAL(TestFramework, Archetype3ResultAfter.LocalCache.MatchingEntries.Num(), 1))
			{
				return;
			}

			// The old archetype should have half of its entities removed.
			TArray<int32> ExpectedArchetype3CountsAfterUpdate;
			ExpectedArchetype3CountsAfterUpdate.Append({
				4, // chunk 0
				4, // chunk 1
				4, // chunk 2
				3  // chunk 3
			});

			for (int32 ChunkIndex = 0; ChunkIndex < ExpectedArchetype3CountsAfterUpdate.Num(); ++ChunkIndex)
			{
				TestFramework->TestEqual(
					FString::Printf(TEXT("Result.NumEntities[%d][%d]"), 0, ChunkIndex),
					Archetype3ResultAfter.LocalCache.MatchingEntries[0].ChunkedEntityIds[ChunkIndex].Num(),
					ExpectedArchetype3CountsAfterUpdate[ChunkIndex]
				);
			}
		}

		TestWorld.Get()->EditorDestroyActor(TargetActor_Archetype2, false);
		TestWorld.Get()->EditorDestroyActor(TargetActor_UpdatedArchetype, false);
	}

	void ErrorTest_GetUnknownArrayView()
	{
		InitDB();

		TArray<FInstancedStruct> ComponentsToAdd_1;
		auto Transform_1 = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ComponentsToAdd_1.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform_1)));

		MantleDB->AddEntity(ComponentsToAdd_1);
		MantleDB->AddEntity(ComponentsToAdd_1);
		FMantleIterator Result = MantleDB->AddEntityAndIterate(ComponentsToAdd_1);
		ANANKE_TEST_TRUE(TestFramework, Result.IsValid());
		
		if(!ANANKE_TEST_TRUE(TestFramework, Result.Next()))
		{
			return;
		}

		TestFramework->AddExpectedError(
			TEXT("Chunks for component FakeItemComponent are missing."), EAutomationExpectedErrorFlags::Contains, 1);

		TArrayView<FFakeTransformComponent> TransformComponents = Result.GetArrayView<FFakeTransformComponent>();
		TArrayView<FFakeItemComponent> ItemComponents = Result.GetArrayView<FFakeItemComponent>();
		
		ANANKE_TEST_EQUAL(TestFramework, TransformComponents.Num(), 1);
		ANANKE_TEST_EQUAL(TestFramework, ItemComponents.Num(), 0);
	}

	void Test_StripComponents()
	{
		InitDB(1*1024); // 1kb per chunk.

		int32 NumberToAdd = 30;
		TArray<FInstancedStruct> ToAdd;
		auto Transform = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ToAdd.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform)));
		FMantleIterator AddResult = MantleDB->AddEntities(ToAdd, NumberToAdd);

		TArray<FGuid> ToStrip;

		// Strip half of the entities.
		const int32 NumberToStrip = 15;
		
		while (AddResult.Next() && ToStrip.Num() < NumberToStrip)
		{
			TArrayView<FGuid> Entities = AddResult.GetEntities();
			for (int EntityIndex = 0; EntityIndex < Entities.Num() && ToStrip.Num() < NumberToStrip; ++EntityIndex)
			{
				ToStrip.Add(Entities[EntityIndex]);
			}
		}

		TArray<UScriptStruct*> TypesToRemove;
		TypesToRemove.Add(FFakeTransformComponent::StaticStruct());

		FMantleIterator StripResult = MantleDB->UpdateEntities(ToStrip, TypesToRemove);

		if (!ANANKE_TEST_EQUAL(TestFramework, StripResult.LocalCache.MatchingEntries.Num(), 1))
		{
			return;
		}
		FMantleCachedEntry CachedEntry = StripResult.LocalCache.MatchingEntries[0];
		ANANKE_TEST_TRUE(TestFramework, CachedEntry.ChunkedComponents.IsEmpty());
		
		if (!ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[0].Num(), NumberToStrip);
		
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 2);

		TBitArray<> BareArchetype = TBitArray<>(false, NumComponents);
		TSharedPtr<FMantleDBEntry>* BareArchetypeEntryPtr = MantleDB->EntriesByArchetype.Find(BareArchetype);

		if (!ANANKE_TEST_TRUE(TestFramework, BareArchetypeEntryPtr && BareArchetypeEntryPtr->IsValid()))
		{
			return;
		}
		
		FMantleDBEntry* Entry = BareArchetypeEntryPtr->Get();

		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Num(), 1);
		if (!ANANKE_TEST_EQUAL(TestFramework, Entry->AllChunkIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Find(Entry->AllChunkIds[0])->EntityIds.Num(), NumberToStrip);

		FMantleComponentQuery TransformQuery;
		TransformQuery.AddRequiredComponent<FFakeTransformComponent>();
		FMantleIterator TransformQueryResult = MantleDB->RunQuery(TransformQuery);

		ANANKE_TEST_TRUE(TestFramework, TransformQueryResult.IsValid());

		int32 EntitiesChecked = 0;
		
		while (TransformQueryResult.Next())
		{
			TArrayView<FFakeTransformComponent> ResultTransforms = TransformQueryResult.GetArrayView<FFakeTransformComponent>();

			for (FFakeTransformComponent ResultTransform : ResultTransforms)
			{
				ANANKE_TEST_EQUAL(TestFramework, ResultTransform.Transform.GetLocation(), FVector(10.0f, 20.0f, 30.0f));
				EntitiesChecked++;
			}
		}

		ANANKE_TEST_EQUAL(TestFramework, EntitiesChecked, NumberToAdd - NumberToStrip);
	}

	void Test_StripComponentsAndEmptyEntry()
	{
		InitDB(1*1024); // 1kb per chunk.

		int32 NumberToAdd = 30;
		TArray<FInstancedStruct> ToAdd;
		auto Transform = FTransform(FVector(10.0f, 20.0f, 30.0f));
		ToAdd.Add(FInstancedStruct::Make(FFakeTransformComponent(Transform)));
		FMantleIterator AddResult = MantleDB->AddEntities(ToAdd, NumberToAdd);

		TArray<FGuid> ToStrip;

		// Strip all of the entities.
		const int32 NumberToStrip = NumberToAdd;
		
		while (AddResult.Next() && ToStrip.Num() < NumberToStrip)
		{
			TArrayView<FGuid> Entities = AddResult.GetEntities();
			for (int EntityIndex = 0; EntityIndex < Entities.Num() && ToStrip.Num() < NumberToStrip; ++EntityIndex)
			{
				ToStrip.Add(Entities[EntityIndex]);
			}
		}

		TArray<UScriptStruct*> TypesToRemove;
		TypesToRemove.Add(FFakeTransformComponent::StaticStruct());

		FMantleIterator StripResult = MantleDB->UpdateEntities(ToStrip, TypesToRemove);

		if (!ANANKE_TEST_EQUAL(TestFramework, StripResult.LocalCache.MatchingEntries.Num(), 1))
		{
			return;
		}
		FMantleCachedEntry CachedEntry = StripResult.LocalCache.MatchingEntries[0];
		ANANKE_TEST_TRUE(TestFramework, CachedEntry.ChunkedComponents.IsEmpty());
		
		if (!ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, CachedEntry.ChunkedEntityIds[0].Num(), NumberToStrip);
		
		ANANKE_TEST_EQUAL(TestFramework, MantleDB->EntriesByArchetype.Num(), 2);

		TBitArray<> BareArchetype = TBitArray<>(false, NumComponents);
		TSharedPtr<FMantleDBEntry>* BareArchetypeEntryPtr = MantleDB->EntriesByArchetype.Find(BareArchetype);

		if (!ANANKE_TEST_TRUE(TestFramework, BareArchetypeEntryPtr && BareArchetypeEntryPtr->IsValid()))
		{
			return;
		}
		
		FMantleDBEntry* Entry = BareArchetypeEntryPtr->Get();

		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Num(), 1);
		if (!ANANKE_TEST_EQUAL(TestFramework, Entry->AllChunkIds.Num(), 1))
		{
			return;
		}
		ANANKE_TEST_EQUAL(TestFramework, Entry->Chunks.Find(Entry->AllChunkIds[0])->EntityIds.Num(), NumberToStrip);

		FMantleComponentQuery TransformQuery;
		TransformQuery.AddRequiredComponent<FFakeTransformComponent>();
		FMantleIterator TransformQueryResult = MantleDB->RunQuery(TransformQuery);

		ANANKE_TEST_TRUE(TestFramework, TransformQueryResult.IsValid());

		// Currently, Next() still returns true because the iterator doesn't know anything about how many entities are
		// in a particular chunk
		ANANKE_TEST_TRUE(TestFramework, TransformQueryResult.Next());
		ANANKE_TEST_EQUAL(TestFramework, TransformQueryResult.GetEntities().Num(), 0);
		ANANKE_TEST_EQUAL(TestFramework, TransformQueryResult.GetArrayView<FFakeTransformComponent>().Num(), 0);
		ANANKE_TEST_FALSE(TestFramework, TransformQueryResult.Next());
	}

	// IMPORTANT! Be sure to register your fn inside your AutomationTest class below!

private:
	FAutomationTestBase* TestFramework;

	// Test objects
	TStrongObjectPtr<UMantleDB> MantleDB;
	TStrongObjectPtr<UWorld> TestWorld;

	// Other test data
	FString TransformComponentName;
	FString ItemComponentName;
	FString TargetingComponentName;
	FString BigComponentName;
	FString EmptyComponentName;
	int32 TransformComponentBitIndex;
	int32 ItemComponentBitIndex;
	int32 TargetingComponentBitIndex;
	int32 BigComponentBitIndex;
	int32 EmptyComponentBitIndex;

	int32 NumComponents;
};

#define REGISTER_TEST_SUITE_FN(TargetTestName) Tests.Add(TEXT(#TargetTestName), &TestSuite::TargetTestName)

class FMantleDBTest: public FAutomationTestBase
{
public:
	typedef void (TestSuite::*TestFunction)();
	
	FMantleDBTest(const FString& TestName): FAutomationTestBase(TestName, false)
	{
		REGISTER_TEST_SUITE_FN(Test_SmokeTest);
		REGISTER_TEST_SUITE_FN(Test_InitializeDB);
		REGISTER_TEST_SUITE_FN(Test_DBDoesNotAddComponentTypesIfAlreadyInitialized);
		REGISTER_TEST_SUITE_FN(Test_AddEntity);
		REGISTER_TEST_SUITE_FN(Test_AddEntities);
		REGISTER_TEST_SUITE_FN(Test_AddBareEntities);
		REGISTER_TEST_SUITE_FN(Test_SingleArchetypeQuery);
		REGISTER_TEST_SUITE_FN(Test_SingleArchetypeQueryNoResults);
		REGISTER_TEST_SUITE_FN(Test_MultiArchetypeQuery);
		REGISTER_TEST_SUITE_FN(Test_EmptyComponentQuery);
		REGISTER_TEST_SUITE_FN(Test_GetComponent);
		REGISTER_TEST_SUITE_FN(Test_RemoveEntities);
		REGISTER_TEST_SUITE_FN(Test_DBModificationInvalidatesIterator);
		REGISTER_TEST_SUITE_FN(Test_UpdateEntities);
		REGISTER_TEST_SUITE_FN(Test_StripComponents);
		REGISTER_TEST_SUITE_FN(Test_StripComponentsAndEmptyEntry);

		// Error tests.
		REGISTER_TEST_SUITE_FN(ErrorTest_GetUnknownArrayView);

		// TODO(): Add tests for new FGuid version of AddEntity()
		
		// TODO(): Test EntitiesSkipped during Update call.
		// TODO(): Test remove all entities (remove)
	}
	
	virtual uint32 GetTestFlags() const override
	{
		return EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	}
	virtual bool IsStressTest() const { return false; }
	virtual uint32 GetRequiredDeviceNum() const override { return 1; }

	// It appears that there is no way to check for warnings using the default AutomationTest framework, so we'll just
	// elevate them to errors for now.
	virtual bool ElevateLogWarningsToErrors() override { return true; }

protected:
	virtual FString GetBeautifiedTestName() const override
	{
		// This string is what the editor uses to organize your test in the Automated tests browser.
		return "Ananke.Mantle.Runtime.MantleDBTest";
	}
	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override
	{
		TArray<FString> TargetTestNames;
		Tests.GetKeys(TargetTestNames);
		for (const FString& TargetTestName : TargetTestNames)
		{
			OutBeautifiedNames.Add(TargetTestName);
			OutTestCommands.Add(TargetTestName);
		}
	}
	virtual bool RunTest(const FString& Parameters) override
	{
		TestFunction* CurrentTest = Tests.Find(Parameters);
		if (!CurrentTest || !*CurrentTest)
		{
			ANANKE_LOG(LogMantleTest, Error, TEXT("Cannot find test: %s"), *Parameters);
			return false;
		}

		TestSuite Suite(this);
		(Suite.**CurrentTest)(); // Run the current test from the test suite.

		return true;
	}

	TMap<FString, TestFunction> Tests;
};

namespace
{
	FMantleDBTest FMantleDBTestInstance(TEXT("FMantleDBTest"));
}

#endif //WITH_EDITOR