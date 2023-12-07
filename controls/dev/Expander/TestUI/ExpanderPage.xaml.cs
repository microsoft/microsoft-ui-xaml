// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using Expander = Microsoft.UI.Xaml.Controls.Expander;
using Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Microsoft.UI.Xaml.Automation.Peers;

namespace MUXControlsTestApp
{
    // We are going to test setting the events source of an expander to the customcontrol's
    public sealed class TestControl : ContentControl
    {
        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new TestControlAutomationPeer(this);
        }

    }

    public sealed class TestControlAutomationPeer : FrameworkElementAutomationPeer
    {
        public TestControlAutomationPeer(TestControl owner) : base(owner){}
        protected override AutomationControlType GetAutomationControlTypeCore()
        {
            return AutomationControlType.Custom;
        }

        protected override string GetLocalizedControlTypeCore()
        {
            return "TestControl";
        }
    }

    [TopLevelTestPage(Name = "Expander")]
    [AxeScanTestPage(Name = "Expander-Axe")]
    public sealed partial class ExpanderPage : TestPage
    {
        public ExpanderPage()
        {
            this.InitializeComponent();
            var customControlPeer = FrameworkElementAutomationPeer.FromElement(CustomControl);
            var expanderPeer = FrameworkElementAutomationPeer.FromElement(ExpanderWithCustomEventsSource);

            // Commenting because of MuxTestInfra bug: 
            // https://github.com/microsoft/microsoft-ui-xaml/issues/3491
            //expanderPeer.EventsSource = customControlPeer;
        }
    }
}
