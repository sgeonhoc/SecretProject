#include "CombatStyleBase.h"

int32 UCombatStyleBase::FindMatchedCombo(const TArray<EBoxingInput>& Buffer) const
{
    for (int32 i = 0; i < ComboDefs.Num(); i++)
    {
        const TArray<EBoxingInput>& Seq = ComboDefs[i].InputSequence;
        if (Seq.Num() != Buffer.Num()) continue;

        bool bMatch = true;
        for (int32 j = 0; j < Seq.Num(); j++)
        {
            if (Seq[j] != Buffer[j]) { bMatch = false; break; }
        }
        if (bMatch) return i;
    }
    return -1;
}

bool UCombatStyleBase::IsValidComboPrefix(const TArray<EBoxingInput>& Buffer) const
{
    if (Buffer.Num() == 0) return false;

    for (const FComboDefinition& Combo : ComboDefs)
    {
        if (Buffer.Num() > Combo.InputSequence.Num()) continue;

        bool bPrefix = true;
        for (int32 i = 0; i < Buffer.Num(); i++)
        {
            if (Combo.InputSequence[i] != Buffer[i]) { bPrefix = false; break; }
        }
        if (bPrefix) return true;
    }
    return false;
}
