// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Numerics;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Test-only IAnimatedVisual2 whose root is an empty ContainerVisual. It records how the
    // AnimatedVisualPlayer manages its animations and lifetime so tests can observe the player's
    // behavior without depending on generated Lottie content.
    internal sealed partial class AvpTestAnimatedVisual : IAnimatedVisual2
    {
        public AvpTestAnimatedVisual(Compositor compositor, Vector2 size, TimeSpan duration, bool hasRootVisual)
        {
            RootVisual = hasRootVisual ? compositor.CreateContainerVisual() : null;
            Size = size;
            Duration = duration;
        }

        public Visual RootVisual { get; }
        public Vector2 Size { get; }
        public TimeSpan Duration { get; }

        public int CreateAnimationsCount { get; private set; }
        public int DestroyAnimationsCount { get; private set; }
        public bool IsDisposed { get; private set; }

        public void CreateAnimations() => CreateAnimationsCount++;
        public void DestroyAnimations() => DestroyAnimationsCount++;
        public void Dispose() => IsDisposed = true;
    }

    // IAnimatedVisualSource (legacy, two-argument TryCreateAnimatedVisual) used to drive AnimatedVisualPlayer.
    internal sealed partial class AvpTestSource : IAnimatedVisualSource
    {
        public Vector2 Size { get; set; } = new Vector2(200, 100);
        public TimeSpan Duration { get; set; } = TimeSpan.FromMilliseconds(300);
        public bool FailToCreate { get; set; }
        public bool HasRootVisual { get; set; } = true;
        public object DiagnosticsToReturn { get; set; }

        public int CreateCount { get; private set; }
        public AvpTestAnimatedVisual LastVisual { get; private set; }

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            CreateCount++;
            diagnostics = DiagnosticsToReturn;
            if (FailToCreate)
            {
                LastVisual = null;
                return null;
            }

            LastVisual = new AvpTestAnimatedVisual(compositor, Size, Duration, HasRootVisual);
            return LastVisual;
        }
    }

    // Source implementing both IAnimatedVisualSource and IAnimatedVisualSource3 so tests can observe
    // which overload the player chooses and the createAnimations argument it passes.
    internal sealed partial class AvpTestSource3 : IAnimatedVisualSource, IAnimatedVisualSource3
    {
        public Vector2 Size { get; set; } = new Vector2(200, 100);
        public TimeSpan Duration { get; set; } = TimeSpan.FromMilliseconds(300);

        public int LegacyCreateCount { get; private set; }
        public int Source3CreateCount { get; private set; }
        public bool? LastCreateAnimationsArgument { get; private set; }
        public AvpTestAnimatedVisual LastVisual { get; private set; }

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            LegacyCreateCount++;
            diagnostics = null;
            LastVisual = new AvpTestAnimatedVisual(compositor, Size, Duration, hasRootVisual: true);
            return LastVisual;
        }

        public IAnimatedVisual2 TryCreateAnimatedVisual(Compositor compositor, out object diagnostics, bool createAnimations)
        {
            Source3CreateCount++;
            LastCreateAnimationsArgument = createAnimations;
            diagnostics = null;
            LastVisual = new AvpTestAnimatedVisual(compositor, Size, Duration, hasRootVisual: true);
            return LastVisual;
        }
    }

    // IDynamicAnimatedVisualSource whose content can be changed and then announced via AnimatedVisualInvalidated.
    internal sealed partial class AvpTestDynamicSource : IDynamicAnimatedVisualSource
    {
        private TypedEventHandler<IDynamicAnimatedVisualSource, object> _invalidated;

        public Vector2 Size { get; set; } = new Vector2(200, 100);
        public TimeSpan Duration { get; set; } = TimeSpan.FromMilliseconds(300);

        public int CreateCount { get; private set; }
        public int SubscriberCount { get; private set; }
        public AvpTestAnimatedVisual LastVisual { get; private set; }

        public event TypedEventHandler<IDynamicAnimatedVisualSource, object> AnimatedVisualInvalidated
        {
            add
            {
                SubscriberCount++;
                _invalidated += value;
            }
            remove
            {
                SubscriberCount--;
                _invalidated -= value;
            }
        }

        public void RaiseInvalidated()
        {
            _invalidated?.Invoke(this, null);
        }

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            CreateCount++;
            diagnostics = null;
            LastVisual = new AvpTestAnimatedVisual(compositor, Size, Duration, hasRootVisual: true);
            return LastVisual;
        }
    }
}
