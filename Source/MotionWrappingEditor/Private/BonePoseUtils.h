#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"

namespace BonePoseUtils
{
	// Bone indices from BoneIndex up to the root. With bIncludeRoot false the root bone is left out,
	// so evaluating the path gives the bone relative to the root (i.e. with root motion removed).
	inline TArray<int32> GetBonePathToRoot(const FReferenceSkeleton& RefSkeleton, int32 BoneIndex, bool bIncludeRoot)
	{
		TArray<int32> BonePath;
		for (int32 PathBoneIndex = BoneIndex; PathBoneIndex != INDEX_NONE; PathBoneIndex = RefSkeleton.GetParentIndex(PathBoneIndex))
		{
			if (bIncludeRoot || RefSkeleton.GetParentIndex(PathBoneIndex) != INDEX_NONE)
			{
				BonePath.Add(PathBoneIndex);
			}
		}
		return BonePath;
	}

	// Location of the first bone of BonePath in the space of the path's last bone parent, evaluated from the raw
	// animation data. Bones without a track use their reference pose.
	inline FVector EvaluateBonePathLocation(const UAnimSequence* Sequence, const TArray<int32>& BonePath, const FFrameTime& FrameTime)
	{
		const IAnimationDataModel* Model = Sequence->GetDataModel();
		const FReferenceSkeleton& RefSkeleton = Sequence->GetSkeleton()->GetReferenceSkeleton();

		FTransform Transform = FTransform::Identity;
		for (const int32 PathBoneIndex : BonePath)
		{
			const FName PathBoneName = RefSkeleton.GetBoneName(PathBoneIndex);
			const FTransform LocalTransform = Model->IsValidBoneTrackName(PathBoneName)
				? Model->EvaluateBoneTrackTransform(PathBoneName, FrameTime, EAnimInterpolationType::Linear)
				: RefSkeleton.GetRefBonePose()[PathBoneIndex];
			Transform = Transform * LocalTransform;
		}
		return Transform.GetLocation();
	}
}
