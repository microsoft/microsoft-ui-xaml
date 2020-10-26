// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using Expander = Microsoft.UI.Xaml.Controls.Expander;
using Windows.UI.Xaml.Tests.MUXControls.ApiTests.RepeaterTests.Common;
using Windows.UI.Xaml.Automation.Peers;

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
    }

    [TopLevelTestPage(Name = "Expander")]
    public sealed partial class ExpanderPage : TestPage
    {
        public ExpanderPage()
        {
            this.InitializeComponent();
            var customControlPeer = FrameworkElementAutomationPeer.FromElement(CustomControl);
            var expanderPeer = FrameworkElementAutomationPeer.FromElement(ExpanderWithCustomEventsSource);

            expanderPeer.EventsSource = customControlPeer;
        }
    }
}
