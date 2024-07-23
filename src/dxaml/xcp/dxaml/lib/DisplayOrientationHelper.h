// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace XamlDisplay
{
    enum class Orientation
    {
        None,
        Landscape,
        Portrait,
        LandscapeFlipped,
        PortraitFlipped
    };

    // Xaml needs to use different APIs in order to determine the display orientation depending on what context it is running in.
    // UWP apps make use of the DisplayInformation type while Win32 apps need to use the HMONITOR and HWIND APIs.
    _Check_return_ HRESULT 
        GetDisplayOrientation(_In_ CDependencyObject* dependencyObject, _Out_ Orientation& orientation);

    _Check_return_ HRESULT 
        GetIslandDisplayOrientation(_In_ CDependencyObject* dependencyObject, _Out_ ixp::ContentDisplayOrientations& rotationValue);

    _Check_return_ HRESULT 
        GetUWPDisplayInformation(_Out_ ctl::ComPtr<wgrd::IDisplayInformation>& displayInformation);

}