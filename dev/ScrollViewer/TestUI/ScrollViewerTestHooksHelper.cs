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

using ScrollViewerTestHooks = Microsoft.UI.Private.Controls.ScrollViewerTestHooks;

namespace MUXControlsTestApp.Utilities
{
    // Utility class used to set up ScrollViewer test hooks and automatically reset them when the instance gets disposed.
    public class ScrollViewerTestHooksHelper : IDisposable
    {
        public ScrollViewerTestHooksHelper(ScrollViewer scrollViewer, bool? autoHideScrollControllers = null)
        {
            RunOnUIThread.Execute(() =>
            {
                if (scrollViewer != null)
                {
                    ScrollViewer = scrollViewer;

                    ScrollViewerTestHooks.SetAutoHideScrollControllers(scrollViewer, autoHideScrollControllers);
                }
            });
        }

        public ScrollViewer ScrollViewer
        {
            get;
            set;
        }

        public bool? AutoHideScrollControllers
        {
            get
            {
                return ScrollViewer == null ? true : ScrollViewerTestHooks.GetAutoHideScrollControllers(ScrollViewer);
            }

            set
            {
                if (value != AutoHideScrollControllers)
                {
                    Log.Comment($"ScrollViewerTestHooksHelper: AutoHideScrollControllers set to {value}.");
                    if (ScrollViewer != null)
                    {
                         ScrollViewerTestHooks.SetAutoHideScrollControllers(ScrollViewer, value);
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
