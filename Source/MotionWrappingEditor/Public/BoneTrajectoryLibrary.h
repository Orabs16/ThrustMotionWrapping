#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BoneTrajectoryLibrary.generated.h"

class UAnimSequence;

UCLASS()
class MOTIONWRAPPINGEDITOR_API UBoneTrajectoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Analyzes a bone's trajectory between two frames and writes the result as blendable vector attributes on that bone,
	 * keyed constant over the whole sequence. Positions are in component space including the root bone, so root motion is included.
	 * Existing attributes with the same names on the bone are replaced.
	 *
	 * @param Sequence					Sequence to analyze and write to
	 * @param BoneName					Bone whose trajectory is analyzed and that receives the attributes
	 * @param OutDirection				Direction written to DirectionAttributeName, zero if the inputs are invalid
	 * @param OutSummit					Summit written to SummitAttributeName, zero if the inputs are invalid
	 * @param StartFrame				First frame of the range (inclusive)
	 * @param EndFrame					Last frame of the range (inclusive), -1 uses the last frame of the sequence
	 * @param DirectionAttributeName	Attribute receiving the normalized direction of the best-fit line through the path samples,
	 *									oriented toward the summit. Zero if the bone does not move
	 * @param SummitAttributeName		Attribute receiving the path position farthest from the start position
	 * @return							False if the inputs are invalid, in which case the sequence is left untouched
	 */
	UFUNCTION(BlueprintCallable, Category = "Animation|Bone Trajectory")
	static bool AnalyzeBoneTrajectory(UAnimSequence* Sequence, FName BoneName, FVector& OutDirection, FVector& OutSummit, int32 StartFrame = 0, int32 EndFrame = -1, FName DirectionAttributeName = "AverageDirection", FName SummitAttributeName = "Summit");
};
