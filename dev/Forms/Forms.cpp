// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Forms.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

Forms::Forms()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Forms);

    SetDefaultStyleKey(this);
}

void Forms::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  Forms::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
