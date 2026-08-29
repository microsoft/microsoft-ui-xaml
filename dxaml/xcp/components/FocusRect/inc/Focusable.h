// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "FocusRectHost.h"

class CDependencyObject;
class CFrameworkElement;
class CUIElement;

// Light weight helper class that represents a focusable object.  Makes the logic in
// the client code clearer because it can avoid so many if/else cases
struct Focusable
{
    Focusable(_In_ CDependencyObject* obj)
    {
        Attach(obj);
    }

    void Attach(_In_ CDependencyObject* obj);

    CDependencyObject* GuaranteedFocusable() const;
    CFrameworkElement* AsFrameworkElement() const;

    bool IsNull() const
    {
        return Object == nullptr;
    }

    bool SupportsRevealFocus() const
    {
        return IsFrameworkElement();
    }

    XRECTF GetBounds(_Inout_opt_ std::vector<XRECTF>* multipleBounds = nullptr) const;
    CUIElement* GetElementResponsibleForDrawingThisElement() const;

    bool IsFrameworkElement() const { return m_isFrameworkElement; }
    bool IsFocusableElement() const { return m_isFocusableElement; }

    CDependencyObject* Object;

private:
    static void GetBoundsCollectionForFocusable(
        _In_ CDependencyObject* focusable,
        _Out_ XRECTF* totalBounds,
        _Inout_opt_ std::vector<XRECTF>* boundsVector);

    bool m_isFrameworkElement:1;
    bool m_isFocusableElement:1;
};
