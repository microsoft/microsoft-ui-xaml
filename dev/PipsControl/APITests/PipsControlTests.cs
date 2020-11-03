// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

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
    public class PipsControlTests : ApiTestBase
    {
        [TestMethod]
        public void BasicTest()
        {
            Log.Comment("PipsControl Basic Test");
        }
    }
}
