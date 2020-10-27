// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define MUXCONTROLS_NAMESPACE Microsoft::UI::Xaml::Controls
#define WINRT_MUXC_NAMESPACE winrt::Microsoft::UI::Xaml::Controls
#define WINRT_MUXM_NAMESPACE winrt::Microsoft::UI::Xaml::Media


#define MUXCONTROLSROOT_NAMESPACE_STR L"Microsoft.UI.Xaml"
#define MUXCONTROLSMEDIA_NAMESPACE_STR L"Microsoft.UI.Xaml.Media"

// We have the same framework package name between Debug and Release because our customers (generally) don't want
// to be running debug bits from MUX when their app is debug. VCLibs and .NETNative differ in this regard because
// they do want different behavior, but generally asserts in MUX code aren't relevant to apps. We are leaving in
// this ifdef in case it becomes useful in the future.
#ifdef _DEBUG
// NOTE: This could be "Microsoft.UI.Xaml.Debug" if we wanted to have Debug framework packages be distinct and
// installed side-by-side on a machine.
#define MUXCONTROLS_PACKAGE_NAME L"Microsoft.UI.Xaml.CBS"
#else
#define MUXCONTROLS_PACKAGE_NAME L"Microsoft.UI.Xaml.CBS"
#endif
