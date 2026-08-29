// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;
using Common;
using Microsoft.UI.Dispatching;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp.Utilities
{
    public class RunOnUIThread
    {
        public static void Execute(Action action)
        {
            Execute(IdleSynchronizer.DispatcherQueue, action);
        }

        public static void Execute(CoreApplicationView whichView, Action action)
        {
            // TODO: Figure out how to implement this without needing CoreApplicationView.
            throw new NotImplementedException();
        }

        public static void Execute(DispatcherQueue dispatcherQueue, Action action)
        {
            Exception exception = null;

            if (dispatcherQueue.HasThreadAccess)
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
                    if (dispatcherQueue.HasThreadAccess)
                    {
                        try
                        {
                            action();
                        }
                        catch (Exception e)
                        {
                            exception = e;
                            throw;
                        }
                        finally // Unblock calling thread even if action() throws
                        {
                            workComplete.Set();
                        }
                    }
                    else
                    {
                        // Otherwise queue the work to the UI thread and then set the completion event on that thread.
                        var ignore = dispatcherQueue.TryEnqueue(DispatcherQueuePriority.Normal,
                            () =>
                            {
                                try
                                {
                                    action();
                                }
                                catch (Exception e)
                                {
                                    exception = e;
                                    throw;
                                }
                                finally // Unblock calling thread even if action() throws
                                {
                                    workComplete.Set();
                                }
                            });
                    }
                });

                workComplete.WaitOne();
                if (exception != null)
                {
                    Verify.Fail("Exception thrown by action on the UI thread: " + exception.ToString());
                }
            }
        }

        public static void WaitForTick()
        {
            var renderingEvent = new ManualResetEvent(false);

            EventHandler<object> renderingHandler = (object sender, object args) =>
            {
                renderingEvent.Set();
            };

            RunOnUIThread.Execute(() =>
            {
                CompositionTarget.Rendering += renderingHandler;
            });

            renderingEvent.WaitOne();

            RunOnUIThread.Execute(() =>
            {
                CompositionTarget.Rendering -= renderingHandler;
            });
        }

    }
}
