#include "ThrustLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogThrust, Log, All);

namespace Thrust
{
	// Cofactors of the symmetric matrix [XX XY XZ; XY YY YZ; XZ YZ ZZ], symmetric as well. Divided by the
	// determinant they give the inverse.
	struct FCofactors
	{
		double C00, C01, C02, C11, C12, C22;

		FCofactors(double XX, double XY, double XZ, double YY, double YZ, double ZZ)
			: C00(YY * ZZ - YZ * YZ)
			, C01(XZ * YZ - XY * ZZ)
			, C02(XY * YZ - XZ * YY)
			, C11(XX * ZZ - XZ * XZ)
			, C12(XY * XZ - XX * YZ)
			, C22(XX * YY - XY * XY)
		{
		}

		FVector Transform(const FVector& V) const
		{
			return FVector(
				C00 * V.X + C01 * V.Y + C02 * V.Z,
				C01 * V.X + C11 * V.Y + C12 * V.Z,
				C02 * V.X + C12 * V.Y + C22 * V.Z);
		}
	};
}

bool UThrustLibrary::ComputeThrustIntersection(const TArray<FThrustData>& Thrusts, FVector& OutPoint)
{
	OutPoint = FVector::ZeroVector;

	// Each line contributes its projector onto the plane perpendicular to it, P = I - D * D^T, and the point
	// minimizing the summed squared distances solves (sum P) * X = sum (P * Position). Sum P is symmetric.
	double XX = 0.0, XY = 0.0, XZ = 0.0, YY = 0.0, YZ = 0.0, ZZ = 0.0;
	FVector B = FVector::ZeroVector;
	int32 NumLines = 0;
	for (const FThrustData& Thrust : Thrusts)
	{
		const FVector& Position = Thrust.Position;
		const FVector D = Thrust.Direction.GetSafeNormal();
		if (D.IsZero())
		{
			continue;
		}

		XX += 1.0 - D.X * D.X; XY -= D.X * D.Y; XZ -= D.X * D.Z;
		YY += 1.0 - D.Y * D.Y; YZ -= D.Y * D.Z; ZZ += 1.0 - D.Z * D.Z;
		B += Position - D * (D | Position);
		++NumLines;
	}

	const Thrust::FCofactors Cofactors(XX, XY, XZ, YY, YZ, ZZ);
	const double Determinant = XX * Cofactors.C00 + XY * Cofactors.C01 + XZ * Cofactors.C02;
	if (NumLines < 2 || FMath::Abs(Determinant) < UE_DOUBLE_SMALL_NUMBER)
	{
		UE_LOG(LogThrust, Warning, TEXT("ComputeThrustIntersection: need at least two non-parallel lines (got %d usable)."), NumLines);
		return false;
	}

	OutPoint = Cofactors.Transform(B) / Determinant;
	return true;
}
