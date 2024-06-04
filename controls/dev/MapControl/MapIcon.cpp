// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MapIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "MapControl.h"
#include "MapElementsLayer.h"

MapIcon::MapIcon()
{
}

void  MapIcon::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    if (property == s_LocationProperty)
    {
        auto map = SharedHelpers::GetAncestorOfType<winrt::MapControl>(winrt::VisualTreeHelper::GetParent(*this));
        auto layer = SharedHelpers::GetAncestorOfType<winrt::MapElementsLayer>(winrt::VisualTreeHelper::GetParent(*this));
        if (map && layer)
        {
            winrt::get_self<MapControl>(map)->UpdateMapIcon(Location(), Id,  winrt::get_self<MapElementsLayer>(layer)->Id);
        }
    }
}