// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml.Controls;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.AppContainer;
using System;
using System.Collections.Generic;
using System.Linq;

namespace WinUICsUnitTestApp
{
    [TestClass]
    public class WinUICsUnitTestAppTests
    {
        [TestMethod]
        public void SampleTest1()
        {
            Assert.AreEqual(0, 0);
        }

        // Use the UITestMethod attribute for tests that need to run on the UI thread.
        [UITestMethod]
        public void SampleTest2()
        {
            var grid = new Grid();
            Assert.AreEqual(0, grid.MinWidth);

            WinUICsDesktopClassLibrary.ButtonControl control = new WinUICsDesktopClassLibrary.ButtonControl();
        }
    }
}
