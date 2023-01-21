using UnrealBuildTool;

public class ALSGas : ModuleRules
{
    public ALSGas(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "GameplayTags",
            "EnhancedInput",
            "AIModule",
            "ALS",
            "ALSCamera",
            "GameplayAbilities",
            "GameplayTasks",
            "NetCore"
        });
    }
}