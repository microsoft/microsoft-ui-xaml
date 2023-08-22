// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using Windows.UI.Composition;

namespace AnimatedVisuals
{
    // An IAnimatedVisualSource that always fails to create an animated visual.
    // Used for testing of the fallback path in AnimatedVisualPlayer.
    sealed class nullsource : IAnimatedVisualSource
    {
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            // Return null to indicate failure.
            return null;
        }
    }
}
