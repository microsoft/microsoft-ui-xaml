// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;
using Windows.Foundation.Metadata;

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
    public class Expander : UIObject, IExpandCollapse
    {
        public Expander(UIObject uiObject)
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

        new public static IFactory<Expander> Factory
        {
            get
            {
                if (null == Expander._factory)
                {
                    Expander._factory = new ExpanderFactory();
                }
                return Expander._factory;
            }
        }

        private IExpandCollapse _expandCollapsePattern;
        private static IFactory<Expander> _factory = null;
        private class ExpanderFactory : IFactory<Expander>
        {
            public Expander Create(UIObject element)
            {
                return new Expander(element);
            }
        }
    }

    [TestClass]
    public class ExpanderTests
    {
        public const string ExpandedExpanderAutomationId = "ExpandedExpander";
        public const string CollapsedExpanderAutomationId = "CollapsedExpander";
        public const string ExpandedExpanderContentAutomationId = "ExpandedExpanderContent";
        public const string CollapsedExpanderContentAutomationId = "CollapsedExpanderContent";

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void ExpandCollapseViaAutomation()
        {
            using (var setup = new TestSetupHelper("Expander Tests"))
            {
                Expander expander = FindElement.ByName<Expander>(ExpandedExpanderAutomationId);

                Log.Comment("Collapse using UIA ExpandCollapse pattern");
                expander.CollapseAndWait();
                //// Should be collapsed now
                Verify.AreEqual(expander.ExpandCollapseState, ExpandCollapseState.Collapsed);

                Log.Comment("Expand using UIA ExpandCollapse pattern");
                expander.ExpandAndWait();
                //// Should be expanded now
                Verify.AreEqual(expander.ExpandCollapseState, ExpandCollapseState.Expanded);
            }
        }

        [TestMethod]
        public void ExpandCollapseViaKeyboard()
        {
            using (var setup = new TestSetupHelper("Expander Tests"))
            {
                Expander expander = FindElement.ByName<Expander>(ExpandedExpanderAutomationId);

                Log.Comment("Collapse using keyboard space key.");
                expander.SetFocus();
                Wait.ForIdle();
                // This should collapse it.
                KeyboardHelper.PressKey(Key.Space);
                // Should be collapsed now
                Verify.AreEqual(expander.ExpandCollapseState, ExpandCollapseState.Collapsed);

                Log.Comment("Expand using keyboard space key.");
                expander.SetFocus();
                Wait.ForIdle();
                // This should expand it again.
                KeyboardHelper.PressKey(Key.Space);
                // Should be expanded now
                Verify.AreEqual(expander.ExpandCollapseState, ExpandCollapseState.Expanded);
            }
        }
    }
}
