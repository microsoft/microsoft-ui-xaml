// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  ProgressRing::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
