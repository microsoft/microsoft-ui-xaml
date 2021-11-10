// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SampleControl.h"
#include "ResourceAccessor.h"

SampleControl::SampleControl()
{
    SetDefaultStyleKey(this);
}

void SampleControl::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };
}

void  SampleControl::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
}
