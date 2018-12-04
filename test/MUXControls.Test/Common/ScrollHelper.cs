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

#if BUILD_WINDOWS || USING_TESTNET
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common
{
    public static class ScrollHelper
    {
        public static void ScrollHorizontally(Scroller scroller, ScrollAmount amount)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollHorizontally scroller={0}, amount={1}, before-offset={2}.",
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                amount,
                scroller.HorizontalScrollPercent);

            scroller.ScrollHorizontal(amount);
            Wait.ForScrollChanged(scroller, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.ScrollHorizontally after-offset={0}.",
                scroller.HorizontalScrollPercent);
        }

        public static void ScrollVertically(Scroller scroller, ScrollAmount amount)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(amount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.ScrollVertically scroller={0}, amount={1}, before-offset={2}.", 
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                amount,
                scroller.VerticalScrollPercent);

            scroller.ScrollVertical(amount);
            Wait.ForScrollChanged(scroller, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.ScrollVertically after-offset={0}.",
                scroller.VerticalScrollPercent);
        }

        public static void Scroll(Scroller scroller, ScrollAmount horizontalAmount, ScrollAmount verticalAmount)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(horizontalAmount, ScrollAmount.NoAmount);
            Verify.AreNotEqual(verticalAmount, ScrollAmount.NoAmount);
            Log.Comment("ScrollHelper.Scroll scroller={0}, horizontalAmount={1}, verticalAmount={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                horizontalAmount,
                verticalAmount,
                scroller.HorizontalScrollPercent,
                scroller.VerticalScrollPercent);

            scroller.Scroll(horizontalAmount, verticalAmount);
            Wait.ForScrollChanged(scroller, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.Scroll after-horizontal-offset={0}, after-vertical-offset={1}.",
                scroller.HorizontalScrollPercent,
                scroller.VerticalScrollPercent);
        }

        public static void SetHorizontalScrollPercent(Scroller scroller, double horizontalPercent)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Log.Comment("ScrollHelper.SetHorizontalScrollPercent scroller={0}, horizontalPercent={1}, before-horizontal-offset={2}.",
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                horizontalPercent,
                scroller.HorizontalScrollPercent);

            scroller.SetScrollPercent(horizontalPercent, -1 /*NoScroll*/);
            Wait.ForScrollChanged(scroller, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetHorizontalScrollPercent after-horizontal-offset={0}.",
                scroller.HorizontalScrollPercent);
        }

        public static void SetVerticalScrollPercent(Scroller scroller, double verticalPercent)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetVerticalScrollPercent scroller={0}, verticalPercent={1}, before-vertical-offset={2}.",
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                verticalPercent,
                scroller.VerticalScrollPercent);

            scroller.SetScrollPercent(-1 /*NoScroll*/, verticalPercent);
            Wait.ForScrollChanged(scroller, ScrollProperty.VerticalScrollPercent);

            Log.Comment("ScrollHelper.SetVerticalScrollPercent after-vertical-offset={0}.",
                scroller.VerticalScrollPercent);
        }

        public static void SetScrollPercent(Scroller scroller, double horizontalPercent, double verticalPercent)
        {
            Verify.IsNotNull(scroller);
            Verify.AreNotEqual(horizontalPercent, 0.0);
            Verify.AreNotEqual(verticalPercent, 0.0);
            Log.Comment("ScrollHelper.SetScrollPercent scroller={0}, horizontalPercent={1}, verticalPercent={2}, before-horizontal-offset={3}, before-vertical-offset={4}.",
                string.IsNullOrWhiteSpace(scroller.AutomationId) ? scroller.Name : scroller.AutomationId,
                horizontalPercent,
                verticalPercent,
                scroller.HorizontalScrollPercent,
                scroller.VerticalScrollPercent);

            scroller.SetScrollPercent(horizontalPercent, verticalPercent);
            Wait.ForScrollChanged(scroller, ScrollProperty.HorizontalScrollPercent);

            Log.Comment("ScrollHelper.SetScrollPercent after-horizontal-offset={0}, after-vertical-offset={1}.",
                scroller.HorizontalScrollPercent,
                scroller.VerticalScrollPercent);
        }
    }
}
