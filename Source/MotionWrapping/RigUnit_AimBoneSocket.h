#pragma once

#include "Units/Highlevel/RigUnit_HighlevelBase.h"
#include "RigUnit_AimBoneSocket.generated.h"

/**
 * Rotates a bone so that the X axis of a socket attached to it points at a global target.
 * The socket keeps its offset to the bone, so it swings around with the bone while the bone stays in place.
 * Of all the rotations that solve it, the one closest to the current bone rotation is used.
 */
USTRUCT(meta = (DisplayName = "Aim Bone Socket", Category = "Motion Wrapping", Keywords = "Lookat,Aim,Socket,Offset"))
struct MOTIONWRAPPING_API FRigUnit_AimBoneSocket : public FRigUnit_HighlevelBaseMutable
{
	GENERATED_BODY()

	FRigUnit_AimBoneSocket()
	{
		Bone = FRigElementKey(NAME_None, ERigElementType::Bone);
	}

	RIGVM_METHOD()
	virtual void Execute() override;

	/** The bone to rotate */
	UPROPERTY(meta = (Input, ExpandByDefault))
	FRigElementKey Bone;

	/** Transform of the socket relative to the bone */
	UPROPERTY(meta = (Input))
	FTransform SocketOffset = FTransform::Identity;

	/** Global position the socket X axis aims at */
	UPROPERTY(meta = (Input))
	FVector Target = FVector::ZeroVector;

	/** How much of the rotation is applied */
	UPROPERTY(meta = (Input, UIMin = "0.0", UIMax = "1.0"))
	float Weight = 1.f;

	/** Moves the bone's children along with it */
	UPROPERTY(meta = (Input))
	bool bPropagateToChildren = true;

	/**
	 * False when the socket axis can't pass through the target, because the target is closer to the bone than the axis ever gets.
	 * The bone is then rotated to get the axis as close to the target as it can.
	 */
	UPROPERTY(meta = (Output))
	bool bReachable = false;

	UPROPERTY()
	FCachedRigElement CachedBone;
};
