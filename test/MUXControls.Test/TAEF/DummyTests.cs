// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class DummyTests
    {
        [TestMethod]
        public void PassingTest()
        {
            Verify.IsTrue(true);
        }

        [TestMethod]
        public void FailingTest()
        {
            Verify.IsTrue(false);
        }

        [TestMethod]
        public void UnreliableTest()
        {
            Verify.IsTrue(new Random(DateTime.Now.Millisecond).Next() % 2 == 0);
        }
    }
}
