// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ThemeResourcesFactory.h"
#include "RevealBrush.h"

CppWinRTActivatableClass(ThemeResources);

// Normally global reveal lights are attached automtically via the first call to
// RevealBrush::OnConnected for the current view. However, there are some corner cases
// where lights were lost or never connected. In those situations, apps will manually call this API.
// In general, it is ok to can call this multiple times on elements in the live tree.
// If RevealHoverLight's are already present on the root, we will not try to attach more lights.
//
// Currently known scenarios requiring this API :
// (1) Reveal on Full Window media controls (call when ME/MPE is in FullWindow and pass its MediaTransportControls instance)
// (2) App sets Window.Content (this action destroys the RootScrolLViewer's lights, and they need to be recreated. Pass any element in the main tree.)

void ThemeResources::EnsureRevealLights(winrt::UIElement const& element)
{
    // Ensure that ambient and border lights needed for reveal effects are set on tree root
    if (SharedHelpers::IsXamlCompositionBrushBaseAvailable()
        // If Xaml can apply a light on the root visual, then the app doesn't need to manually attach lights to some other root
        && !SharedHelpers::DoesXamlMoveRSVLightToRootVisual())
    {
        // Defer until next Rendering event. Otherwise, in the FullWindow media case 
        // VisualTreehelper may fail to find the FullWindowMediaRoot that had been created just prior to this call
        auto renderingEventToken = std::make_shared<winrt::event_token>();
        *renderingEventToken = winrt::Xaml::Media::CompositionTarget::Rendering(
            [renderingEventToken, element](auto&, auto&) {
            // Detach event or Rendering will keep calling us back.
            winrt::Xaml::Media::CompositionTarget::Rendering(*renderingEventToken);

            RevealBrush::AttachLightsToAncestor(element, false);
        });
    }
}
