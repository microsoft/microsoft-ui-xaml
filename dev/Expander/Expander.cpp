// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Expander.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

Expander::Expander()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Expander);

    SetDefaultStyleKey(this);
}

void Expander::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  Expander::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
