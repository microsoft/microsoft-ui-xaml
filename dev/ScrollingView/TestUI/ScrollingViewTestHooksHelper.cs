// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml.Controls;
using System;

#if USING_TAEF
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollingViewTestHooks = Microsoft.UI.Private.Controls.ScrollingViewTestHooks;

namespace MUXControlsTestApp.Utilities
{
    // Utility class used to set up ScrollingView test hooks and automatically reset them when the instance gets disposed.
    public class ScrollingViewTestHooksHelper : IDisposable
    {
        public ScrollingViewTestHooksHelper(ScrollingView scrollingView, bool? autoHideScrollControllers = null)
        {
            RunOnUIThread.Execute(() =>
            {
                if (scrollingView != null)
                {
                    ScrollingView = scrollingView;

                    ScrollingViewTestHooks.SetAutoHideScrollControllers(scrollingView, autoHideScrollControllers);
                }
            });
        }

        public ScrollingView ScrollingView
        {
            get;
            set;
        }

        public bool? AutoHideScrollControllers
        {
            get
            {
                return ScrollingView == null ? true : ScrollingViewTestHooks.GetAutoHideScrollControllers(ScrollingView);
            }

            set
            {
                if (value != AutoHideScrollControllers)
                {
                    Log.Comment($"ScrollingViewTestHooksHelper: AutoHideScrollControllers set to {value}.");
                    if (ScrollingView != null)
                    {
                         ScrollingViewTestHooks.SetAutoHideScrollControllers(ScrollingView, value);
                    }
                }
            }
        }

        public void Dispose()
        {
            RunOnUIThread.Execute(() =>
            {
                Log.Comment("PrivateLoggingHelper disposal: Resetting AutoHideScrollControllers.");
                AutoHideScrollControllers = null;
            });
        }
    }
}
