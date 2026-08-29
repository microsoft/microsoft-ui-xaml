#pragma once

#include "ItemContainerInvokedEventArgs.g.h"

class ItemContainerInvokedEventArgs :
    public ReferenceTracker<ItemContainerInvokedEventArgs, winrt::implementation::ItemContainerInvokedEventArgsT, winrt::composable, winrt::composing>
{
public:
    ItemContainerInvokedEventArgs(const winrt::ItemContainerInteractionTrigger& interactionTrigger, const winrt::IInspectable& originalSource);

#pragma region IItemContainerInvokedEventArgs
    winrt::IInspectable OriginalSource();
    winrt::ItemContainerInteractionTrigger InteractionTrigger();
    bool Handled();
    void Handled(bool value);
#pragma endregion

private:
    tracker_ref<winrt::IInspectable> m_originalSource{ this };
    winrt::ItemContainerInteractionTrigger m_interactionTrigger{};
    bool m_handled{ false };
};
