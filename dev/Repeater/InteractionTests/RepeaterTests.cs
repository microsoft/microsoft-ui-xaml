// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RepeaterTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void FocusedItemGetsRecycledUponCollectionReset()
        {
            Button navButton = new Button(FindElement.ByName("ItemsRepeater Tests"));
            navButton.InvokeAndWait();

            navButton = new Button(FindElement.ByName("Collection Changes Demo"));
            navButton.InvokeAndWait();

            Random r = new Random();
            int index = r.Next(1, 10);
            string elementToRemove = "item" + index;
            Button someRandomElementToRemove = new Button(FindElement.ByName(elementToRemove));
            someRandomElementToRemove.InvokeAndWait();

            bool[] foundButtons = new bool[10];

            // Save all buttons we found
            for (int i = 0; i < 10; i++)
            {
                Button currentButton = new Button(FindElement.ByName("item" + i));
                int buttonIndex = int.Parse(currentButton.Name.Replace("item", ""));
                foundButtons[buttonIndex] = true;
            }

            // Check if every button is present EXCEPT the randomly selected and thus removed button
            for(int i = 0; i < 10; i++)
            {
                if(i == index)
                {
                    Verify.IsFalse(foundButtons[i]);
                }
                else
                {
                    Verify.IsTrue(foundButtons[i]);
                }
            }
        }

    }
}
