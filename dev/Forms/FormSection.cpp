// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "FormSection.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Vector.h"
#include "VectorIterator.h"

FormSection::FormSection()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_Forms);

    auto items = winrt::make<ObservableVector<winrt::FrameworkElement>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);
}

void FormSection::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void FormSection::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
