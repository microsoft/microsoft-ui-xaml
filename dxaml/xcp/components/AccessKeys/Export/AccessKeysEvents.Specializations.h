// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <CDependencyObject.h>
#include <UIElement.h>
#include <TextElement.h>
#include "AccessKeysEvents.h"

template <>
inline bool AccessKeys::AKOwnerEvents::InvokeEvent<CDependencyObject> (_In_ CDependencyObject* const pElement)
{
    bool handled = false;

    if(pElement->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement *pUIElement = static_cast<CUIElement*>(pElement);
        handled = pUIElement->RaiseAccessKeyInvoked();
    }
    else if (pElement->OfTypeByIndex<KnownTypeIndex::TextElement>())
    {
        CTextElement *pTextElement = static_cast<CTextElement*>(pElement);
        handled = pTextElement->RaiseAccessKeyInvoked();
    }

    return handled;
}

template <>
inline void AccessKeys::AKOwnerEvents::RaiseAccessKeyShown<CDependencyObject> (_In_ CDependencyObject* const pElement, _In_ const wchar_t* const pressedKeys)
{
    if (pElement->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement *pUIElement = static_cast<CUIElement*>(pElement);
        pUIElement->RaiseAccessKeyShown(pressedKeys);
    }
    else if (pElement->OfTypeByIndex<KnownTypeIndex::TextElement>())
    {
        CTextElement *pTextElement = static_cast<CTextElement*>(pElement);
        pTextElement->RaiseAccessKeyShown(pressedKeys);
    }
}

template <>
inline void AccessKeys::AKOwnerEvents::RaiseAccessKeyHidden<CDependencyObject> (_In_ CDependencyObject* const pElement)
{
    if(pElement->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement *pUIElement = static_cast<CUIElement*>(pElement);
        pUIElement->RaiseAccessKeyHidden();
    }
    else if (pElement->OfTypeByIndex<KnownTypeIndex::TextElement>())
    {
        CTextElement *pTextElement = static_cast<CTextElement*>(pElement);
        pTextElement->RaiseAccessKeyHidden();
    }
}

