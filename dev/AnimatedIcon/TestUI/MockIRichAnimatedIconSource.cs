using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Controls_09_Hamburger = Microsoft.UI.Xaml.Controls.AnimatedVisuals.Controls_09_Hamburger;

namespace MUXControlsTestApp
{
    class MockIRichAnimatedIconSource : IRichAnimatedVisualSource
    {
        Dictionary<string, double> markers = new Dictionary<string, double>();

        IRichAnimatedVisualSource visual = new Controls_09_Hamburger();
        public MockIRichAnimatedIconSource()
        {
            markers.Add("aTob_Start", 0.12345);
            markers.Add("aTob_End", 0.12345);
            markers.Add("bToc_Start", 0.12345);
            markers.Add("cTod_End", 0.12345);
            markers.Add("dToe", 0.12345);
            markers.Add("f", 0.12345);
        }
        public IAnimatedVisual TryCreateAnimatedIconVisual(Compositor compositor)
        {
            return visual.TryCreateAnimatedIconVisual(Window.Current.Compositor);
        }

        public void SetColorProperty(string propertyName, Color value)
        {
            visual.SetColorProperty(propertyName, value);
        }

        public IReadOnlyDictionary<string, double> Markers => markers;
    }
}
