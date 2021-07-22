// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;
using SymbolIconSource = Microsoft.UI.Xaml.Controls.SymbolIconSource;

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
    public class InfoBadgeTests : ApiTestBase
    {
        [TestMethod]
        public void BasicTest()
        {
            InfoBadge infoBadge = null;
            SymbolIconSource symbolIconSource = null;
            RunOnUIThread.Execute(() =>
            {
                infoBadge = new InfoBadge();
                symbolIconSource = new SymbolIconSource();
                symbolIconSource.Symbol = Symbol.Setting;

                Content = infoBadge;
                Content.UpdateLayout();
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                FrameworkElement textBlock = infoBadge.FindVisualChildByName("ValueTextBlock");
                Verify.IsNotNull(textBlock, "The underlying value text block could not be retrieved");

                FrameworkElement iconViewBox = infoBadge.FindVisualChildByName("IconPresenter");
                Verify.IsNotNull(textBlock, "The underlying icon presenter view box could not be retrieved");

                Verify.AreEqual(Visibility.Collapsed, textBlock.Visibility, "The value text block should be initally collapsed since the default value is -1");
                Verify.AreEqual(Visibility.Collapsed, iconViewBox.Visibility, "The icon presenter should be initally collapsed since the default value is null");

                infoBadge.IconSource = symbolIconSource;
                Content.UpdateLayout();

                Verify.AreEqual(Visibility.Collapsed, textBlock.Visibility, "The value text block should be initally collapsed since the default value is -1");
                Verify.AreEqual(Visibility.Visible, iconViewBox.Visibility, "The icon presenter should be visible since we've set the icon source property and value is -1");

                infoBadge.Value = 10;
                Content.UpdateLayout();

                Verify.AreEqual(Visibility.Visible, textBlock.Visibility, "The value text block should be visible since the value is set to 10");
                Verify.AreEqual(Visibility.Visible, iconViewBox.Visibility, "The icon presenter should be collapsed since we've set the icon source property but value is not -1");

                infoBadge.IconSource = null;
                Content.UpdateLayout();

                Verify.AreEqual(Visibility.Visible, textBlock.Visibility, "The value text block should be visible since the value is set to 10");
                Verify.AreEqual(Visibility.Collapsed, iconViewBox.Visibility, "The icon presenter should be collapsed since the icon source property is null");

                infoBadge.Value = -1;
                Content.UpdateLayout();

                Verify.AreEqual(Visibility.Collapsed, textBlock.Visibility, "The value text block should be collapsed since the value is set to -1");
                Verify.AreEqual(Visibility.Collapsed, iconViewBox.Visibility, "The icon presenter should be collapsed since the value is set to null");
            });
        }
    }
}
