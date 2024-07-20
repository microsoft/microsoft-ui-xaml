// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Windows.UI.Core;
using Windows.UI.ViewManagement;
using Microsoft.UI.Xaml;
using Common;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace MUXControlsTestApp.Utilities
{
    class ViewHelper
    {
        public static CoreApplicationView MakeSecondaryView(Action populateWindowContent)
        {
            CoreApplicationView secondaryView = null;
            AutoResetEvent secondaryViewShown = new AutoResetEvent(false);

            RunOnUIThread.Execute(async () =>
            {
                Log.Comment("Creating secondary view");

                // Make a secondary view and then show it.
                secondaryView = await MakeSecondaryViewFromUIThread(populateWindowContent);

                secondaryViewShown.Set();
            });

            IdleSynchronizer.Wait();
            Verify.IsTrue(secondaryViewShown.WaitOne(TimeSpan.FromMinutes(2)), "Waiting for secondary view to be shown");

            return secondaryView;
        }

        public static async Task<CoreApplicationView> MakeSecondaryViewFromUIThread(Action populateWindowContent)
        {
            Log.Comment("Creating secondary view");
            
            // TODO: Use window instead of view. TASK 30018331
            if (CoreWindow.GetForCurrentThread() == null)
            {
                Verify.Fail(String.Format("Test is not supported in Desktop due to dependency on CoreApplication. Update test to not run in WPF"));
                return null;
            }

            // Make a secondary view and then show it.
            var secondaryView = CoreApplication.CreateNewView();
            ApplicationView secondaryAppView = null;

            await secondaryView.Dispatcher.RunAsync(CoreDispatcherPriority.Normal, () =>
            {
                Log.Comment("Secondary view created, populating Window content");
                populateWindowContent();
                secondaryAppView = ApplicationView.GetForCurrentView();
                App.CurrentWindow.Activate();
            });

            Log.Comment("Secondary view activation complete, trying to show");
            var shown = await ApplicationViewSwitcher.TryShowAsStandaloneAsync(secondaryAppView.Id);
            
            Verify.IsTrue(shown, "Checking if TryShowAsStandaloneAsync succeeded");

            return secondaryView;
        }

        public static void CloseSecondaryView(CoreApplicationView view)
        {
            // Clean up.
            RunOnUIThread.Execute(view, () =>
            {
                Log.Comment("CloseSecondaryView: Window.Close");
                App.CurrentWindow.Close();
            });
            // Idle synchronizer throws an exception on RS1 because sometimes wait for animations times out. Just TryWait and move on.
            IdleSynchronizer.TryWait();
            Log.Comment("CloseSecondaryView: Done.");
        }
    }
}
