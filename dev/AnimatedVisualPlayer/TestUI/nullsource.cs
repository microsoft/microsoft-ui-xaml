// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Composition;

namespace AnimatedVisuals
{
    sealed class nullsource : IAnimatedVisualSource
    {
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            return null;
        }

        sealed class AnimatedVisual : IAnimatedVisual
        {
            const long c_durationTicks = 59670000;
            ContainerVisual _root;

            internal AnimatedVisual(Compositor compositor)
            {
                _root = null;
            }

            Visual IAnimatedVisual.RootVisual => _root;
            TimeSpan IAnimatedVisual.Duration => TimeSpan.FromTicks(c_durationTicks);
            Vector2 IAnimatedVisual.Size => new Vector2(375, 667);
            void IDisposable.Dispose() => _root?.Dispose();
        }
    }
}
