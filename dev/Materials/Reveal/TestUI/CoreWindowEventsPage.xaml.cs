// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Navigation;
using Windows.UI.Xaml.Markup;
using Windows.UI.Xaml.Media;
using Windows.UI.Core;
using System;

using MaterialHelperTestApi = Microsoft.UI.Private.Media.MaterialHelperTestApi;

namespace MUXControlsTestApp
{
    public sealed partial class CoreWindowEventsPage : TestPage
    {
        ListViewItem _listViewIem1 = null;
        UInt32 _revealAddedCount = 0;

        public CoreWindowEventsPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsTypePresent("Windows.UI.Xaml.Media.XamlCompositionBrushBase"))
            {
                AutomationProperties.SetName(this, "CoreWindowEventsPage");
                AutomationProperties.SetAutomationId(this, "CoreWindowEventsPage");

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

        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            base.OnNavigatedTo(e);

            // Make sure Page registers for PointerExited here, before the RevealBorderLight (via AddReveal_Click)
            CoreWindow coreWindow = CoreWindow.GetForCurrentThread();
            coreWindow.PointerExited += CoreWindow_PointerExited;
        }

        private void CoreWindow_PointerExited(CoreWindow sender, PointerEventArgs args)
        {
            if (_listViewIem1 != null)
            {
                RevealContainer.Children.Remove(_listViewIem1);
                _listViewIem1 = null;
            }
        }

        private void AddReveal_Click(object sender, RoutedEventArgs e)
        {
            _revealAddedCount++;

            if (_listViewIem1 == null)
            {
                _listViewIem1 = new ListViewItem();
                _listViewIem1.Height = 100;
                _listViewIem1.Width = 300;
                _listViewIem1.Content = "Test ListViewItem";
                RevealContainer.Children.Add(_listViewIem1);
            }

            if (_revealAddedCount == 2)
            {
                // Log success once we got this far.
                using (var logger = new ResultsLogger("CoreWindowEventsTests", TestResult)) { }
            }
        }
    }
}