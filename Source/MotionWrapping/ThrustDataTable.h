#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ThrustDataTable.generated.h"

/** One thrust line, a row of UThrustDataTable. */
USTRUCT(BlueprintType)
struct MOTIONWRAPPING_API FThrustData : public FTableRowBase
{
	GENERATED_BODY()

	/** Point the thrust line passes through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrust")
	FVector Position = FVector::ZeroVector;

	/** Direction of the thrust line, doesn't need to be normalized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Thrust")
	FVector Direction = FVector::ZeroVector;

	/**
	 * Coordinates of this thrust in the blend space, computed by the table. With V the cross product of
	 * the unit vectors from the average position and from Position to the convergence point, X is V.Z and Y is V.X.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thrust", meta = (DisplayName = "BS Position"))
	FVector2D BlendSpacePosition = FVector2D::ZeroVector;
};

/**
 * Data table of FThrustData rows that keeps the table-wide thrust data and each row's BlendSpacePosition
 * up to date. They are recomputed whenever the table changes: rows added, removed or edited, or the table reimported.
 */
UCLASS(BlueprintType)
class MOTIONWRAPPING_API UThrustDataTable : public UDataTable
{
	GENERATED_BODY()

public:
	/** Average of the row positions, zero if there are fewer than two rows. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thrust")
	FVector AveragePosition = FVector::ZeroVector;

	/** Least-squares intersection of the row thrust lines, zero if it can't be computed. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thrust")
	FVector Convergence = FVector::ZeroVector;

	virtual void PostInitProperties() override;

	/**
	 * Recomputes AveragePosition, Convergence and every row's BlendSpacePosition. Called automatically when the table changes.
	 * Values that can't be computed are set to zero.
	 *
	 * @return	False if the row struct isn't FThrustData, if there are fewer than two usable rows or their directions are all
	 *			parallel, if the convergence point is on the average position, or if a row's position is on the convergence point
	 */
	UFUNCTION(BlueprintCallable, Category = "Motion Wrapping|Thrust")
	bool RecomputeThrustData();

private:
	void HandleThrustTableChanged();
};
