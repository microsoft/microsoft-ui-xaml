using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.UI;
using Microsoft.UI.Composition;
using Microsoft.UI.Xaml;
using Controls_02_Hamburger = Microsoft.UI.Xaml.Controls.AnimatedVisuals.AnimatedGlobalNavigationButtonVisualSource;

namespace MUXControlsTestApp
{
    class MockIAnimatedIconSource2 : IAnimatedVisualSource2
    {
        Dictionary<string, double> markers = new Dictionary<string, double>();

        IAnimatedVisualSource2 visual = new Controls_02_Hamburger();
        public MockIAnimatedIconSource2()
        {
            markers.Add("aTob_Start", 0.12345);
            markers.Add("aTob_End", 0.12345);
            markers.Add("bToc_Start", 0.12345);
            markers.Add("cTod_End", 0.12345);
            markers.Add("dToe", 0.12345);
            markers.Add("f", 0.12345);
        }
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor, out object diagnostics)
        {
            return visual.TryCreateAnimatedVisual(Microsoft.UI.Xaml.Media.CompositionTarget.GetCompositorForCurrentThread(), out diagnostics);
        }

        public void SetColorProperty(string propertyName, Color value)
        {
            visual.SetColorProperty(propertyName, value);
        }

        public IReadOnlyDictionary<string, double> Markers => markers;
    }
}
