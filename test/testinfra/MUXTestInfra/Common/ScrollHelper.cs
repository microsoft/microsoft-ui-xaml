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
        public static void ScrollHorizontally(ScrollPresenter scrollPresenter, ScrollAmount amount)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollHorizontally scrollPresenter={0}, amount={1}, before-offset={2}.",
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                amount,
                scrollPresenter.HorizontalScrollPercent);

            scrollPresenter.ScrollHorizontal(amount);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.ScrollHorizontally after-offset={0}.",
                scrollPresenter.HorizontalScrollPercent);
        }

        public static void ScrollVertically(ScrollPresenter scrollPresenter, ScrollAmount amount)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollVertically scrollPresenter={0}, amount={1}, before-offset={2}.", 
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                amount,
                scrollPresenter.VerticalScrollPercent);

            scrollPresenter.ScrollVertical(amount);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.ScrollVertically after-offset={0}.",
                scrollPresenter.VerticalScrollPercent);
        }

        public static void Scroll(ScrollPresenter scrollPresenter, ScrollAmount horizontalAmount, ScrollAmount verticalAmount)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(horizontalAmount, ScrollAmount.NoAmount);
            Verify.AreNotEqual(verticalAmount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.Scroll scrollPresenter={0}, horizontalAmount={1}, verticalAmount={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                horizontalAmount,
                verticalAmount,
                scrollPresenter.HorizontalScrollPercent,
                scrollPresenter.VerticalScrollPercent);

            scrollPresenter.Scroll(horizontalAmount, verticalAmount);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.Scroll after-horizontal-offset={0}, after-vertical-offset={1}.",
                scrollPresenter.HorizontalScrollPercent,
                scrollPresenter.VerticalScrollPercent);
        }

        public static void SetHorizontalScrollPercent(ScrollPresenter scrollPresenter, double horizontalPercent)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Log.Comment("ScrollHelper.SetHorizontalScrollPercent scrollPresenter={0}, horizontalPercent={1}, before-horizontal-offset={2}.",
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                horizontalPercent,
                scrollPresenter.HorizontalScrollPercent);

            scrollPresenter.SetScrollPercent(horizontalPercent, -1 /*NoScroll*/);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetHorizontalScrollPercent after-horizontal-offset={0}.",
                scrollPresenter.HorizontalScrollPercent);
        }

        public static void SetVerticalScrollPercent(ScrollPresenter scrollPresenter, double verticalPercent)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetVerticalScrollPercent scrollPresenter={0}, verticalPercent={1}, before-vertical-offset={2}.",
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                verticalPercent,
                scrollPresenter.VerticalScrollPercent);

            scrollPresenter.SetScrollPercent(-1 /*NoScroll*/, verticalPercent);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.SetVerticalScrollPercent after-vertical-offset={0}.",
                scrollPresenter.VerticalScrollPercent);
        }

        public static void SetScrollPercent(ScrollPresenter scrollPresenter, double horizontalPercent, double verticalPercent)
        {
            Verify.IsNotNull(scrollPresenter);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetScrollPercent scrollPresenter={0}, horizontalPercent={1}, verticalPercent={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scrollPresenter.AutomationId) ? scrollPresenter.Name : scrollPresenter.AutomationId,
                horizontalPercent,
                verticalPercent,
                scrollPresenter.HorizontalScrollPercent,
                scrollPresenter.VerticalScrollPercent);

            scrollPresenter.SetScrollPercent(horizontalPercent, verticalPercent);
            Wait.ForScrollChanged(scrollPresenter, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetScrollPercent after-horizontal-offset={0}, after-vertical-offset={1}.",
                scrollPresenter.HorizontalScrollPercent,
                scrollPresenter.VerticalScrollPercent);
        }
    }
}
