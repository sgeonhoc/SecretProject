#include "MenuActionButton.h"

void UMenuActionButton::Init(int32 InIndex, FName InTag)
{
    ActionIndex = InIndex;
    ActionTag = InTag;
    if (!bBound)
    {
        OnClicked.AddDynamic(this, &UMenuActionButton::HandleClicked);
        OnHovered.AddDynamic(this, &UMenuActionButton::HandleHovered);
        OnUnhovered.AddDynamic(this, &UMenuActionButton::HandleUnhovered);
        bBound = true;
    }
}

void UMenuActionButton::HandleClicked()
{
    OnMenuClicked.Broadcast(ActionIndex, ActionTag);
}

void UMenuActionButton::HandleHovered()
{
    OnMenuHover.Broadcast(ActionIndex, ActionTag, true);
}

void UMenuActionButton::HandleUnhovered()
{
    OnMenuHover.Broadcast(ActionIndex, ActionTag, false);
}
