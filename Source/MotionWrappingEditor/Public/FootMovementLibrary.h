#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FootMovementLibrary.generated.h"

class UAnimSequence;

UCLASS()
class MOTIONWRAPPINGEDITOR_API UFootMovementLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Writes a 0-1 "planted" weight curve for a bone (e.g. heel or tip of a foot).
	 * The bone's world-space speed (root motion included) is sampled over the sequence:
	 * at or below PlantedSpeed the weight is 1, at or above MovingSpeed it is 0, with a smoothstep ramp in between.
	 * An existing curve with the same name is replaced.
	 *
	 * @param Sequence		Sequence to write the curve to
	 * @param BoneName		Bone whose movement is tracked
	 * @param CurveName		Name of the float curve to write
	 * @param PlantedSpeed	Speed (cm/s) at or below which the weight is 1
	 * @param MovingSpeed	Speed (cm/s) at or above which the weight is 0. Equal to PlantedSpeed gives a hard step
	 * @param SampleRate	Samples per second, 0 uses the sequence's frame rate
	 * @param bLooping		Wrap speed computation around the clip ends instead of using one-sided differences
	 * @param SmoothingTime	Width (seconds) of the moving average applied to the speed to remove jitter, 0 disables it
	 * @return				False if the inputs are invalid, in which case the sequence is left untouched
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Foot Movement")
	static bool DetectFootBoneMovement(UAnimSequence* Sequence, FName BoneName, FName CurveName, float PlantedSpeed = 10.f, float MovingSpeed = 30.f, float SampleRate = 0.f, bool bLooping = false, float SmoothingTime = 0.f);
};
