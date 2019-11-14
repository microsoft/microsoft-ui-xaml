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
    public class DropDownButton : UIObject, IExpandCollapse
    {
        public DropDownButton(UIObject uiObject)
            : base(uiObject)
        {
            this.Initialize();
        }

        private void Initialize()
        {
            _expandCollapsePattern = new ExpandCollapseImplementation(this);
        }

        public void ExpandAndWait()
        {
            using (var waiter = GetExpandedWaiter())
            {
                Expand();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public void CollapseAndWait()
        {
            using (var waiter = GetCollapsedWaiter())
            {
                Collapse();
                waiter.Wait();
            }

            Wait.ForIdle();
        }

        public void Expand()
        {
            _expandCollapsePattern.Expand();
        }

        public void Collapse()
        {
            _expandCollapsePattern.Collapse();
        }

        public UIEventWaiter GetExpandedWaiter()
        {
            return _expandCollapsePattern.GetExpandedWaiter();
        }

        public UIEventWaiter GetCollapsedWaiter()
        {
            return _expandCollapsePattern.GetCollapsedWaiter();
        }

        public ExpandCollapseState ExpandCollapseState
        {
            get { return _expandCollapsePattern.ExpandCollapseState; }
        }

        new public static IFactory<DropDownButton> Factory
        {
            get
            {
                if (null == DropDownButton._factory)
                {
                    DropDownButton._factory = new DropDownButtonFactory();
                }
                return DropDownButton._factory;
            }
        }

        private IExpandCollapse _expandCollapsePattern;
        private static IFactory<DropDownButton> _factory = null;

        private class DropDownButtonFactory : IFactory<DropDownButton>
        {
            public DropDownButton Create(UIObject element)
            {
                return new DropDownButton(element);
            }
        }
    }

    [TestClass]
    public class DropDownButtonTests
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
        public void AccessibilityTest()
        {
            using (var setup = new TestSetupHelper("DropDownButton Tests"))
            {
                DropDownButton dropDownButton = FindElement.ByName<DropDownButton>("TestDropDownButton");
                
                TextBlock flyoutOpenedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutOpenedCountTextBlock");
                TextBlock flyoutClosedCountTextBlock = FindElement.ByName<TextBlock>("FlyoutClosedCountTextBlock");

                Log.Comment("Verify that DropDownButton implements IExpandCollapseProvider");
                Verify.AreEqual("0", flyoutOpenedCountTextBlock.DocumentText);
                Log.Comment("IExpandCollapseProvider.Expand");
                dropDownButton.ExpandAndWait();
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);
                VerifyElement.Found("TestFlyout", FindBy.Name);
                Verify.AreEqual(dropDownButton.ExpandCollapseState, ExpandCollapseState.Expanded);

                Verify.AreEqual("0", flyoutClosedCountTextBlock.DocumentText);
                Log.Comment("IExpandCollapseProvider.Collapse");
                dropDownButton.CollapseAndWait();
                Wait.ForIdle();
                Verify.AreEqual("1", flyoutClosedCountTextBlock.DocumentText);
                Verify.AreEqual(dropDownButton.ExpandCollapseState, ExpandCollapseState.Collapsed);

                Log.Comment("Change to a different flyout");
                CheckBox setFlyoutCheckBox = FindElement.ByName<CheckBox>("SetFlyoutCheckbox");
                setFlyoutCheckBox.Check();
                Wait.ForIdle();
                
                Verify.AreEqual("1", flyoutOpenedCountTextBlock.DocumentText);
                Log.Comment("IExpandCollapseProvider.Expand");
                dropDownButton.ExpandAndWait();
                Wait.ForIdle();
                Verify.AreEqual("2", flyoutOpenedCountTextBlock.DocumentText);
                Verify.AreEqual(dropDownButton.ExpandCollapseState, ExpandCollapseState.Expanded);

                Verify.AreEqual("1", flyoutClosedCountTextBlock.DocumentText);
                Log.Comment("IExpandCollapseProvider.Collapse");
                dropDownButton.CollapseAndWait();
                Wait.ForIdle();
                Verify.AreEqual("2", flyoutClosedCountTextBlock.DocumentText);
                Verify.AreEqual(dropDownButton.ExpandCollapseState, ExpandCollapseState.Collapsed);
            }
        }
    }
}