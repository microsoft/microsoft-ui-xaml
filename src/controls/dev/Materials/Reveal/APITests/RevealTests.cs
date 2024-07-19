﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RevealTests : ApiTestBase
    {
        [TestMethod] //TODO: Re-enable once issue #644 is fixed.
        [TestProperty("Ignore", "True")]
        public void ValidateAppBarButtonRevealStyles()
        {
            AppBarButton buttonLabelsOnRight = null;
            AppBarButton buttonOverflow = null;

            AppBarToggleButton toggleButtonLabelsOnRight = null;
            AppBarToggleButton toggleButtonOverflow = null;

            RunOnUIThread.Execute(() =>
            {
                var cmdBar = (CommandBar)XamlReader.Load(@"
                        <CommandBar xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'
                            DefaultLabelPosition='Right'
                            IsOpen='True'>
                            <AppBarButton Label='button' Icon='Accept'/>
                            <AppBarToggleButton Label='button' Icon='Accept'/>
                            <CommandBar.SecondaryCommands>
                                <AppBarButton Label='Supercalifragilisticexpialidocious' Style='{StaticResource AppBarButtonRevealOverflowStyle}'/>
                                <AppBarToggleButton Label='Supercalifragilisticexpialidocious' Style='{StaticResource AppBarToggleButtonRevealOverflowStyle}'/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>");

                buttonLabelsOnRight = (AppBarButton)cmdBar.PrimaryCommands[0];
                buttonOverflow = (AppBarButton)cmdBar.SecondaryCommands[0];

                toggleButtonLabelsOnRight = (AppBarToggleButton)cmdBar.PrimaryCommands[1];
                toggleButtonOverflow = (AppBarToggleButton)cmdBar.SecondaryCommands[1];

                Content = cmdBar;
                Content.UpdateLayout();

                const double expectedLabelsOnRightWidth = 84;
                const double expectedOverflowWidth = 264;

                Verify.AreEqual(expectedLabelsOnRightWidth, buttonLabelsOnRight.ActualWidth);
                Verify.AreEqual(expectedOverflowWidth, buttonOverflow.ActualWidth);

                Verify.AreEqual(expectedLabelsOnRightWidth, toggleButtonLabelsOnRight.ActualWidth);
                Verify.AreEqual(expectedOverflowWidth, toggleButtonOverflow.ActualWidth);
            });
        }
    }
}
