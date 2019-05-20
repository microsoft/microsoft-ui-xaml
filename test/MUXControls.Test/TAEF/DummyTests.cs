using System;
using System.Collections.Generic;
using System.Text;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace MUXControls.Test
{
    [TestClass]
    public class DummyTests
    {
        [TestMethod]
        public void UnreliableTest()
        {
            Verify.IsTrue(new Random().Next() % 3 == 0);
        }
    }
}
