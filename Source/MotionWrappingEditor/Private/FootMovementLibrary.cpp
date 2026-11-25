#include "FootMovementLibrary.h"

#include "BonePoseUtils.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"

DEFINE_LOG_CATEGORY_STATIC(LogFootMovement, Log, All);

#define LOCTEXT_NAMESPACE "FootMovementLibrary"

namespace FootMovement
{
	float SpeedToPlantedWeight(float Speed, float PlantedSpeed, float MovingSpeed)
	{
		if (Speed <= PlantedSpeed)
		{
			return 1.f;
		}
		if (Speed >= MovingSpeed)
		{
			return 0.f;
		}
		return 1.f - FMath::SmoothStep(PlantedSpeed, MovingSpeed, Speed);
	}
}

bool UFootMovementLibrary::DetectFootBoneMovement(UAnimSequence* Sequence, FName BoneName, FName CurveName, float PlantedSpeed, float MovingSpeed, float SampleRate, bool bLooping, float SmoothingTime)
{
	if (!Sequence || !Sequence->GetSkeleton())
	{
		UE_LOG(LogFootMovement, Warning, TEXT("DetectFootBoneMovement: invalid sequence."));
		return false;
	}

	if (CurveName.IsNone())
	{
		UE_LOG(LogFootMovement, Warning, TEXT("DetectFootBoneMovement: no curve name given for %s."), *Sequence->GetName());
		return false;
	}

	const FReferenceSkeleton& RefSkeleton = Sequence->GetSkeleton()->GetReferenceSkeleton();
	const int32 BoneIndex = RefSkeleton.FindBoneIndex(BoneName);
	if (BoneIndex == INDEX_NONE)
	{
		UE_LOG(LogFootMovement, Warning, TEXT("DetectFootBoneMovement: bone %s not found in skeleton of %s."), *BoneName.ToString(), *Sequence->GetName());
		return false;
	}

	if (MovingSpeed < PlantedSpeed)
	{
		UE_LOG(LogFootMovement, Warning, TEXT("DetectFootBoneMovement: MovingSpeed (%f) is lower than PlantedSpeed (%f)."), MovingSpeed, PlantedSpeed);
		return false;
	}

	const float Rate = SampleRate > 0.f ? SampleRate : static_cast<float>(Sequence->GetSamplingFrameRate().AsDecimal());
	const float Length = Sequence->GetPlayLength();
	if (Rate <= 0.f || Length <= 0.f)
	{
		UE_LOG(LogFootMovement, Warning, TEXT("DetectFootBoneMovement: %s has no length or frame rate."), *Sequence->GetName());
		return false;
	}

	const float DeltaTime = 1.f / Rate;
	const int32 NumSamples = FMath::Max(2, FMath::RoundToInt(Length * Rate) + 1);

	// The root bone track is evaluated as authored, so root motion is part of the world location
	// (assuming the actor sits at the origin).
	const TArray<int32> BonePath = BonePoseUtils::GetBonePathToRoot(RefSkeleton, BoneIndex, true);
	const FFrameRate ModelFrameRate = Sequence->GetDataModel()->GetFrameRate();

	TArray<float> Times;
	TArray<FVector> Locations;
	Times.Reserve(NumSamples);
	Locations.Reserve(NumSamples);
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		const float Time = FMath::Min(Index * DeltaTime, Length);
		Times.Add(Time);
		Locations.Add(BonePoseUtils::EvaluateBonePathLocation(Sequence, BonePath, ModelFrameRate.AsFrameTime(Time)));
	}

	auto SegmentSpeed = [&Times, &Locations](int32 From, int32 To)
	{
		const float Duration = Times[To] - Times[From];
		return Duration > UE_KINDA_SMALL_NUMBER ? FVector::Distance(Locations[From], Locations[To]) / Duration : 0.f;
	};

	const int32 Last = NumSamples - 1;
	TArray<float> Speeds;
	Speeds.Reserve(NumSamples);
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		if (Index > 0 && Index < Last)
		{
			Speeds.Add(SegmentSpeed(Index - 1, Index + 1));
		}
		else if (bLooping)
		{
			// First and last samples are the same instant of the loop. Average the speed on both sides of
			// the seam rather than differencing across it, which would include the clip's root displacement.
			Speeds.Add(0.5f * (SegmentSpeed(0, 1) + SegmentSpeed(Last - 1, Last)));
		}
		else
		{
			Speeds.Add(Index == 0 ? SegmentSpeed(0, 1) : SegmentSpeed(Last - 1, Last));
		}
	}

	// Denoise with a centered moving average. Looping clips wrap the window around the seam
	// (sample Last is the same instant as sample 0), others shrink it at the clip ends.
	const int32 HalfWindow = FMath::RoundToInt(SmoothingTime * Rate * 0.5f);
	if (HalfWindow > 0)
	{
		const TArray<float> RawSpeeds = Speeds;
		for (int32 Index = 0; Index < NumSamples; ++Index)
		{
			float Sum = 0.f;
			int32 Count = 0;
			for (int32 Offset = -HalfWindow; Offset <= HalfWindow; ++Offset)
			{
				int32 SampleIndex = Index + Offset;
				if (bLooping)
				{
					SampleIndex = ((SampleIndex % Last) + Last) % Last;
				}
				else if (SampleIndex < 0 || SampleIndex > Last)
				{
					continue;
				}
				Sum += RawSpeeds[SampleIndex];
				++Count;
			}
			Speeds[Index] = Sum / Count;
		}
	}

	TArray<FRichCurveKey> Keys;
	Keys.Reserve(NumSamples);
	for (int32 Index = 0; Index < NumSamples; ++Index)
	{
		FRichCurveKey& Key = Keys.Add_GetRef(FRichCurveKey(Times[Index], FootMovement::SpeedToPlantedWeight(Speeds[Index], PlantedSpeed, MovingSpeed)));
		Key.InterpMode = RCIM_Linear;
	}

	IAnimationDataController& Controller = Sequence->GetController();
	const FAnimationCurveIdentifier CurveId(CurveName, ERawCurveTrackTypes::RCT_Float);

	Controller.OpenBracket(LOCTEXT("DetectFootBoneMovement", "Detect Foot Bone Movement"));
	if (Sequence->GetDataModel()->FindCurve(CurveId))
	{
		Controller.RemoveCurve(CurveId);
	}
	Controller.AddCurve(CurveId);
	Controller.SetCurveKeys(CurveId, Keys);
	Controller.CloseBracket();

	return true;
}

#undef LOCTEXT_NAMESPACE
