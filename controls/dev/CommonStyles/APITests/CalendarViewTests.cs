// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using System.Linq;
using System.Collections.Generic;
using System;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class CalendarViewTests : ApiTestBase
    {
        //Issue #6649 Some tests fail when run on OS 21H1 
        [TestMethod]
        [TestProperty("Ignore", "True")] // TODO 29616788: Inconsistent visual tree after converting MUXControlsTestApp to a desktop .NET 5 app.  Re-enable when fixed.
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
    }
}
