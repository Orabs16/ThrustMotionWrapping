#include "BoneTrajectoryLibrary.h"

#include "BonePoseUtils.h"
#include "Animation/AnimSequence.h"
#include "Animation/BuiltInAttributeTypes.h"
#include "Animation/AnimData/AttributeIdentifier.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogBoneTrajectory, Log, All);

#define LOCTEXT_NAMESPACE "BoneTrajectoryLibrary"

namespace BoneTrajectory
{
	// Direction of the least-squares line through the points: the principal axis of their covariance,
	// found by power iteration starting from InitialGuess. Zero if the points don't spread out.
	FVector FitLineDirection(const TArray<FVector>& Points, const FVector& InitialGuess)
	{
		FVector Centroid = FVector::ZeroVector;
		for (const FVector& Point : Points)
		{
			Centroid += Point;
		}
		Centroid /= Points.Num();

		// Symmetric covariance matrix, unnormalized since only its principal axis matters
		double XX = 0.0, XY = 0.0, XZ = 0.0, YY = 0.0, YZ = 0.0, ZZ = 0.0;
		for (const FVector& Point : Points)
		{
			const FVector D = Point - Centroid;
			XX += D.X * D.X; XY += D.X * D.Y; XZ += D.X * D.Z;
			YY += D.Y * D.Y; YZ += D.Y * D.Z; ZZ += D.Z * D.Z;
		}

		FVector Axis = InitialGuess.GetSafeNormal();
		if (Axis.IsZero())
		{
			Axis = FVector::OneVector.GetSafeNormal();
		}
		for (int32 Iteration = 0; Iteration < 64; ++Iteration)
		{
			const FVector Next = FVector(
				XX * Axis.X + XY * Axis.Y + XZ * Axis.Z,
				XY * Axis.X + YY * Axis.Y + YZ * Axis.Z,
				XZ * Axis.X + YZ * Axis.Y + ZZ * Axis.Z).GetSafeNormal();
			if (Next.IsZero())
			{
				return FVector::ZeroVector;
			}
			const bool bConverged = (Next | Axis) > 1.0 - UE_DOUBLE_KINDA_SMALL_NUMBER;
			Axis = Next;
			if (bConverged)
			{
				break;
			}
		}
		return Axis;
	}

	// Replaces the attribute with a constant value keyed at both ends of the sequence. Must be called inside a controller bracket.
	void WriteConstantVectorAttribute(UAnimSequence* Sequence, FName AttributeName, FName BoneName, const FVector& Value)
	{
		IAnimationDataController& Controller = Sequence->GetController();
		const FAnimationAttributeIdentifier AttributeId = UAnimationAttributeIdentifierExtensions::CreateAttributeIdentifier(Sequence, AttributeName, BoneName, FVectorAnimationAttribute::StaticStruct());

		if (Sequence->GetDataModel()->FindAttribute(AttributeId))
		{
			Controller.RemoveAttribute(AttributeId);
		}
		Controller.AddAttribute(AttributeId);

		FVectorAnimationAttribute Attribute;
		Attribute.Value = Value;
		const float Times[] = { 0.f, Sequence->GetPlayLength() };
		const FVectorAnimationAttribute Values[] = { Attribute, Attribute };
		Controller.SetTypedAttributeKeys<FVectorAnimationAttribute>(AttributeId, Times, Values);
	}
}

bool UBoneTrajectoryLibrary::AnalyzeBoneTrajectory(UAnimSequence* Sequence, FName BoneName, FVector& OutDirection, FVector& OutSummit, int32 StartFrame, int32 EndFrame, FName DirectionAttributeName, FName SummitAttributeName)
{
	OutDirection = FVector::ZeroVector;
	OutSummit = FVector::ZeroVector;

	if (!Sequence || !Sequence->GetSkeleton())
	{
		UE_LOG(LogBoneTrajectory, Warning, TEXT("AnalyzeBoneTrajectory: invalid sequence."));
		return false;
	}

	if (DirectionAttributeName.IsNone() || SummitAttributeName.IsNone())
	{
		UE_LOG(LogBoneTrajectory, Warning, TEXT("AnalyzeBoneTrajectory: missing attribute name for %s."), *Sequence->GetName());
		return false;
	}

	const FReferenceSkeleton& RefSkeleton = Sequence->GetSkeleton()->GetReferenceSkeleton();
	const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogBoneTrajectory, Warning, TEXT("AnalyzeBoneTrajectory: bone %s not found in skeleton of %s."), *BoneName.ToString(), *Sequence->GetName());
		return false;
	}

	const int32 LastFrame = Sequence->GetDataModel()->GetNumberOfFrames();
	const int32 ResolvedEndFrame = EndFrame == -1 ? LastFrame : EndFrame;
	if (StartFrame < 0 || ResolvedEndFrame > LastFrame || StartFrame >= ResolvedEndFrame)
	{
		UE_LOG(LogBoneTrajectory, Warning, TEXT("AnalyzeBoneTrajectory: invalid frame range [%d, %d] for %s (last frame is %d)."), StartFrame, EndFrame, *Sequence->GetName(), LastFrame);
		return false;
	}

	// Including the root in the path gives component space with root motion, matching the global space
	// shown in the animation preview.
	const TArray<int32> BonePath = BonePoseUtils::GetBonePathToRoot(RefSkeleton, BoneIndex, true);

	const FVector StartLocation = BonePoseUtils::EvaluateBonePathLocation(Sequence, BonePath, FFrameTime(FFrameNumber(StartFrame)));
	TArray<FVector> Locations;
	Locations.Reserve(ResolvedEndFrame - StartFrame + 1);
	FVector Summit = StartLocation;
	double SummitDistanceSquared = 0.0;
	for (int32 Frame = StartFrame; Frame <= ResolvedEndFrame; ++Frame)
	{
		const FVector& Location = Locations.Add_GetRef(BonePoseUtils::EvaluateBonePathLocation(Sequence, BonePath, FFrameTime(FFrameNumber(Frame))));

		const double DistanceSquared = FVector::DistSquared(Location, StartLocation);
		UE_LOG(LogTemp, Log, TEXT("Location : %f, %f, %f, distance : %f"), Location.X, Location.Y, Location.Z, DistanceSquared);
		if (DistanceSquared > SummitDistanceSquared)
		{
			SummitDistanceSquared = DistanceSquared;
			Summit = Location;
		}
	}

	const FVector StartToSummit = Summit - StartLocation;
	FVector Direction = BoneTrajectory::FitLineDirection(Locations, StartToSummit);
	if ((Direction | StartToSummit) < 0.0)
	{
		Direction = -Direction;
	}
	if (Direction.IsZero())
	{
		UE_LOG(LogBoneTrajectory, Warning, TEXT("AnalyzeBoneTrajectory: %s does not move in [%d, %d] of %s, writing a zero direction."), *BoneName.ToString(), StartFrame, ResolvedEndFrame, *Sequence->GetName());
	}

	IAnimationDataController& Controller = Sequence->GetController();
	Controller.OpenBracket(LOCTEXT("AnalyzeBoneTrajectory", "Analyze Bone Trajectory"));
	BoneTrajectory::WriteConstantVectorAttribute(Sequence, DirectionAttributeName, BoneName, Direction);
	BoneTrajectory::WriteConstantVectorAttribute(Sequence, SummitAttributeName, BoneName, Summit);
	Controller.CloseBracket();

	OutDirection = Direction;
	OutSummit = Summit;
	return true;
}

#undef LOCTEXT_NAMESPACE
