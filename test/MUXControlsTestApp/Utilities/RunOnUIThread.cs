// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

namespace MUXControlsTestApp.Utilities
{
    public class RunOnUIThread
    {
        public static void Execute(Action action)
        {
            Execute(CoreApplication.MainView, action);
        }

        public static void Execute(CoreApplicationView whichView, Action action)
        {
            var dispatcher = whichView.Dispatcher;
            if (dispatcher.HasThreadAccess)
            {
                action();
            }
            else
            {
                // We're not on the UI thread, queue the work. Make sure that the action is not run until
                // the splash screen is dismissed (i.e. that the window content is present).
                var workComplete = new AutoResetEvent(false);
                App.RunAfterSplashScreenDismissed(() =>
                {
                    // If the Splash screen dismissal happens on the UI thread, run the action right now.
                    if (dispatcher.HasThreadAccess)
                    {
                        try
                        {
                            action();
                        }
                        catch (Exception e)
                        {
                            Verify.Fail("Exception thrown by action on the UI thread: " + e.ToString());
                        }
                        finally // Unblock calling thread even if action() throws
                        {
                            workComplete.Set();
                        }
                    }
                    else
                    {
                        // Otherwise queue the work to the UI thread and then set the completion event on that thread.
                        var ignore = dispatcher.RunAsync(CoreDispatcherPriority.Normal,
                            () => {
                                try
                                {
                                    action();
                                }
                                catch (Exception e)
                                {
                                    Verify.Fail("Exception thrown by action on the UI thread: " + e.ToString());
                                }
                                finally // Unblock calling thread even if action() throws
                                {
                                    workComplete.Set();
                                }
                            });
                    }
                });

                workComplete.WaitOne();
            }
        }

        public async Task WaitForTick()
        {
            var renderingEventFired = new TaskCompletionSource<object>();

            EventHandler<object> renderingCallback = (sender, arg) =>
            {
                renderingEventFired.TrySetResult(null);
            };
            CompositionTarget.Rendering += renderingCallback;

            await renderingEventFired.Task;

            CompositionTarget.Rendering -= renderingCallback;
        }

    }
}
