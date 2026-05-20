// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using WEX.Logging.Interop;

namespace Private.Infrastructure
{
    public static class TestServicesExtensions
    {
        public static void EnsureForegroundWindow()
        {
            // On desktop, check if there is any other window currently having the window focus
            // If so, try to bring the test app to foreground before attempting focus on the element
            if (TestServices.Utilities.IsDesktop &&
                !TestServices.WindowHelper.IsForegroundWindow)
            {
                Log.Comment("Test app does not have window focus, bring it to foreground and try again!");
                TestServices.WindowHelper.RestoreForegroundWindow();
                TestServices.WindowHelper.WaitForIdle();
            }
        }

        private static void CollectGarbage()
        {
            GC.Collect(2, GCCollectionMode.Forced, true);
            GC.WaitForPendingFinalizers();
        }

        public static void EnsureInitialized()
        {
            TestServices.EnsureInitialized();
            TestServices.WindowHelper.SetGCCollectCallback(CollectGarbage);
        }
    }
}
