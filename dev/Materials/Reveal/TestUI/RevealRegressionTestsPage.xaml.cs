// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;
using Common;

using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;

namespace MUXControlsTestApp
{
    public sealed partial class RevealRegressionTestsPage : TestPage
    {
        Button _test1_RevealButton;

        public RevealRegressionTestsPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.XamlCompositionBrushBase"))
            {
                AutomationProperties.SetName(this, "RevealRegressionTestsPage");
                AutomationProperties.SetAutomationId(this, "RevealRegressionTestsPage");

                MaterialHelperTestApi.IgnoreAreEffectsFast = true;
                MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            }
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            base.OnNavigatedFrom(e);
        }

        private void AddRevealButton_Click(object sender, RoutedEventArgs e)
        {
            _test1_RevealButton = (Button)XamlReader.Load(@"
                        <Button xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                                xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                                x:Name='Test1_RevealButton' Style='{StaticResource ButtonRevealStyle}'
                                Content='Test1 Reveal Button' AutomationProperties.AutomationId='Test1_RevealButton' AutomationProperties.Name='Test1_RevealButton'/>");
            Test1_RevealButtonContainer.Children.Add(_test1_RevealButton);
        }

        private void _test1_RevealButton_Click(object sender, RoutedEventArgs e)
        {
            throw new System.NotImplementedException();
        }

        private void RemoveRevealButton_Click(object sender, RoutedEventArgs e)
        {
            Test1_RevealButtonContainer.Children.Remove(_test1_RevealButton);
        }

        private void AddBackRevealButton_Click(object sender, RoutedEventArgs e)
        {
            Test1_RevealButtonContainer.Children.Add(_test1_RevealButton);
            
            // Add handler for validation
            _test1_RevealButton.Click += RevealButton_Click;
        }

        private UIElement GetElementForHoverLight(object element)
        {
            if (Common.PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone2))
            {
                var target = element as Control;
                return VisualTreeHelper.GetChild(target, 0) as UIElement;
            }
            return element as UIElement;
        }

        private void RevealButton_Click(object sender, RoutedEventArgs e)
        {
            using (var logger = new ResultsLogger("RevealCreateThenEnableEffects", TestResult))
            {
                UIElement lightsElement = GetElementForHoverLight(sender);
                var lights = lightsElement.Lights;

                bool lightCountResult = lights.Count == 2;          // Reveal Press light is also present
                logger.Verify(lightCountResult, "lightCountResult:" + lightCountResult);
            }
        }
 
        private void SimluateDisabledByPolicyToggleButton_Checked(object sender, RoutedEventArgs e)
        {
            MaterialHelperTestApi.SimulateDisabledByPolicy = true;
        }

        private void SimluateDisabledByPolicyToggleButton_Unchecked(object sender, RoutedEventArgs e)
        {
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
        }
    }
}