#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ThrustDataTable.h"
#include "ThrustLibrary.generated.h"

UCLASS()
class MOTIONWRAPPING_API UThrustLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Finds the point closest to all the thrust lines, minimizing the sum of squared distances to them.
	 * Each line passes through Position along Direction, which doesn't need to be normalized.
	 *
	 * @param Thrusts		Thrust lines, entries with a zero direction are ignored
	 * @param OutPoint		Least-squares intersection of the lines, zero on failure
	 * @return				False if fewer than two lines remain or they are all parallel
	 */
	UFUNCTION(BlueprintPure, Category = "Motion Wrapping|Thrust")
	static bool ComputeThrustIntersection(const TArray<FThrustData>& Thrusts, FVector& OutPoint);
};
