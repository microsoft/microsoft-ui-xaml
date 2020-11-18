// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ImageIcon.g.h"
#include "ImageIcon.properties.h"

class ImageIcon :
    public ReferenceTracker<ImageIcon, DeriveFromBitmapIconHelper_base, winrt::ImageIcon>,
    public ImageIconProperties
{

public:
    ImageIcon();
    ~ImageIcon() {}

    void OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    // IFrameworkElement
    void OnApplyTemplate();

    void OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:

    tracker_ref<winrt::Image> m_rootImage{ this };
};
