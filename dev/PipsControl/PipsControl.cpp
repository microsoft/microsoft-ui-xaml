// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "PipsControl.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

PipsControl::PipsControl()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PipsControl);

    SetDefaultStyleKey(this);
}

void PipsControl::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void PipsControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
