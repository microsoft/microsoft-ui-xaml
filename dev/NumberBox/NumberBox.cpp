// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

NumberBox::NumberBox()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NumberBox);

    SetDefaultStyleKey(this);
}

void NumberBox::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  NumberBox::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    
    // TODO: Implement
}
