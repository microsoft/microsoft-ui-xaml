// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "paltypes.h"
#include "Indexes.g.h"

namespace AccessKeys
{
    static wchar_t ALT = (wchar_t)18;
    static wchar_t ESC = (wchar_t)27;

    template<class Element = CDependencyObject>
    struct AKInvokeReturnParams
    {
        bool invokeAttempted = false; // When we've found an element within the Scope to invoke, we set this to true
        bool invokeFoundValidPattern = true; // When we try to invoke the element, but we were unable to find a pattern, we set this to false
        xref::weakref_ptr<Element> invokedElement; // The element we are trying to invoke
    };

    template<class Element=CDependencyObject,typename CValue=CValue>
    xstring_ptr GetAccessKey(_In_ Element* const element)
    {
        CValue value;
        xstring_ptr accessKey;

        if (element->template OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::UIElement_AccessKey, &value)))
            {
                accessKey = value.AsString();
            }
        }
        else if (element->template OfTypeByIndex<KnownTypeIndex::TextElement>())
        {
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::TextElement_AccessKey, &value)))
            {
                accessKey = value.AsString();
            }
        }

        return std::move(accessKey);
    }

    inline bool IsLeftAltKey(_In_ const InputMessage* const inputMessage)
    {
        return inputMessage->m_platformKeyCode == wsy::VirtualKey::VirtualKey_Menu /*e.g. Alt key*/ && !inputMessage->m_physicalKeyStatus.m_bIsExtendedKey;
    }

    inline bool IsMenuKeyDown(_In_ const InputMessage* const inputMessage)
    {
        // return if the message has alt pressed
        return inputMessage->m_physicalKeyStatus.m_bIsMenuKeyDown && !inputMessage->m_physicalKeyStatus.m_bWasKeyDown;
    }

    template<class Element = CDependencyObject, typename CValue = CValue>
    bool DismissOnInvoked(_In_ Element* const element)
    {
        CValue value;
        bool dismissOnInvoked = false;

        if (element->template OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::UIElement_ExitDisplayModeOnAccessKeyInvoked, &value)))
            {
                dismissOnInvoked = value.AsBool();
            }
        }
        else if (element->template OfTypeByIndex<KnownTypeIndex::TextElement>())
        {
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::TextElement_ExitDisplayModeOnAccessKeyInvoked, &value)))
            {
                dismissOnInvoked = value.AsBool();
            }
        }

        return dismissOnInvoked;
    }

    template<class Element = CDependencyObject>
    inline bool IsValidAKOwnerType(_In_ Element* const element)
    {
        return element->template OfTypeByIndex<KnownTypeIndex::FlyoutBase>() || element->template OfTypeByIndex<KnownTypeIndex::UIElement>() || element->template OfTypeByIndex<KnownTypeIndex::TextElement>();
    }

    template<class Element = CDependencyObject, typename CValue = CValue>
    static bool IsAccessKeyScope(_In_ Element* element)
    {
        if (element->template OfTypeByIndex<KnownTypeIndex::FlyoutBase>())
        {
            // Any flyout must always owns its Scope.
            // Besides, moving the property IsAccessKeyScope to CDependencyObject is expensive
            return true;
        }

        KnownPropertyIndex index = KnownPropertyIndex::UnknownType_UnknownProperty;

        if (element->template OfTypeByIndex<KnownTypeIndex::UIElement>()) { index = KnownPropertyIndex::UIElement_IsAccessKeyScope; }
        else if (element->template OfTypeByIndex<KnownTypeIndex::TextElement>()) { index = KnownPropertyIndex::TextElement_IsAccessKeyScope; }

        if (index == KnownPropertyIndex::UnknownType_UnknownProperty) { return false; }

        CValue result;
        VERIFYHR(element->GetValueByIndex(index, &result));
        return result.AsBool();
    }

}

// Uncomment and rebuild to enable AccessKey debug spew:
//#define AK_DBG

#ifdef AK_DBG
#define AK_TRACE(...) AK_Trace(__VA_ARGS__)

inline void AK_Trace(const wchar_t* format, ...)
{
    wchar_t buffer[4096];
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, ARRAYSIZE(buffer), format, args);
    va_end(args);

    ::OutputDebugString(buffer);
}

#else
#define AK_TRACE(...)
#endif
