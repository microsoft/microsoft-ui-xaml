// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define MUXCONTROLS_NAMESPACE Microsoft::UI::Xaml::Controls
#define WINRT_MUXC_NAMESPACE winrt::Microsoft::UI::Xaml::Controls
#define WINRT_MUXM_NAMESPACE winrt::Microsoft::UI::Xaml::Media


#define MUXCONTROLSROOT_NAMESPACE_STR L"Microsoft.UI.Xaml"
#define MUXCONTROLSMEDIA_NAMESPACE_STR L"Microsoft.UI.Xaml.Media"

// Tabular ships its own resource map, so its theme resources are addressed under this root
// rather than MUXC's; sharing MUXC's root caused a PRI277 collision.
#define MUXTABULARROOT_NAMESPACE_STR L"Microsoft.UI.Xaml.Controls.Tabular"
