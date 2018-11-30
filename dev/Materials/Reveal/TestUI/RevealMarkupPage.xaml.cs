// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Composition;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

#if !BUILD_WINDOWS
using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class RevealMarkupPage : TestPage
    {
        public RevealMarkupPage()
        {
            this.InitializeComponent();

            MaterialHelperTestApi.IgnoreAreEffectsFast = true;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
            base.OnNavigatedFrom(e);
        }

        private void RunTestButton_Clicked(object sender, RoutedEventArgs e)
        {
            using (var logger = new ResultsLogger("RevealBrushFromMarkup", TestResult))
            {
                int globalLightCount = 0;
                const int globalLightCountExpected = 5;
                UIElement visualRoot = GetTopParent(Window.Current.Content);

                if (visualRoot != null && visualRoot.GetType() == typeof(ScrollViewer))
                {
                    globalLightCount = visualRoot.Lights.Count;
                }
                else
                {
                    logger.Verify(false, "visualRoot = null or type is not ScrollViewer");
                }

                logger.Verify(globalLightCount == globalLightCountExpected, "globalLightCount: " + globalLightCount + ", expected: " + globalLightCountExpected);
            }
        }

        private UIElement GetTopParent(UIElement current)
        {
            UIElement parent = current;
            do
            {
                current = parent;
                parent = (UIElement)VisualTreeHelper.GetParent(current);
            } while (parent != null);

            return current;
        }
    }
}
