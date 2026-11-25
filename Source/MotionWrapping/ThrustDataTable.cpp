#include "ThrustDataTable.h"
#include "ThrustLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogThrustDataTable, Log, All);

void UThrustDataTable::PostInitProperties()
{
	Super::PostInitProperties();

	// Fired after every change, AddRow and RemoveRow included, so the Blueprint node AddDataTableRow triggers it too
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnDataTableChanged().AddUObject(this, &UThrustDataTable::HandleThrustTableChanged);
	}
}

void UThrustDataTable::HandleThrustTableChanged()
{
	RecomputeThrustData();
}

bool UThrustDataTable::RecomputeThrustData()
{
	AveragePosition = FVector::ZeroVector;
	Convergence = FVector::ZeroVector;

	if (!RowStruct || !RowStruct->IsChildOf(FThrustData::StaticStruct()))
	{
		UE_LOG(LogThrustDataTable, Warning, TEXT("%s: the row struct must be FThrustData."), *GetName());
		return false;
	}

	TArray<FThrustData*> Rows;
	Rows.Reserve(GetRowMap().Num());
	for (const TPair<FName, uint8*>& Pair : GetRowMap())
	{
		FThrustData* Row = reinterpret_cast<FThrustData*>(Pair.Value);
		Row->BlendSpacePosition = FVector2D::ZeroVector;
		Rows.Add(Row);
	}

	// The table is filled row by row, so having too few rows yet isn't worth a warning
	if (Rows.Num() < 2)
	{
		return false;
	}

	TArray<FThrustData> Thrusts;
	Thrusts.Reserve(Rows.Num());
	FVector PositionSum = FVector::ZeroVector;
	for (const FThrustData* Row : Rows)
	{
		Thrusts.Add(*Row);
		PositionSum += Row->Position;
	}

	// Warns on its own when the lines are parallel or too few have a direction
	FVector Point;
	if (!UThrustLibrary::ComputeThrustIntersection(Thrusts, Point))
	{
		return false;
	}

	const FVector Average = PositionSum / Rows.Num();
	AveragePosition = Average;
	Convergence = Point;

	const FVector FromAverage = (Point - Average).GetSafeNormal();
	if (FromAverage.IsZero())
	{
		UE_LOG(LogThrustDataTable, Warning, TEXT("%s: the convergence point is on the average position, the blend space positions are undefined."), *GetName());
		return false;
	}

	bool bAllRowsValid = true;
	for (const TPair<FName, uint8*>& Pair : GetRowMap())
	{
		FThrustData* Row = reinterpret_cast<FThrustData*>(Pair.Value);
		const FVector FromPosition = (Point - Row->Position).GetSafeNormal();
		if (FromPosition.IsZero())
		{
			UE_LOG(LogThrustDataTable, Warning, TEXT("%s: row %s is on the convergence point, its blend space position is undefined."), *GetName(), *Pair.Key.ToString());
			bAllRowsValid = false;
			continue;
		}

		const FVector Cross = FromAverage ^ FromPosition;
		Row->BlendSpacePosition = FVector2D(Cross.Z, Cross.X);
	}
	return bAllRowsValid;
}
