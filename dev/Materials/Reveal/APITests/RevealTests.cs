// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using MUXControlsTestApp.Utilities;

using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Markup;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RevealTests
    {
        //Bug 19007647: ApiTests.RevealTests.ValidateAppBarButtonRevealStyles failing in master
        //[TestMethod]
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
                            <AppBarButton Label='button' Icon='Accept' Style='{StaticResource AppBarButtonRevealLabelsOnRightStyle}'/>
                            <AppBarToggleButton Label='button' Icon='Accept' Style='{StaticResource AppBarToggleButtonRevealLabelsOnRightStyle}'/>
                            <CommandBar.SecondaryCommands>
                                <AppBarButton Label='Supercalifragilisticexpialidocious' Style='{StaticResource AppBarButtonRevealOverflowStyle}'/>
                                <AppBarToggleButton Label='Supercalifragilisticexpialidocious' Style='{StaticResource AppBarToggleButtonRevealOverflowStyle}'/>
                            </CommandBar.SecondaryCommands>
                        </CommandBar>");

                buttonLabelsOnRight = (AppBarButton)cmdBar.PrimaryCommands[0];
                buttonOverflow = (AppBarButton)cmdBar.SecondaryCommands[0];

                toggleButtonLabelsOnRight = (AppBarToggleButton)cmdBar.PrimaryCommands[1];
                toggleButtonOverflow = (AppBarToggleButton)cmdBar.SecondaryCommands[1];

                MUXControlsTestApp.App.TestContentRoot = cmdBar;
            });
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                const double expectedLabelsOnRightWidth = 88;
                const double expectedOverflowWidth = 264;

                Verify.AreEqual(expectedLabelsOnRightWidth, buttonLabelsOnRight.ActualWidth);
                Verify.AreEqual(expectedOverflowWidth, buttonOverflow.ActualWidth);

                Verify.AreEqual(expectedLabelsOnRightWidth, toggleButtonLabelsOnRight.ActualWidth);
                Verify.AreEqual(expectedOverflowWidth, toggleButtonOverflow.ActualWidth);
            });
        }
    }
}
