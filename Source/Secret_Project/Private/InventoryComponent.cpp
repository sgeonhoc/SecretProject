#include "InventoryComponent.h"
#include "StatComponent.h"

UInventoryComponent::UInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

const TArray<FConsumableDef>& UInventoryComponent::GetCatalog()
{
    static TArray<FConsumableDef> Catalog;
    if (Catalog.Num() == 0)
    {
        auto Make = [](const TCHAR* Id, const TCHAR* Name, EConsumableEffect Eff, float Mag, const TCHAR* Desc)
        {
            FConsumableDef D;
            D.Id = Id; D.Name = Name; D.Effect = Eff; D.Magnitude = Mag; D.Description = Desc;
            return D;
        };
        Catalog.Add(Make(TEXT("HealPotion"), TEXT("회복약"),      EConsumableEffect::HealHP,      50.f,
            TEXT("약국에서 흔히 파는 상처약. 이면(裏面)에선 왜인지 베인 데까지 아문다. 깊게 묻지 않는 편이 좋다.")));
        Catalog.Add(Make(TEXT("HiPotion"),   TEXT("고급 회복약"), EConsumableEffect::HealHP,      150.f,
            TEXT("성분은 평범한 회복약과 같다는데, 효과는 곱절. 정 선생님이 '믿음이 절반'이라며 웃었다.")));
        Catalog.Add(Make(TEXT("SPPotion"),   TEXT("SP 회복약"),   EConsumableEffect::HealSP,      30.f,
            TEXT("머리가 맑아지는 한 모금. 집중력이 돌아온다 = 페르소나를 부를 기력이 찬다.")));
        Catalog.Add(Make(TEXT("Antidote"),   TEXT("해독제"),      EConsumableEffect::CureAilment, 0.f,
            TEXT("독·마비·화상… 몸에 깃든 나쁜 상태를 씻어낸다. 수의사 선우가 동물용으로도 쓴다고.")));
        Catalog.Add(Make(TEXT("Elixir"),     TEXT("종합 영양제"), EConsumableEffect::FullHeal,    0.f,
            TEXT("'이거 하나면 다 된다'는 광고는 보통 거짓말이지만 — 이 녀석만은 진짜다. HP·SP·상태이상 전부 회복.")));
        Catalog.Add(Make(TEXT("OldKey"),     TEXT("녹슨 열쇠"),   EConsumableEffect::KeyItem,     0.f,
            TEXT("어디 것인지 모를 낡은 열쇠. 잠긴 문 하나를 여는 데 쓰면 사라진다. 들고 있으면 묘하게 손이 시리다.")));
        // 채집물 — HarvestNode에서 획득 (현대 배경: 편의점/자판기 간식류)
        Catalog.Add(Make(TEXT("Herb"),       TEXT("에너지바"),    EConsumableEffect::HealHP,      30.f,
            TEXT("편의점 매대 끝에 늘 있는 그것. 맛은 그럭저럭, 그래도 한 입이면 다시 뛸 힘이 난다.")));
        Catalog.Add(Make(TEXT("ManaFlower"), TEXT("에너지 드링크"), EConsumableEffect::HealSP,    20.f,
            TEXT("'밤샘의 친구'. 카페인의 힘인지 마음의 힘인지, 한 캔이면 정신이 번쩍 든다. 너무 자주는 금물.")));
    }
    return Catalog;
}

bool UInventoryComponent::FindDef(FName Id, FConsumableDef& OutDef)
{
    for (const FConsumableDef& D : GetCatalog())
    {
        if (D.Id == Id)
        {
            OutDef = D;
            return true;
        }
    }
    return false;
}

void UInventoryComponent::AddItem(FName Id, int32 Count)
{
    if (Id.IsNone() || Count <= 0) return;

    for (FItemStack& S : Stacks)
    {
        if (S.Id == Id)
        {
            S.Count += Count;
            return;
        }
    }
    FItemStack NewStack;
    NewStack.Id = Id;
    NewStack.Count = Count;
    Stacks.Add(NewStack);
}

bool UInventoryComponent::RemoveItem(FName Id, int32 Count)
{
    if (Count <= 0) return false;

    for (int32 i = 0; i < Stacks.Num(); ++i)
    {
        if (Stacks[i].Id == Id)
        {
            if (Stacks[i].Count < Count) return false; // 부족 → 차감 안 함
            Stacks[i].Count -= Count;
            if (Stacks[i].Count <= 0)
                Stacks.RemoveAt(i);
            return true;
        }
    }
    return false;
}

int32 UInventoryComponent::GetCount(FName Id) const
{
    for (const FItemStack& S : Stacks)
    {
        if (S.Id == Id) return S.Count;
    }
    return 0;
}

bool UInventoryComponent::UseItemOn(UStatComponent* Target, FName Id)
{
    if (!Target || Target->IsDead()) return false;
    if (GetCount(Id) <= 0) return false;

    FConsumableDef Def;
    if (!FindDef(Id, Def)) return false;

    // 효과 적용 (StatComponent의 public 함수만 사용)
    switch (Def.Effect)
    {
    case EConsumableEffect::HealHP:
        Target->Heal(Def.Magnitude);
        break;
    case EConsumableEffect::HealSP:
        Target->RestoreSP(Def.Magnitude);
        break;
    case EConsumableEffect::FullHeal:
        Target->FullRestore();
        break;
    case EConsumableEffect::CureAilment:
        Target->CureAilment();
        break;
    case EConsumableEffect::KeyItem:
        return false; // 열쇠는 '사용'으로 소모 불가 (문 등에서만 소모)
    }

    RemoveItem(Id, 1);
    return true;
}
