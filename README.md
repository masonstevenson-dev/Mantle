# Mantle ECS

Author: Mason Stevenson

[https://masonstevenson.dev](https://masonstevenson.dev/)

<br>

Mantle is a simplified entity component system (ECS) for Unreal Engine. I worked on this project in 2023 because I wanted to better understand how the official UE mass entity plugin works. I ended up creating my own ECS from scratch. However, the architecture is heavily inspired by the mass entity plugin, and for most people who are not *me*, it would probably be better if you just went and used that instead. This repo is mostly just as a record of the work I did. 

<br>

The main goal for this project is to explore ECS architecture and how it could be integrated with other common gameplay systems such as player perception or ability systems. Performance is not a main focus. If you are looking for a performant and regularly maintained ECS for Unreal Engine, I suggest looking into [Mass Entity](https://dev.epicgames.com/documentation/en-us/unreal-engine/mass-entity-in-unreal-engine) or [flecs](https://github.com/SanderMertens/flecs).

<br>

## Required Plugins

This plugin uses my [AnankeCore](https://github.com/masonstevenson-dev/AnankeCore) plugin for logging. This plugin is currently required; however, I may make it optional in the future.

<br>

## Installation

Install this plugin in your project's plugins folder. The folder structure should be: `YourProject/Plugins/Mantle/Mantle/Source`.

<br>

The plugin is enabled by default.

<br>

### Initial Setup

Make sure your game instance inherits from **UMantleGameInstance**.

<br>

## Usage

When working with Mantle, there are two primary types of actions that can be performed:

* Entity management
* Operating across entities

<br>

### Entity Management

An "entity" is simply an ID that points to some collection of data (i.e. one or more components). All entities are registered and maintained with the **MantleDB**, which is a special UObject maintained by a GameInstanceSubsystem called **UMantleEngine**.

<br>

For example, a player may find a sword actor in the world, and that sword actor may have some interesting properties, like base damage, durability, etc. Perhaps also this particular sword has an elemental property as well, such as FIRE. These properties can be collected on your own custom components, which can be used to describe the sword entity:

```c++
UENUM()
enum class EWeaponType: uint8
{
    Unknown,
    Sword,
    Dagger,
    Spear,
    Bow
}

// A component that describes some basic attributes every weapon should have
USTRUCT()
struct FMC_Weapon : public FMantleComponent
{
    GENERATED_BODY()

public:
    // Some properties associated with a "weapon" and their default values
    EWeaponType WeaponType = EWeaponType::Unknown;
    float Damage = 1.0f;
    float Durability = 100.0f;
};

UENUM()
enum class EPrimaryElementType: uint8
{
    None,
    Fire,
    Ice,
    Lightning,
}

UENUM()
enum class ESecondaryElementType: uint8
{
    None,
    Cursed,
    Blessed,
    Shadow,
    Radiant
}

USTRUCT()
struct FMC_Element : public FMantleComponent
{
    GENERATED_BODY()

public:
    EPrimaryElementType PrimaryElement = EPrimaryElementType::None;
    ESecondaryElementType SecondaryElement = ESecondaryElementType::None;
};
```

<br>

When you create a struct that inherits from **FMantleComponent**, it is automatically registered as a known component type when MantleEngine initializes.

<br>

In your sword actor code, you can register an entity that describes your sword:

```c++
void AShadowFlameSword::PreInitializeComponents()
{
    Super::PreInitializeComponents();
    
    MantleDB = UMantleEngineLibrary::GetMantleDB(GetGameInstance());
    if (!MantleDB.IsValid())
    {
        UE_LOG(LogYourProj, Error, TEXT("Expected valid MantleDB instance."));
        return;
    }

    TArray<FInstancedStruct> ComponentList;
    
    // Create all the components that describe this entity
    FMC_Weapon WeaponComponent;
    WeaponComponent.WeaponType = EWeaponType::Sword;
    WeaponComponent.Damage = 30.0f;
       
    FMC_Element ElementComponent;
    ElementComponent.SecondaryElement = ESecondaryElementType::Shadow;
    ElementComponent.PrimaryElement = EPrimaryElementType::Fire;
    
    ComponentList.Add(FInstancedStruct::Make(WeaponComponent));
    ComponentList.Add(FInstancedStruct::Make(ElementComponent));
    
    FGuid Result = MantleDB->AddEntity(ComponentList);
    if(!Result.IsValid())
    {
        UE_LOG(LogYourProj, Error, TEXT("Expected AddEntity to have a valid result."));
        return;
    }
    
    MantleId = Result;
    if (!MantleId.IsValid())
    {
        UE_LOG(LogYourProj, Error, TEXT("Expected valid entity id"));
        return;
    }
}

```

<br>

Note that you don't have to create a custom actor for each sword type here. You could set something up where a weapon definition is described in a UE [data asset](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-assets-in-unreal-engine) or [data table](https://dev.epicgames.com/documentation/en-us/unreal-engine/data-driven-gameplay-elements-in-unreal-engine#datatables) and then you could create a generic AWeapon actor and have it ingest the data asset and then construct the Mantle entity based on the values in that asset. This is an implementation detail that is left up to you to decide what is best for your project.

<br>

For a full list of all the entity management functions, see UMantleDB in MantleDB.h.

<br>

### Operating on Entities

Once you have created a bunch of entities, you probably want to do something with them! A common paradigm is to iterate through every entity with a particular composition each engine tick and perform some type of computation on it. Mantle provides a mechanism for performing these iterations through the use of MantleOperations. 

<br>

For example, we may want to iterate through all entities with the *Spell* and *AreaOfEffect* components, THEN iterate through all entities with the *Enemy* and the *Flight* components, and to check if an enemy has been affected by a *Gravity* spell: 

```cpp
UCLASS()
class UMO_ApplyGravitySpells : public UMantleOperation
{
    GENERATED_BODY()

public:
    UMO_ApplyGravitySpells(const FObjectInitializer& Initializer);
    
    virtual void PerformOperation(FMantleOperationContext& Ctx) override;

protected:
    FMantleComponentQuery SpellQuery;
    FMantleComponentQuery FlyingEnemyQuery;
};
```

```c++
UMO_ApplyGravitySpells::UMO_ApplyGravitySpells(const FObjectInitializer& Initializer): Super(Initializer)
{
    // Query for AOE spells
    SpellQuery.AddRequiredComponent<FMC_Spell>();
    SpellQuery.AddRequiredComponent<FMC_AreaOfEffect>();
    
    // Query for enemies that can fly
    FlyingEnemyQuery.AddRequiredComponent<FMC_Enemy>();
    FlyingEnemyQuery.AddRequiredComponent<FMC_Flight>();
}

void UMO_ImpactDamage::PerformOperation(FMantleOperationContext& Ctx)
{
    TArray<FMC_AreaOfEffect> ActiveGravitySpellAreas;
    FMantleIterator SpellQueryResult = Ctx.MantleDB->RunQuery(SpellQuery);
    
    while (SpellQueryResult.Next())
    {
        TArrayView<FGuid> SourceEntities = SpellQueryResult.GetEntities();
        TArrayView<FMC_Spell> SpellInfo = SpellQueryResult.GetArrayView<FMC_Collision>();
        TArrayView<FMC_AreaOfEffect> AOEInfo = SpellQueryResult.GetArrayView<FMC_AreaOfEffect>();
        
        for (int32 EntityIndex = 0; EntityIndex < SourceEntities.Num(); ++EntityIndex)
        {
            FMC_Spell Spell = SpellInfo[EntityIndex];
            FMC_AreaOfEffect AOE = AOEInfo[EntityIndex];
            
            if (!Spell.bIsActive)
            {
                continue; 
            }
            
            if (Spell.Type == ESpellTypes::Gravity)
            {
                ActiveGravitySpellAreas.Add(AOE);
            }
        }
    }
    
    FMantleIterator EnemyQueryResult = Ctx.MantleDB->RunQuery(FlyingEnemyQuery);
    
    while (EnemyQueryResult.Next())
    {
        TArrayView<FGuid> SourceEntities = SpellQueryResult.GetEntities();
        TArrayView<FMC_Enemy> EnemyInfo = SpellQueryResult.GetArrayView<FMC_Enemy>();
        TArrayView<FMC_Flight> FlightInfo = SpellQueryResult.GetArrayView<FMC_Flight>();
        
        for (int32 EntityIndex = 0; EntityIndex < SourceEntities.Num(); ++EntityIndex)
        { 
            FMC_Enemy Enemy = FlightInfo[EntityIndex];
            FMC_Flight EnemyFlight = FlightInfo[EntityIndex];
            
            if (EnemyFlight.bIsFlying)
            {
                // Skip over enemies that aren't currently flying
                continue;
            }
             
            for (FMC_AreaOfEffect AoeRegion : ActiveGravitySpellAreas)
            {
                // assume there is some helper fn that checks for overlaps
                if (AoeRegion.Overlaps(Enemy.BoundingBox))
                {
                    EnemyFlight.bIsFlying = false;
                    EnemyFlight.bIsAffectedByGravitySpell = true;
                    
                    // Maybe you want to have something happen on screen or play a sound when an enemy
                    // gets hit with the spell? This is one way you could do it.
                    EnemyFlight.StateChangeDelegate.Broadcast();
                }
            }
        }
    }
}
```

<br>

Unlike MantleComponents, these operations need to be registered in your game instance in order for them to run. First, make sure your game instance class inherits from **UMantleGameInstance**, then implement the ConfigureMantleEngine() function and define when you want your operations to tick:

```cpp
void UYourGameInstance::ConfigureMantleEngine(UMantleEngine& MantleEngine)
{
    FMantleEngineLoopOptions PrePhysicsPhase;
    PrePhysicsPhase.OperationGroups.AddDefaulted();

    // These operations will run in the pre-physics tick group
    PrePhysicsPhase.OperationGroups[0].Operations.Append({
        Engine.NewOperation<UMO_YourCustomOperation_1>(),
        Engine.NewOperation<UMO_YourCustomOperation_2>(),
        Engine.NewOperation<UMO_YourCustomOperation_3>()
    });
    
    Engine.ConfigureEngineLoop(TG_PrePhysics, PrePhysicsPhase);

    // These operations will run in the frame-end tick group
    FMantleEngineLoopOptions FrameEndPhase;
    FrameEndPhase.OperationGroups.AddDefaulted();
    FrameEndPhase.OperationGroups[0].Operations.Append({
        Engine.NewOperation<UMO_YourCustomOperation_4>()
    });
    Engine.ConfigureEngineLoop(TG_LastDemotable, FrameEndPhase);
}
```

<br>

Operations will run in the order you add them to an operation group. Currently having multiple operation groups doesn't do anything, but in the future, these may be used to define groups of operations that can be run in parallel (so basically you could have operations A->B->C run independently from operations D->E->F). 

<br>

## Under the Hood

Mantle uses a [Struct of Arrays](https://en.wikipedia.org/wiki/AoS_and_SoA#Structure_of_arrays) (SOA) architecture for entity storage. This means that whenever a new entity is defined, mantle will assign it an "archetype" based on its component composition, and will then store each component of that entity in an array of components of the same type, from entities with the same archetype.

<br>

So for example, an entity with components A, B, and F, would have an archetype of **ABF**. Then, when the component is stored in the db, these components are actually separated out and placed together with other components of the same type, from different entities.

<br>

So entities entity_1, entity_2, and entity_3 of archetype ABF can be viewed as:

* entity_1: [A<sub>1</sub>, B<sub>1</sub>, F<sub>1</sub>]
* entity_2: [A<sub>2</sub>, B<sub>2</sub>, F<sub>2</sub>]
* entity_3: [A<sub>3</sub>, B<sub>3</sub>, F<sub>3</sub>]

<br>

But in memory, these entities will get stored as follows:

* ABF archetype chunk:
  * |A<sub>1</sub>|A<sub>2</sub>|A<sub>3</sub>|\*|\*|B<sub>1</sub>|B<sub>2</sub>|B<sub>3</sub>|\*|\*|C<sub>1</sub>|C<sub>2</sub>|C<sub>3</sub>|\*|\*|

<br>

Note in this simplified example, the chunk can store 5 entities, there are empty spots for two more entities illustrated by '\*'. By default, mantle creates 128kb sized chunks. It will dynamically create and destroy chunks as needed for each archetype. When you iterate through the FMantleIterator returned by `Ctx.MantleDB->RunQuery(SomeQuery)`, you are actually iterating through every DB memory chunk for every entity archetype that matches the query requirements. 

<br>

For example, if you had the following archetypes:

* ABF
* ABCD
* ABDF
* FG
* AF

<br>

And you ran a query looking for entities containing AF, the resulting iterator would loop through chunks from the following archetypes:

* ABF
* ABDF
* AF

<br>

Remember that mantle may create multiple chunks for the same archetype if a chunk runs out of space.

<br>