#include "ThrustDataTableFactory.h"
#include "AssetTypeCategories.h"
#include "ThrustDataTable.h"

#define LOCTEXT_NAMESPACE "ThrustDataTableFactory"

UThrustDataTableFactory::UThrustDataTableFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UThrustDataTable::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
	Struct = FThrustData::StaticStruct();
}

bool UThrustDataTableFactory::ConfigureProperties()
{
	// The row struct is fixed, skip the struct picker
	Struct = FThrustData::StaticStruct();
	return true;
}

FText UThrustDataTableFactory::GetDisplayName() const
{
	return LOCTEXT("DisplayName", "Thrust Data Table");
}

uint32 UThrustDataTableFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}

UDataTable* UThrustDataTableFactory::MakeNewDataTable(UObject* InParent, FName Name, EObjectFlags Flags)
{
	return NewObject<UThrustDataTable>(InParent, Name, Flags);
}

#undef LOCTEXT_NAMESPACE
