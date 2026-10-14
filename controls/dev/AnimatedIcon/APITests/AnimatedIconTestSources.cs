// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Numerics;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.AnimatedVisuals;
using Windows.UI;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    // Supplies a real generated visual but lets a test choose the markers and observe SetColorProperty calls.
    sealed partial class ControlledAnimatedVisualSource : IAnimatedVisualSource2
    {
        private readonly IAnimatedVisualSource2 _visualSource = new AnimatedBackVisualSource();
        private readonly IReadOnlyDictionary<string, double> _markers;

        public ControlledAnimatedVisualSource(IReadOnlyDictionary<string, double> markers)
        {
            _markers = markers;
        }

        public List<KeyValuePair<string, Color>> ColorPropertyCalls { get; } = new List<KeyValuePair<string, Color>>();

        public IReadOnlyDictionary<string, double> Markers => _markers;

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            return _visualSource.TryCreateAnimatedVisual(compositor, out diagnostics);
        }

        public void SetColorProperty(string propertyName, Color value)
        {
            ColorPropertyCalls.Add(new KeyValuePair<string, Color>(propertyName, value));
            _visualSource.SetColorProperty(propertyName, value);
        }
    }

    // A source that fails to create a visual, which must make AnimatedIcon show its FallbackIconSource.
    sealed partial class NullVisualAnimatedVisualSource : IAnimatedVisualSource2
    {
        public IReadOnlyDictionary<string, double> Markers { get; } = new Dictionary<string, double>();

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            return null;
        }

        public void SetColorProperty(string propertyName, Color value)
        {
        }
    }

    // A source whose visual reports a zero natural size.
    sealed partial class ZeroSizeAnimatedVisualSource : IAnimatedVisualSource2
    {
        public IReadOnlyDictionary<string, double> Markers { get; } = new Dictionary<string, double>();

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            return new ContainerAnimatedVisual(compositor, Vector2.Zero, TimeSpan.FromSeconds(1));
        }

        public void SetColorProperty(string propertyName, Color value)
        {
        }
    }

    // A source with chosen markers whose 48x48 visual has a chosen duration, so a test controls exact segment durations.
    sealed partial class TimedAnimatedVisualSource : IAnimatedVisualSource2
    {
        private readonly TimeSpan _duration;

        public TimedAnimatedVisualSource(IReadOnlyDictionary<string, double> markers, TimeSpan duration)
        {
            Markers = markers;
            _duration = duration;
        }

        public IReadOnlyDictionary<string, double> Markers { get; }

        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            diagnostics = null;
            return new ContainerAnimatedVisual(compositor, new Vector2(48, 48), _duration);
        }

        public void SetColorProperty(string propertyName, Color value)
        {
        }
    }

    // An empty container visual with a chosen natural size and duration.
    sealed partial class ContainerAnimatedVisual : IAnimatedVisual
    {
        private readonly ContainerVisual _root;
        private readonly Vector2 _size;
        private readonly TimeSpan _duration;

        public ContainerAnimatedVisual(Compositor compositor, Vector2 size, TimeSpan duration)
        {
            _root = compositor.CreateContainerVisual();
            _size = size;
            _duration = duration;
        }

        Visual IAnimatedVisual.RootVisual => _root;
        TimeSpan IAnimatedVisual.Duration => _duration;
        Vector2 IAnimatedVisual.Size => _size;
        void IDisposable.Dispose() => _root.Dispose();
    }

    // Expected IAnimatedVisualSource2.Markers of the generated AnimatedVisuals sources.
    static class ExpectedMarkers
    {
        public static readonly IReadOnlyDictionary<string, double> Accept = new Dictionary<string, double>
        {
            ["NormalOnToNormalOff_Start"] = 0.0,
            ["NormalOnToNormalOff_End"] = 0.0253125,
            ["NormalOnToPointerOverOn_Start"] = 0.0315625,
            ["NormalOnToPointerOverOn_End"] = 0.0565625,
            ["NormalOnToPressedOn_Start"] = 0.0628125,
            ["NormalOnToPressedOn_End"] = 0.0878125,
            ["NormalOffToNormalOn_Start"] = 0.0940625,
            ["NormalOffToNormalOn_End"] = 0.2128125,
            ["NormalOffToPointerOverOff_Start"] = 0.2190625,
            ["NormalOffToPointerOverOff_End"] = 0.2440625,
            ["NormalOffToPressedOff_Start"] = 0.2503125,
            ["NormalOffToPressedOff_End"] = 0.2753125,
            ["PointerOverOnToPointerOverOff_Start"] = 0.2815625,
            ["PointerOverOnToPointerOverOff_End"] = 0.3065625,
            ["PointerOverOnToNormalOn_Start"] = 0.3128125,
            ["PointerOverOnToNormalOn_End"] = 0.3378125,
            ["PointerOverOnToPressedOn_Start"] = 0.3440625,
            ["PointerOverOnToPressedOn_End"] = 0.3690625,
            ["PointerOverOffToPointerOverOn_Start"] = 0.3753125,
            ["PointerOverOffToPointerOverOn_End"] = 0.4940625,
            ["PointerOverOffToNormalOff_Start"] = 0.5003125,
            ["PointerOverOffToNormalOff_End"] = 0.5253125,
            ["PointerOverOffToPressedOff_Start"] = 0.5315625,
            ["PointerOverOffToPressedOff_End"] = 0.5565625,
            ["PressedOnToPressedOff_Start"] = 0.5628125,
            ["PressedOnToPressedOff_End"] = 0.5878125,
            ["PressedOnToPointerOverOff_Start"] = 0.5940625,
            ["PressedOnToPointerOverOff_End"] = 0.6190625,
            ["PressedOnToNormalOff_Start"] = 0.6253125,
            ["PressedOnToNormalOff_End"] = 0.6503125,
            ["PressedOffToPressedOn_Start"] = 0.6565625,
            ["PressedOffToPressedOn_End"] = 0.7128125,
            ["PressedOffToPointerOverOn_Start"] = 0.7190625,
            ["PressedOffToPointerOverOn_End"] = 0.8378125,
            ["PressedOffToNormalOn_Start"] = 0.8440625,
            ["PressedOffToNormalOn_End"] = 0.9628125,
            ["NormalIndeterminate"] = 0.9690625,
            ["PointerOverIndeterminate"] = 0.9815625,
            ["PressedIndeterminate"] = 0.9940625,
        };

        public static readonly IReadOnlyDictionary<string, double> Back = new Dictionary<string, double>
        {
            ["NormalToPointerOver_Start"] = 0.0,
            ["NormalToPointerOver_End"] = 0.113125,
            ["NormalToPressed_Start"] = 0.125625,
            ["NormalToPressed_End"] = 0.238125,
            ["PointerOverToNormal_Start"] = 0.250625,
            ["PointerOverToNormal_End"] = 0.363125,
            ["PointerOverToPressed_Start"] = 0.375625,
            ["PointerOverToPressed_End"] = 0.488125,
            ["PressedToNormal_Start"] = 0.500625,
            ["PressedToNormal_End"] = 0.738125,
            ["PressedToPointerOver_Start"] = 0.750625,
            ["PressedToPointerOver_End"] = 0.988125,
        };

        public static readonly IReadOnlyDictionary<string, double> ChevronDownSmall = new Dictionary<string, double>
        {
            ["NormalToPointerOver_Start"] = 0.0,
            ["NormalToPointerOver_End"] = 0.0,
            ["NormalToPressed_Start"] = 0.0,
            ["NormalToPressed_End"] = 0.312068965517241,
            ["PointerOverToNormal_Start"] = 0.0,
            ["PointerOverToNormal_End"] = 0.0,
            ["PointerOverToPressed_Start"] = 0.0,
            ["PointerOverToPressed_End"] = 0.312068965517241,
            ["PressedToNormal_Start"] = 0.346551724137931,
            ["PressedToNormal_End"] = 0.967241379310345,
            ["PressedToPointerOver_Start"] = 0.346551724137931,
            ["PressedToPointerOver_End"] = 0.967241379310345,
        };

        public static readonly IReadOnlyDictionary<string, double> ChevronRightDownSmall = new Dictionary<string, double>
        {
            ["NormalOnToNormalOff_Start"] = 0.0,
            ["NormalOnToNormalOff_End"] = 0.0411363636363636,
            ["NormalOnToPointerOverOn_Start"] = 0.0456818181818182,
            ["NormalOnToPointerOverOn_End"] = 0.0865909090909091,
            ["NormalOnToPressedOn_Start"] = 0.0911363636363636,
            ["NormalOnToPressedOn_End"] = 0.132045454545455,
            ["NormalOffToNormalOn_Start"] = 0.136590909090909,
            ["NormalOffToNormalOn_End"] = 0.268409090909091,
            ["NormalOffToPointerOverOff_Start"] = 0.272954545454545,
            ["NormalOffToPointerOverOff_End"] = 0.313863636363636,
            ["NormalOffToPressedOff_Start"] = 0.318409090909091,
            ["NormalOffToPressedOff_End"] = 0.359318181818182,
            ["PointerOverOnToPointerOverOff_Start"] = 0.363863636363636,
            ["PointerOverOnToPointerOverOff_End"] = 0.404772727272727,
            ["PointerOverOnToNormalOn_Start"] = 0.409318181818182,
            ["PointerOverOnToNormalOn_End"] = 0.450227272727273,
            ["PointerOverOnToPressedOn_Start"] = 0.454772727272727,
            ["PointerOverOnToPressedOn_End"] = 0.495681818181818,
            ["PointerOverOffToPointerOverOn_Start"] = 0.500227272727273,
            ["PointerOverOffToPointerOverOn_End"] = 0.541136363636364,
            ["PointerOverOffToNormalOff_Start"] = 0.545681818181818,
            ["PointerOverOffToNormalOff_End"] = 0.586590909090909,
            ["PointerOverOffToPressedOff_Start"] = 0.591136363636364,
            ["PointerOverOffToPressedOff_End"] = 0.632045454545455,
            ["PressedOnToPressedOff_Start"] = 0.636590909090909,
            ["PressedOnToPressedOff_End"] = 0.6775,
            ["PressedOnToPointerOverOff_Start"] = 0.682045454545455,
            ["PressedOnToPointerOverOff_End"] = 0.722954545454546,
            ["PressedOnToNormalOff_Start"] = 0.7275,
            ["PressedOnToNormalOff_End"] = 0.768409090909091,
            ["PressedOffToPressedOn_Start"] = 0.772954545454546,
            ["PressedOffToPressedOn_End"] = 0.813863636363636,
            ["PressedOffToPointerOverOn_Start"] = 0.818409090909091,
            ["PressedOffToPointerOverOn_End"] = 0.904772727272727,
            ["PressedOffToNormalOn_Start"] = 0.909318181818182,
            ["PressedOffToNormalOn_End"] = 0.995681818181818,
        };

        public static readonly IReadOnlyDictionary<string, double> ChevronUpDownSmall = new Dictionary<string, double>
        {
            ["NormalOnToNormalOff_Start"] = 0.0,
            ["NormalOnToNormalOff_End"] = 0.111730769230769,
            ["NormalOnToPointerOverOn_Start"] = 0.115576923076923,
            ["NormalOnToPointerOverOn_End"] = 0.150192307692308,
            ["NormalOnToPressedOn_Start"] = 0.154038461538462,
            ["NormalOnToPressedOn_End"] = 0.188653846153846,
            ["NormalOffToNormalOn_Start"] = 0.1925,
            ["NormalOffToNormalOn_End"] = 0.304038461538462,
            ["NormalOffToPointerOverOff_Start"] = 0.307884615384615,
            ["NormalOffToPointerOverOff_End"] = 0.3425,
            ["NormalOffToPressedOff_Start"] = 0.346346153846154,
            ["NormalOffToPressedOff_End"] = 0.380961538461538,
            ["PointerOverOnToPointerOverOff_Start"] = 0.384807692307692,
            ["PointerOverOnToPointerOverOff_End"] = 0.419423076923077,
            ["PointerOverOnToNormalOn_Start"] = 0.423269230769231,
            ["PointerOverOnToNormalOn_End"] = 0.457884615384615,
            ["PointerOverOnToPressedOn_Start"] = 0.461730769230769,
            ["PointerOverOnToPressedOn_End"] = 0.496346153846154,
            ["PointerOverOffToPointerOverOn_Start"] = 0.500192307692308,
            ["PointerOverOffToPointerOverOn_End"] = 0.534807692307692,
            ["PointerOverOffToNormalOff_Start"] = 0.538653846153846,
            ["PointerOverOffToNormalOff_End"] = 0.573269230769231,
            ["PointerOverOffToPressedOff_Start"] = 0.577115384615385,
            ["PointerOverOffToPressedOff_End"] = 0.611730769230769,
            ["PressedOnToPressedOff_Start"] = 0.615576923076923,
            ["PressedOnToPressedOff_End"] = 0.650192307692308,
            ["PressedOnToPointerOverOff_Start"] = 0.654038461538462,
            ["PressedOnToPointerOverOff_End"] = 0.727115384615385,
            ["PressedOnToNormalOff_Start"] = 0.730961538461539,
            ["PressedOnToNormalOff_End"] = 0.804038461538462,
            ["PressedOffToPressedOn_Start"] = 0.807884615384615,
            ["PressedOffToPressedOn_End"] = 0.8425,
            ["PressedOffToPointerOverOn_Start"] = 0.846346153846154,
            ["PressedOffToPointerOverOn_End"] = 0.919423076923077,
            ["PressedOffToNormalOn_Start"] = 0.923269230769231,
            ["PressedOffToNormalOn_End"] = 0.996346153846154,
        };

        public static readonly IReadOnlyDictionary<string, double> Find = new Dictionary<string, double>
        {
            ["NormalToPointerOver_Start"] = 0.0,
            ["NormalToPointerOver_End"] = 0.113125,
            ["NormalToPressed_Start"] = 0.125625,
            ["NormalToPressed_End"] = 0.238125,
            ["PointerOverToNormal_Start"] = 0.250625,
            ["PointerOverToNormal_End"] = 0.363125,
            ["PointerOverToPressed_Start"] = 0.375625,
            ["PointerOverToPressed_End"] = 0.488125,
            ["PressedToNormal_Start"] = 0.500625,
            ["PressedToNormal_End"] = 0.738125,
            ["PressedToPointerOver_Start"] = 0.750625,
            ["PressedToPointerOver_End"] = 0.988125,
        };

        public static readonly IReadOnlyDictionary<string, double> GlobalNavigationButton = new Dictionary<string, double>
        {
            ["NormalToPointerOver_Start"] = 0.0,
            ["NormalToPointerOver_End"] = 0.113125,
            ["NormalToPressed_Start"] = 0.125625,
            ["NormalToPressed_End"] = 0.238125,
            ["PointerOverToNormal_Start"] = 0.250625,
            ["PointerOverToNormal_End"] = 0.363125,
            ["PointerOverToPressed_Start"] = 0.375625,
            ["PointerOverToPressed_End"] = 0.488125,
            ["PressedToNormal_Start"] = 0.500625,
            ["PressedToNormal_End"] = 0.738125,
            ["PressedToPointerOver_Start"] = 0.750625,
            ["PressedToPointerOver_End"] = 0.988125,
        };

        public static readonly IReadOnlyDictionary<string, double> Settings = new Dictionary<string, double>
        {
            ["NormalToPointerOver_Start"] = 0.0,
            ["NormalToPointerOver_End"] = 0.0754166666666667,
            ["NormalToPressed_Start"] = 0.08375,
            ["NormalToPressed_End"] = 0.15875,
            ["PointerOverToNormal_Start"] = 0.167083333333333,
            ["PointerOverToNormal_End"] = 0.242083333333333,
            ["PointerOverToPressed_Start"] = 0.250416666666667,
            ["PointerOverToPressed_End"] = 0.325416666666667,
            ["PressedToNormal_Start"] = 0.33375,
            ["PressedToNormal_End"] = 0.65875,
            ["PressedToPointerOver_Start"] = 0.667083333333333,
            ["PressedToPointerOver_End"] = 0.992083333333333,
        };
    }
}
