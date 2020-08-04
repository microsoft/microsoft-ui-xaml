// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Pager.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

Pager::Pager()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Pager);

    SetDefaultStyleKey(this);
}

void Pager::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  Pager::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
