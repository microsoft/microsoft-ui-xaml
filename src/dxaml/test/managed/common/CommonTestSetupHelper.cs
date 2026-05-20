// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Markup;

using Private.Infrastructure;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public static class CommonTestSetupHelper
    {
        private static bool _isInitialized = false;

        public static void CommonTestClassSetup()
        {
            if (!_isInitialized)
            {
                global::WinRT.ComWrappersSupport.InitializeComWrappers();
                ConfigureWin32Host();
                _isInitialized = true;
            }
            TestServicesExtensions.EnsureInitialized();
        }

        private static void ConfigureWin32Host()
        {
            global::Private.Infrastructure.TestHostSettings.Win32HostFactory = new global::Private.Infrastructure.Hosting.WPF.HostFactory();
        }

        public static void CommonTestClassCleanup()
        {
            //
            // Do a GC.Collect to clean up all DXaml peers.
            //
            // When a DXaml peer gets collected, we get a release call on the GC thread. If this call is the last
            // release, WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread will put it in the final release queue
            // for the DO to be revisited and deleted on the UI thread. This process requires a way to get from the
            // WeakReferenceSourceNoThreadId to the DXamlCore. Otherwise, we'll try to delete the DO on the GC thread
            // and hit an assert.
            //
            // The ShutdownXaml call below will call DXamlCore::ShutdownAllPeers and disconnect all reference trackers.
            // This cuts the link between all WeakReferenceSourceNoThreadId and the DXamlCore. So we have to make sure
            // all peers get collected before we call ShutdownXaml.
            //
            GC.Collect(2, GCCollectionMode.Forced, blocking: true);
            GC.WaitForPendingFinalizers();

            // An additonal GC Collect / Finalize is needed to avoid failures in a couple of the MoCoViewportBasicTests tests
            // which were failing post cleanup due to some objects (ListView) were getting cleaned up after the UI thread is gone.
            // The initial theory is this is due to in CsWinRT 1.3.3 there are fixes to better manage the lifetime of events
            // and that fix is requiring 2 GCs to clean up some objects.
            GC.Collect(2, GCCollectionMode.Forced, blocking: true);
            GC.WaitForPendingFinalizers();

            TestServices.WindowHelper.ShutdownXaml();
            TestServices.WindowHelper.VerifyTestCleanup();
        }
    }
}