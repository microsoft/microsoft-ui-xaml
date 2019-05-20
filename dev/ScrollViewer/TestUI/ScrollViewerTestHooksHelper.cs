// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
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
        public ScrollViewerTestHooksHelper(bool? autoHideScrollControllers = null)
        {
            RunOnUIThread.Execute(() =>
            {
                AutoHideScrollControllers = autoHideScrollControllers;
            });
        }

        public bool? AutoHideScrollControllers
        {
            get
            {
                return ScrollViewerTestHooks.AutoHideScrollControllers;
            }

            set
            {
                if (value != AutoHideScrollControllers)
                {
                    Log.Comment($"ScrollViewerTestHooksHelper: AutoHideScrollControllers set to {value}.");
                    ScrollViewerTestHooks.AutoHideScrollControllers = value;
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
