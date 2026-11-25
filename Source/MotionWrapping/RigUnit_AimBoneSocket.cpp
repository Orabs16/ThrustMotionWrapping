#include "RigUnit_AimBoneSocket.h"
#include "Units/RigUnitContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RigUnit_AimBoneSocket)

FRigUnit_AimBoneSocket_Execute()
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()

	bReachable = false;

	URigHierarchy* Hierarchy = ExecuteContext.Hierarchy;
	if (Hierarchy == nullptr)
	{
		return;
	}

	if (!CachedBone.UpdateCache(Bone, Hierarchy))
	{
		UE_CONTROLRIG_RIGUNIT_REPORT_WARNING(TEXT("Bone not found '%s'."), *Bone.ToString());
		return;
	}

	FTransform BoneTransform = Hierarchy->GetGlobalTransform(CachedBone);

	// Socket position and axis in bone space with the bone scale applied, so only the bone rotation is left to solve
	const FVector Offset = BoneTransform.GetScale3D() * SocketOffset.GetLocation();
	const FVector Axis = (BoneTransform.GetScale3D() * SocketOffset.GetRotation().GetAxisX()).GetSafeNormal();
	const FVector ToTarget = Target - BoneTransform.GetLocation();
	if (Axis.IsNearlyZero() || ToTarget.IsNearlyZero())
	{
		return;
	}

	// Rotating the bone keeps every point at its distance from the bone, so the target can only be hit by the point
	// of the socket axis line (Offset + T * Axis) at the target's distance, ahead of the socket: the larger root of
	// |Offset + T * Axis|^2 = |ToTarget|^2
	const double OffsetAlongAxis = Offset | Axis;
	const double Discriminant = FMath::Square(OffsetAlongAxis) - Offset.SizeSquared() + ToTarget.SizeSquared();
	double T = -OffsetAlongAxis;
	if (Discriminant >= 0.0)
	{
		T += FMath::Sqrt(Discriminant);
		bReachable = T > 0.0;
	}
	// Unreachable: fall back on the point of the axis closest to the bone, or the socket itself if that's behind it
	T = FMath::Max(T, 0.0);

	const FVector AimPoint = Offset + T * Axis;
	if (AimPoint.IsNearlyZero())
	{
		return;
	}

	// The shortest arc bringing that point onto the target, so the twist around the target direction is kept
	const FQuat Rotation = BoneTransform.GetRotation();
	const FQuat Delta = FQuat::FindBetweenVectors(Rotation.RotateVector(AimPoint), ToTarget);
	const FQuat Solved = Delta * Rotation;

	BoneTransform.SetRotation(FQuat::Slerp(Rotation, Solved, FMath::Clamp(Weight, 0.f, 1.f)).GetNormalized());
	Hierarchy->SetGlobalTransform(CachedBone, BoneTransform, bPropagateToChildren);
}
