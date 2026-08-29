// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IFocusable.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CHyperlink;

class CFocusableHelper : public IFocusable
{

public:
    static IFocusable* GetIFocusableForDO(_In_ CDependencyObject* cdo);
    static const bool IsFocusableDO(_In_ CDependencyObject* cdo);
    static CFrameworkElement* GetContainingFrameworkElementIfFocusable(_In_ CDependencyObject* cdo);


    CDependencyObject* GetDOForIFocusable() override;

    CFocusableHelper(_In_ CHyperlink* hl);
    ~CFocusableHelper() override;

    bool IsFocusable() override;

    KnownPropertyIndex GetElementSoundModePropertyIndex() override;
    KnownPropertyIndex GetFocusStatePropertyIndex() override;
    bool GetIsTabStop() override;
    int GetTabIndex() override;

    KnownPropertyIndex GetXYFocusDownPropertyIndex() override;
    KnownPropertyIndex GetXYFocusDownNavigationStrategyPropertyIndex() override;
    KnownPropertyIndex GetXYFocusLeftPropertyIndex() override;
    KnownPropertyIndex GetXYFocusLeftNavigationStrategyPropertyIndex() override;
    KnownPropertyIndex GetXYFocusRightPropertyIndex() override;
    KnownPropertyIndex GetXYFocusRightNavigationStrategyPropertyIndex() override;
    KnownPropertyIndex GetXYFocusUpPropertyIndex() override;
    KnownPropertyIndex GetXYFocusUpNavigationStrategyPropertyIndex() override;

    KnownEventIndex GetLostFocusEventIndex() override;
    KnownEventIndex GetGotFocusEventIndex() override;


private:
    CHyperlink* m_hyperlink;

    CFocusableHelper(const CFocusableHelper&) = delete;
    CFocusableHelper& operator=(const CFocusableHelper&) = delete;

};