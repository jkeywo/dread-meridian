#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DMUtilityAI.h"
#include "DMAIProfile.generated.h"

class FJsonObject;

/**
 * Per-role / per-hero utility weights as an editable data asset.
 *
 * Assets live under /Game/DreadMeridian/AI/AIP_<ProfileName> (Gunman, Bruiser, Lookout, Bomber, GangBoss,
 * Sapper, Photographer, Medium, Smuggler, Raider). tools/create_ai_profiles.py creates them from the C++
 * defaults. Runtime never requires the assets: Resolve falls back to a transient object built from
 * DMUtilityAI::DefaultWeights, so headless soak, smoke and CI stay content-independent.
 *
 * Tuning overrides: the command line switch -DMAIWeights=<path.json> applies a JSON document on top of
 * whatever Resolve loaded. Shape:
 *   { "Gunman": { "KStock": 2.5,
 *                 "Actions": { "KeepDistance": { "Weight": 1.2, "MaxRuntimeTicks": 40 } },
 *                 "Abilities": { "Signature": { "Base": 6, "Magnitude": 8 } } },
 *     "Sapper": { ... } }
 * Scalar keys at the profile level map onto FDMAIWeights UPROPERTYs by name; "Actions" is keyed by
 * EDMAIAction name and patches FDMAIActionSpec fields; "Abilities" is keyed by EDMAIAction name and
 * patches FDMAIAbilityTemplate fields. Unknown keys are logged and ignored.
 */
UCLASS(BlueprintType)
class DREADMERIDIAN_API UDMAIProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FString ProfileId;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FDMAIWeights Weights;

    static FString ProfileName(EDMSmuggler Role, EDMInvestigator Kind);
    static FString AssetPath(const FString& ProfileName);
    static bool ProfileFromName(const FString& Name, EDMSmuggler& OutRole, EDMInvestigator& OutKind);
    static TArray<FString> AllProfileNames();

    /**
     * Load the asset for the role/kind or build a transient default (outer = Outer). If OverrideJsonPath is not
     * empty the file is parsed once and its section for this profile is applied. Logs once per profile.
     */
    static UDMAIProfile* Resolve(UObject* Outer, EDMSmuggler Role, EDMInvestigator Kind, const FString& OverrideJsonPath = FString());

    /** Reset Weights to DMUtilityAI::DefaultWeights for ProfileId. Editor button and Python entry point. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "AI")
    void ApplyDefaults();

    /** Apply one profile's JSON section (see the class comment). Returns false if nothing was applied. */
    bool ApplyJsonOverrides(const TSharedPtr<FJsonObject>& Section);
    /** Full weights as JSON (the inverse shape of ApplyJsonOverrides, for tuning scripts and debugging). */
    TSharedPtr<FJsonObject> ToJson() const;
};
