using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Text;
using Windows.UI;
using Windows.UI.Composition;

namespace MUXControlsTestApp
{
    class MockAnimatedIconSource : IRichAnimatedVisualSource
    {
        Dictionary<string, double> markers = new Dictionary<string, double>();

        IRichAnimatedVisualSource visual = new Controls_09_Hamburger();
        public MockAnimatedIconSource()
        {
            markers.Add("aTob_Start", 0.0);
            markers.Add("aTob_End", 0.316666663);
            markers.Add("NormalToPressedStart", 0.0);
            markers.Add("NormalToPressedEnd", 0.816666663);
            markers.Add("PointerOverToNormalStart", 0.316666663);
            markers.Add("PointerOverToNormalEnd", 0.0);
            markers.Add("PointerOverToPressedStart", 0.333333343);
            markers.Add("PointerOverToPressedEnd", 0.816666663);
            markers.Add("PressedToNormalStart", 0.833333313);
            markers.Add("PressedToNormalEnd", 0.983333349);
            markers.Add("PressedToPointerOverStart", 0.816666663);
            markers.Add("PressedToPointerOverEnd", 0.333333343);
            markers.Add("Disabled", 0.5);
        }
        public IAnimatedVisual TryCreateAnimatedVisual(Compositor compositor)
        {
            return visual.TryCreateAnimatedVisual(Window.Current.Compositor);
        }

        public void SetColorProperty(string propertyName, Color value)
        {
            visual.SetColorProperty(propertyName, value);
        }

        public IReadOnlyDictionary<string, double> Markers => markers;
    }
}
