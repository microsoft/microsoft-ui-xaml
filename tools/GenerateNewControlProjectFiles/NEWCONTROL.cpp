// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NEWCONTROL.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

NEWCONTROL::NEWCONTROL()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NEWCONTROL);

    SetDefaultStyleKey(this);
}

void NEWCONTROL::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  NEWCONTROL::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
