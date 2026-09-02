#include "Core/ReclaimM4CanonLibrary.h"

#include "UObject/UnrealType.h"

namespace ReclaimM4CanonPrivate
{
    struct FCanonicalScalingRow
    {
        int32 EffectivePlayers;
        float ThreatBudgetScalar;
        float EnemyHealthScalar;
        int32 SpecialUnitCap;
    };

    static constexpr FCanonicalScalingRow Rows[] =
    {
        {1, 1.00f, 1.00f, 1},
        {2, 1.55f, 1.05f, 2},
        {3, 2.05f, 1.08f, 3},
        {4, 2.45f, 1.10f, 4},
    };

    static bool SetIntField(UStruct* Struct, void* Data, const TCHAR* Name, int32 Value)
    {
        if (FIntProperty* Property = FindFProperty<FIntProperty>(Struct, Name))
        {
            Property->SetPropertyValue_InContainer(Data, Value);
            return true;
        }
        return false;
    }

    static bool SetFloatField(UStruct* Struct, void* Data, const TCHAR* Name, float Value)
    {
        if (FFloatProperty* Property = FindFProperty<FFloatProperty>(Struct, Name))
        {
            Property->SetPropertyValue_InContainer(Data, Value);
            return true;
        }
        return false;
    }
}

bool UReclaimM4CanonLibrary::ApplyM4DirectorScaling(UObject* AIConfigObject)
{
    if (!AIConfigObject)
    {
        return false;
    }

    FArrayProperty* ArrayProperty = FindFProperty<FArrayProperty>(AIConfigObject->GetClass(), TEXT("PlayerScaling"));
    FStructProperty* StructProperty = ArrayProperty ? CastField<FStructProperty>(ArrayProperty->Inner) : nullptr;
    if (!ArrayProperty || !StructProperty || !StructProperty->Struct)
    {
        UE_LOG(LogTemp, Error, TEXT("[Reclaim M4 Canon] AIConfig.PlayerScaling was not found or is not a struct array."));
        return false;
    }

    FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(AIConfigObject));
    Helper.Resize(UE_ARRAY_COUNT(ReclaimM4CanonPrivate::Rows));

    bool bSuccess = true;
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(ReclaimM4CanonPrivate::Rows); ++Index)
    {
        const ReclaimM4CanonPrivate::FCanonicalScalingRow& Source = ReclaimM4CanonPrivate::Rows[Index];
        void* RowData = Helper.GetRawPtr(Index);

        bSuccess &= ReclaimM4CanonPrivate::SetIntField(
            StructProperty->Struct, RowData, TEXT("EffectivePlayers"), Source.EffectivePlayers);
        bSuccess &= ReclaimM4CanonPrivate::SetFloatField(
            StructProperty->Struct, RowData, TEXT("ThreatBudgetScalar"), Source.ThreatBudgetScalar);
        bSuccess &= ReclaimM4CanonPrivate::SetFloatField(
            StructProperty->Struct, RowData, TEXT("EnemyHealthScalar"), Source.EnemyHealthScalar);
        bSuccess &= ReclaimM4CanonPrivate::SetIntField(
            StructProperty->Struct, RowData, TEXT("SpecialUnitCap"), Source.SpecialUnitCap);
    }

#if WITH_EDITOR
    AIConfigObject->Modify();
    AIConfigObject->MarkPackageDirty();
    AIConfigObject->PostEditChange();
#endif

    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("[Reclaim M4 Canon] One or more PlayerScaling struct fields were not found."));
    }

    return bSuccess;
}
