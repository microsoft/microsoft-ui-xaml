// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;

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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public static class ScrollHelper
    {
        public static void ScrollHorizontally(ScrollingPresenter scrollingPresenter, ScrollAmount amount)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollHorizontally scrollingPresenter={0}, amount={1}, before-offset={2}.",
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                amount,
                scrollingPresenter.HorizontalScrollPercent);

            scrollingPresenter.ScrollHorizontal(amount);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.ScrollHorizontally after-offset={0}.",
                scrollingPresenter.HorizontalScrollPercent);
        }

        public static void ScrollVertically(ScrollingPresenter scrollingPresenter, ScrollAmount amount)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollVertically scrollingPresenter={0}, amount={1}, before-offset={2}.", 
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                amount,
                scrollingPresenter.VerticalScrollPercent);

            scrollingPresenter.ScrollVertical(amount);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.ScrollVertically after-offset={0}.",
                scrollingPresenter.VerticalScrollPercent);
        }

        public static void Scroll(ScrollingPresenter scrollingPresenter, ScrollAmount horizontalAmount, ScrollAmount verticalAmount)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(horizontalAmount, ScrollAmount.NoAmount);
            Verify.AreNotEqual(verticalAmount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.Scroll scrollingPresenter={0}, horizontalAmount={1}, verticalAmount={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                horizontalAmount,
                verticalAmount,
                scrollingPresenter.HorizontalScrollPercent,
                scrollingPresenter.VerticalScrollPercent);

            scrollingPresenter.Scroll(horizontalAmount, verticalAmount);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.Scroll after-horizontal-offset={0}, after-vertical-offset={1}.",
                scrollingPresenter.HorizontalScrollPercent,
                scrollingPresenter.VerticalScrollPercent);
        }

        public static void SetHorizontalScrollPercent(ScrollingPresenter scrollingPresenter, double horizontalPercent)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Log.Comment("ScrollHelper.SetHorizontalScrollPercent scrollingPresenter={0}, horizontalPercent={1}, before-horizontal-offset={2}.",
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                horizontalPercent,
                scrollingPresenter.HorizontalScrollPercent);

            scrollingPresenter.SetScrollPercent(horizontalPercent, -1 /*NoScroll*/);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetHorizontalScrollPercent after-horizontal-offset={0}.",
                scrollingPresenter.HorizontalScrollPercent);
        }

        public static void SetVerticalScrollPercent(ScrollingPresenter scrollingPresenter, double verticalPercent)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetVerticalScrollPercent scrollingPresenter={0}, verticalPercent={1}, before-vertical-offset={2}.",
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                verticalPercent,
                scrollingPresenter.VerticalScrollPercent);

            scrollingPresenter.SetScrollPercent(-1 /*NoScroll*/, verticalPercent);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.SetVerticalScrollPercent after-vertical-offset={0}.",
                scrollingPresenter.VerticalScrollPercent);
        }

        public static void SetScrollPercent(ScrollingPresenter scrollingPresenter, double horizontalPercent, double verticalPercent)
        {
            Verify.IsNotNull(scrollingPresenter);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetScrollPercent scrollingPresenter={0}, horizontalPercent={1}, verticalPercent={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scrollingPresenter.AutomationId) ? scrollingPresenter.Name : scrollingPresenter.AutomationId,
                horizontalPercent,
                verticalPercent,
                scrollingPresenter.HorizontalScrollPercent,
                scrollingPresenter.VerticalScrollPercent);

            scrollingPresenter.SetScrollPercent(horizontalPercent, verticalPercent);
            Wait.ForScrollChanged(scrollingPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetScrollPercent after-horizontal-offset={0}, after-vertical-offset={1}.",
                scrollingPresenter.HorizontalScrollPercent,
                scrollingPresenter.VerticalScrollPercent);
        }
    }
}
