// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI;
using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Hosting;

#if !BUILD_WINDOWS
using RevealTestApi = Microsoft.UI.Private.Media.RevealTestApi;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class RevealScenarioLights : TestPage
    {
        public RevealScenarioLights()
        {
            this.InitializeComponent();
            _revealTest = new RevealTestApi { TargetTheme = CurrentTheme };

            _backgroundLight = Window.Current.Compositor.CreateSpotLight();
            _borderLight = Window.Current.Compositor.CreateSpotLight();
            _borderWideLight = Window.Current.Compositor.CreateSpotLight();

            _borderLight.Offset = _borderWideLight.Offset =
            _backgroundLight.Offset = new Vector3((float)SwatchInnerGrid.Width / 2, (float)SwatchInnerGrid.Height / 2, 100);

            ExprBind(_backgroundLight, _revealTest.BackgroundLight, "InnerConeAngle");
            ExprBind(_backgroundLight, _revealTest.BackgroundLight, "OuterConeAngle");
            ExprBind(_backgroundLight, _revealTest.BackgroundLight, "ConstantAttenuation");
            ExprBind(_backgroundLight, _revealTest.BackgroundLight, "InnerConeColor");
            ExprBind(_backgroundLight, _revealTest.BackgroundLight, "OuterConeColor");

            SpotLight borderSpotlight = _revealTest.GetSpotLight(_revealTest.BorderLight as XamlLight);
            ExprBind(_borderLight, borderSpotlight, "InnerConeAngle");
            ExprBind(_borderLight, borderSpotlight, "OuterConeAngle");
            ExprBind(_borderLight, borderSpotlight, "ConstantAttenuation");
            ExprBind(_borderLight, borderSpotlight, "InnerConeColor");
            ExprBind(_borderLight, borderSpotlight, "OuterConeColor");

            SpotLight borderWideSpotlight = _revealTest.GetSpotLight(_revealTest.BorderWideLight as XamlLight);
            ExprBind(_borderWideLight, borderWideSpotlight, "InnerConeAngle");
            ExprBind(_borderWideLight, borderWideSpotlight, "OuterConeAngle");
            ExprBind(_borderWideLight, borderWideSpotlight, "ConstantAttenuation");
            ExprBind(_borderWideLight, borderWideSpotlight, "InnerConeColor");
            ExprBind(_borderWideLight, borderWideSpotlight, "OuterConeColor");
        }

        void ExprBind(CompositionObject target, CompositionObject source, string targetProperty, string sourceProperty = null)
        {
            if (sourceProperty == null) sourceProperty = targetProperty;
            var expr = target.Compositor.CreateExpressionAnimation(String.Format("source.{0}", sourceProperty));
            expr.SetReferenceParameter("source", source);
            target.StartAnimation(targetProperty, expr);
        }

        private RevealTestApi _revealTest;
        private SpotLight _backgroundLight;
        private SpotLight _borderLight;
        private SpotLight _borderWideLight;

        public RevealTestApi RevealTest
        {
            get { return _revealTest; }
        }

        protected override void OnCurrentThemeChanged()
        {
            // Refresh the UI so the bindings re-resolve against current theme
            Frame.NavigateWithoutAnimation(GetType(), null);
            Frame.GoBack();
        }

        // Animate so that we are consistent with MUX in hitting Bug 10979961:SpotLight.Inner/OuterConeAngleInDegrees return wrong value after animation completes.
        // When that's fixed we don't need to animate and don't need to multiply the cone angle below.
        void AnimateTo(CompositionObject obj, string prop, float value)
        {
            var anim = obj.Compositor.CreateScalarKeyFrameAnimation();
            anim.Duration = TimeSpan.FromMilliseconds(1);
            anim.InsertKeyFrame(1.0f, value);
            obj.StartAnimation(prop, anim);
        }

        double SpotlightInnerConeValue
        {
            get { return _revealTest.BackgroundLight.InnerConeAngleInDegrees; }
            set { _revealTest.BackgroundLight.InnerConeAngleInDegrees = (float)value; }
        }

        double SpotlightOuterConeValue
        {
            get { return _revealTest.BackgroundLight.OuterConeAngleInDegrees; }
            set { _revealTest.BackgroundLight.OuterConeAngleInDegrees = (float)value; }
        }

        double SpotlightInnerConeAlphaValue
        {
            get { return _revealTest.BackgroundLight.InnerConeColor.A; }
            set { _revealTest.BackgroundLight.InnerConeColor = Color.FromArgb((byte)value, (byte)value, (byte)value, (byte)value); }
        }

        double SpotlightOuterConeAlphaValue
        {
            get { return _revealTest.BackgroundLight.OuterConeColor.A; }
            set { _revealTest.BackgroundLight.OuterConeColor = Color.FromArgb((byte)value, (byte)value, (byte)value, (byte)value); }
        }

        double SpotlightAttenuationValue
        {
            get { return _revealTest.BackgroundLight.ConstantAttenuation; }
            set { _revealTest.BackgroundLight.ConstantAttenuation = (float)value; }
        }

        double SpotlightMinSizeValue
        {
            get { return _revealTest.BackgroundLightMinSize; }
            set { _revealTest.BackgroundLightMinSize = value; }
        }

        double SpotlightMaxSizeValue
        {
            get { return _revealTest.BackgroundLightMaxSize; }
            set { _revealTest.BackgroundLightMaxSize = value; }
        }

        double BorderSpotlightInnerConeValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderLight).InnerConeAngleInDegrees / 180 * Math.PI; }
            set { AnimateTo(_revealTest.GetSpotLight(_revealTest.BorderLight), "InnerConeAngleInDegrees", (float)value); }
        }

        double BorderSpotlightOuterConeValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderLight).OuterConeAngleInDegrees / 180 * Math.PI; }
            set { AnimateTo(_revealTest.GetSpotLight(_revealTest.BorderLight), "OuterConeAngleInDegrees", (float)value); }
        }

        double BorderSpotlightAttenuationValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderLight).ConstantAttenuation; }
            set { _revealTest.GetSpotLight(_revealTest.BorderLight).ConstantAttenuation = (float)value; }
        }

        double WideBorderSpotlightInnerConeValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderWideLight).InnerConeAngleInDegrees / 180 * Math.PI; }
            set { AnimateTo(_revealTest.GetSpotLight(_revealTest.BorderWideLight), "InnerConeAngleInDegrees", (float)value); }
        }

        double WideBorderSpotlightOuterConeValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderWideLight).OuterConeAngleInDegrees / 180 * Math.PI; }
            set { AnimateTo(_revealTest.GetSpotLight(_revealTest.BorderWideLight), "OuterConeAngleInDegrees", (float)value); }
        }

        double WideBorderSpotlightAttenuationValue
        {
            get { return _revealTest.GetSpotLight(_revealTest.BorderWideLight).ConstantAttenuation; }
            set { _revealTest.GetSpotLight(_revealTest.BorderWideLight).ConstantAttenuation = (float)value; }
        }
    }
}
