// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

InfoBar::InfoBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBar);

    SetDefaultStyleKey(this);
}

void InfoBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  InfoBar::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
