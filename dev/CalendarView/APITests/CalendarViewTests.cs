// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;
using System.Linq;
using System.Collections.Generic;
using System;

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
    public class CalendarViewTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyVisualTree()
        {
            CalendarView calendarView = null;
            RunOnUIThread.Execute(() =>
            {
                calendarView = new CalendarView();
                calendarView.Width = 400;
                calendarView.Height = 400;
                calendarView.SetDisplayDate(new DateTime(2000, 1, 1));
            });
            TestUtilities.SetAsVisualTreeRoot(calendarView);

            VisualTreeTestHelper.VerifyVisualTree(root: calendarView, verificationFileNamePrefix: "CalendarView");
        }

        [TestMethod]
        public void VerifyToolTips()
        {
            RunOnUIThread.Execute(() =>
            {
                var calendarView = new CalendarView();

                Content = calendarView;
                Content.UpdateLayout();

                var previousButton = calendarView.FindVisualChildByName("PreviousButton") as Button;
                var nextButton = calendarView.FindVisualChildByName("NextButton") as Button;

                Verify.AreEqual("Previous", ToolTipService.GetToolTip(previousButton));
                Verify.AreEqual("Next", ToolTipService.GetToolTip(nextButton));
            });
        }
    }
}
