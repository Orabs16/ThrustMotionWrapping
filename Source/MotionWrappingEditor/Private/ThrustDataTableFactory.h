#pragma once

#include "CoreMinimal.h"
#include "Factories/DataTableFactory.h"
#include "ThrustDataTableFactory.generated.h"

/** Creates UThrustDataTable assets, always with FThrustData rows. */
UCLASS(hidecategories = Object)
class UThrustDataTableFactory : public UDataTableFactory
{
	GENERATED_BODY()

public:
	UThrustDataTableFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual FText GetDisplayName() const override;
	virtual uint32 GetMenuCategories() const override;

protected:
	virtual UDataTable* MakeNewDataTable(UObject* InParent, FName Name, EObjectFlags Flags) override;
};
