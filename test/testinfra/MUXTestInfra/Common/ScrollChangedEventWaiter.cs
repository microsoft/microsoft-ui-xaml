// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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
    public enum ScrollProperty
    {
        HorizontallyScrollable,
        HorizontalScrollPercent,
        HorizontalViewSize,
        VerticallyScrollable,
        VerticalScrollPercent,
        VerticalViewSize
    }

    public class ScrollChangedEventWaiter : UIEventWaiter
    {
        private ScrollProperty scrollProperty;
        private UIObject root = null;
        private double expectedDoubleValue = double.NaN;
        private bool? expectedBoolValue = null;

        public ScrollChangedEventWaiter(IScroll root, ScrollProperty scrollProperty, double expectedValue)
            : base(new PropertyChangedEventSource(root as UIObject, Scope.Element, UIProperty.Get("Scroll." + scrollProperty.ToString())))
        {
            Log.Comment("ScrollChangedEventWaiter - Constructor for scrollProperty={0} and expectedValue={1}.",
                scrollProperty.ToString(), expectedValue.ToString());

            this.root = root as UIObject;
            this.scrollProperty = scrollProperty;
            this.expectedDoubleValue = expectedValue;

            if (!double.IsNaN(this.expectedDoubleValue))
            {
                switch (this.scrollProperty)
                {
                    case ScrollProperty.HorizontalScrollPercent:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.HorizontalScrollPercent == expectedValue));
                        break;
                    case ScrollProperty.HorizontalViewSize:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.HorizontalViewSize == expectedValue));
                        break;
                    case ScrollProperty.VerticalScrollPercent:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.VerticalScrollPercent == expectedValue));
                        break;
                    case ScrollProperty.VerticalViewSize:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.VerticalViewSize == expectedValue));
                        break;
                }
            }
            this.Reset();
        }

        public ScrollChangedEventWaiter(IScroll root, ScrollProperty scrollProperty, bool? expectedValue)
            : base(new PropertyChangedEventSource(root as UIObject, Scope.Element, UIProperty.Get("Scroll." + scrollProperty.ToString())))
        {
            Log.Comment("ScrollChangedEventWaiter - Constructor for scrollProperty={0} and expectedValue={1}.",
                scrollProperty.ToString(), expectedValue == null ? "null" : expectedValue.ToString());

            this.root = root as UIObject;
            this.scrollProperty = scrollProperty;
            this.expectedBoolValue = expectedValue;

            if (this.expectedBoolValue != null)
            {
                switch (this.scrollProperty)
                {
                    case ScrollProperty.HorizontallyScrollable:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.HorizontallyScrollable == expectedValue));
                        break;
                    case ScrollProperty.VerticallyScrollable:
                        this.AddFilter(new Predicate<WaiterEventArgs>(args => root.VerticallyScrollable == expectedValue));
                        break;
                }
            }
        }

        public override string ToString()
        {
            if (!double.IsNaN(this.expectedDoubleValue))
            {
                return string.Format("ScrollChangedEventWaiter - Condition: {0} of {1} changed to '{2}'.", 
                    this.scrollProperty, this.root.AutomationId, this.expectedDoubleValue);
            }
            else if (this.expectedBoolValue != null)
            {
                return string.Format("ScrollChangedEventWaiter - Condition: {0} of {1} changed to '{2}'.",
                    this.scrollProperty, this.root.AutomationId, this.expectedBoolValue);
            }
            else
            {
                return string.Format("ScrollChangedEventWaiter - Condition: {0} of {1} changed.",
                    this.scrollProperty, this.root.AutomationId);
            }
        }

        protected override void OnNotify(WaiterEventArgs e)
        {
            string newValue = string.Empty;
            switch (this.scrollProperty)
            {
                case ScrollProperty.HorizontallyScrollable:
                    newValue = (this.root as IScroll).HorizontallyScrollable.ToString();
                    break;
                case ScrollProperty.HorizontalScrollPercent:
                    newValue = (this.root as IScroll).HorizontalScrollPercent.ToString();
                    break;
                case ScrollProperty.HorizontalViewSize:
                    newValue = (this.root as IScroll).HorizontalViewSize.ToString();
                    break;
                case ScrollProperty.VerticallyScrollable:
                    newValue = (this.root as IScroll).VerticallyScrollable.ToString();
                    break;
                case ScrollProperty.VerticalScrollPercent:
                    newValue = (this.root as IScroll).VerticalScrollPercent.ToString();
                    break;
                case ScrollProperty.VerticalViewSize:
                    newValue = (this.root as IScroll).VerticalViewSize.ToString();
                    break;
            }
            Log.Comment("ScrollChangedEventWaiter - OnNotify {0}={1}", this.scrollProperty.ToString(), newValue);
            base.OnNotify(e);
        }
    }
}
