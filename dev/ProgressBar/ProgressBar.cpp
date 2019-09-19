// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressBar::ProgressBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressBar);

    SetDefaultStyleKey(this);
}

void ProgressBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  ProgressBar::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
