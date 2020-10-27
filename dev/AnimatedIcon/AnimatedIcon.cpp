// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnimatedIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

AnimatedIcon::AnimatedIcon()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_AnimatedIcon);

    SetDefaultStyleKey(this);
}

void AnimatedIcon::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  AnimatedIcon::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
