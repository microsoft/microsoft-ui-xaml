// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusableHelper.h"
#include <Hyperlink.h>
#include "TextPointerWrapper.h"
// statics

IFocusable* CFocusableHelper::GetIFocusableForDO(_In_ CDependencyObject* cdo)
{
    if (CHyperlink* hyperlink = do_pointer_cast<CHyperlink>(cdo))
    {
        return hyperlink->GetIFocusable();
    }
    return nullptr;
}

bool const CFocusableHelper::IsFocusableDO(_In_ CDependencyObject* cdo)
{
    if (cdo->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        return true;
    }
    return false;
}

CFrameworkElement* CFocusableHelper::GetContainingFrameworkElementIfFocusable(_In_ CDependencyObject* cdo)
{
    if (cdo->OfTypeByIndex<KnownTypeIndex::Hyperlink>())
    {
        CFrameworkElement *pContentStartVisualParent = nullptr;
        xref_ptr<CTextPointerWrapper> spContentStart;
        CTextElement *pTextElement = static_cast<CTextElement*>(cdo);
        IFCFAILFAST(pTextElement->GetContentStart(spContentStart.ReleaseAndGetAddressOf()));
        IFCFAILFAST(spContentStart->GetVisualParent(&pContentStartVisualParent));
        if (pContentStartVisualParent)
        {
            return pContentStartVisualParent;
        }
        else
        {
            return pTextElement->GetContainingFrameworkElement();
        }
    }
    return do_pointer_cast<CFrameworkElement>(cdo);
}

// Constructors

CFocusableHelper::CFocusableHelper(_In_ CHyperlink* hl)
{
    m_hyperlink = hl;
}


CFocusableHelper::~CFocusableHelper()
{
}

CDependencyObject* CFocusableHelper::GetDOForIFocusable()
{
    return m_hyperlink;
}

bool CFocusableHelper::IsFocusable()
{
    return m_hyperlink->IsFocusable();
}

KnownPropertyIndex CFocusableHelper::GetElementSoundModePropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_ElementSoundMode;
}

KnownPropertyIndex CFocusableHelper::GetFocusStatePropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_FocusState;
}

bool CFocusableHelper::GetIsTabStop()
{
    bool isTabStop = true; // default value
    CValue result;
    VERIFYHR(m_hyperlink->GetValueByIndex(KnownPropertyIndex::Hyperlink_IsTabStop, &result));
    isTabStop = result.AsBool();
    return isTabStop;
}

int CFocusableHelper::GetTabIndex()
{
    int tabIndex;
    CValue result;
    VERIFYHR(m_hyperlink->GetValueByIndex(KnownPropertyIndex::Hyperlink_TabIndex, &result));
    tabIndex = result.AsSigned();
    return tabIndex;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusDownPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusDown;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusDownNavigationStrategyPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusDownNavigationStrategy;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusLeftPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusLeft;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusLeftNavigationStrategyPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusLeftNavigationStrategy;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusRightPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusRight;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusRightNavigationStrategyPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusRightNavigationStrategy;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusUpPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusUp;
}

KnownPropertyIndex CFocusableHelper::GetXYFocusUpNavigationStrategyPropertyIndex()
{
    return KnownPropertyIndex::Hyperlink_XYFocusUpNavigationStrategy;
}

KnownEventIndex CFocusableHelper::GetLostFocusEventIndex()
{
    return KnownEventIndex::Hyperlink_LostFocus;
}

KnownEventIndex CFocusableHelper::GetGotFocusEventIndex()
{
    return KnownEventIndex::Hyperlink_GotFocus;
}

