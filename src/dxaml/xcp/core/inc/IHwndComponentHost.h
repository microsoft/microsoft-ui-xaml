// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// This interface is used on a Xaml element that is hosting a non-Xaml component containing hwnds, such as WebView2.
//
MIDL_INTERFACE("2EE738F4-402C-4F6E-9F93-DEDD89F09B49")
IHwndComponentHost : public IUnknown
{
public:
    // Retreive the top-most hwnd of the hosted component
    virtual HWND STDMETHODCALLTYPE GetComponentHwnd() = 0;
};
