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
    public class ScrollPresenter : UIObject, IScroll
    {
        public ScrollPresenter(UIObject uiObject)
            : base(uiObject)
        {
            this.Initialize();
        }

        private void Initialize()
        {
            _scrollPattern = new ScrollImplementation(this);
        }

        public ScrollChangedEventWaiter GetScrollChangedWaiter(ScrollProperty scrollProperty, bool? expectedValue)
        {
            Log.Comment("ScrollPresenter.GetScrollChangedWaiter call for scrollProperty={0} and expectedValue={1}.",
                scrollProperty.ToString(), expectedValue == null ? "null" : expectedValue.ToString());
            return new ScrollChangedEventWaiter(this, scrollProperty, expectedValue);
        }

        public ScrollChangedEventWaiter GetScrollChangedWaiter(ScrollProperty scrollProperty, double expectedValue)
        {
            Log.Comment("ScrollPresenter.GetScrollChangedWaiter call for scrollProperty={0} and expectedValue={1}.", 
                scrollProperty.ToString(), expectedValue.ToString());
            return new ScrollChangedEventWaiter(this, scrollProperty, expectedValue);
        }

        new public static IFactory<ScrollPresenter> Factory
        {
            get
            {
                if (null == ScrollPresenter._factory)
                {
                    ScrollPresenter._factory = new ScrollPresenterFactory();
                }
                return ScrollPresenter._factory;
            }
        }

        #region IScroll Members

        public virtual bool HorizontallyScrollable
        {
            get
            {
                return _scrollPattern.HorizontallyScrollable;
            }
        }

        public virtual bool VerticallyScrollable
        {
            get
            {
                return _scrollPattern.VerticallyScrollable;
            }
        }

        public virtual double HorizontalScrollPercent
        {
            get
            {
                return _scrollPattern.HorizontalScrollPercent;
            }
        }

        public virtual double VerticalScrollPercent
        {
            get
            {
                return _scrollPattern.VerticalScrollPercent;
            }
        }

        public virtual double HorizontalViewSize
        {
            get
            {
                return _scrollPattern.HorizontalViewSize;
            }
        }

        public virtual double VerticalViewSize
        {
            get
            {
                return _scrollPattern.VerticalViewSize;
            }
        }

        public virtual void Scroll(ScrollAmount horizontalAmount, ScrollAmount verticalAmount)
        {
            _scrollPattern.Scroll(horizontalAmount, verticalAmount);
        }

        public virtual void ScrollHorizontal(ScrollAmount amount)
        {
            _scrollPattern.ScrollHorizontal(amount);
        }

        public virtual void ScrollVertical(ScrollAmount amount)
        {
            _scrollPattern.ScrollVertical(amount);
        }

        public virtual void SetScrollPercent(double horizontalPercent, double verticalPercent)
        {
            _scrollPattern.SetScrollPercent(horizontalPercent, verticalPercent);
        }

        #endregion

        private IScroll _scrollPattern;        
        private static IFactory<ScrollPresenter> _factory = null;

        private class ScrollPresenterFactory : IFactory<ScrollPresenter>
        {
            public ScrollPresenter Create(UIObject element)
            {
                return new ScrollPresenter(element);
            }
        }
    }
}
