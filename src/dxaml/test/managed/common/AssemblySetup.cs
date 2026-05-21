// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Microsoft.UI.Xaml.Markup;
using System.Collections;
using System.Collections.Generic;

using Private.Infrastructure;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using System.Runtime.CompilerServices;

namespace Microsoft.UI.Xaml.Tests.Common
{
    [TestClass]
    public static partial class AssemblySetup
    {
        private const string PrivateInfraServerDll = "Private.Infrastructure.Server.dll";

        [DllImport(PrivateInfraServerDll, PreserveSig = false)]
        private static extern void RpcServerStart();

        [DllImport(PrivateInfraServerDll, PreserveSig = false)]
        private static extern void RpcServerStop();

        [DllImport(PrivateInfraServerDll, PreserveSig = false)]
        private static extern void ConfigureResourcesPri(bool managed);

        [AssemblyInitialize]
        [TestProperty("RunFixtureAs:Module", "ElevatedUserOrSystem")]
        [TestProperty("EnsureLoggedOnUser:UserCount", "1")]
        [TestProperty("IsolationLevel", "Class")]
        [TestProperty("Tailored:Praid", "XamlManagedTAEFTests")]
        [TestProperty("CoreClrProfile", "localDotNet")]
        [TestProperty("ThreadingModel[@HostingMode='WPF']", "STA")]
        [TestProperty("UAP:Host[@HostingMode='WPF']", "PackagedCwa")]
        [TestProperty("UAP:AppXManifest[@HostingMode='WPF']", AppxManifests.WINDOWS_VERSION_CURRENT_CENTENNIAL)]
        [TestProperty("UAP:AppXManifest[default]", AppxManifests.WINDOWS_VERSION_CURRENT)]
        [TestProperty("UAP:WaitForXamlWindowActivation", "false")]
        [DeploymentItem(@"..\EtwProcessor.dll")]
        [TestProperty("Hosting:Mode", "WPF")] // TODO: Enable MUX Managed tests in UWP mode once .net5 is supported there.
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS4)]
    #if HELIX_CREATE_WORKITEM_PER_TESTCLASS
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
    #endif
        public static void RunModuleSetup(TestContext context)
        {
            Log.Comment("RunModuleSetup");

            // Configure the resources.pri file for managed tests
            ConfigureResourcesPri(true /* managed */);
            
    
            RpcServerStart();
        }

        public static void CommonTestClassSetup()
        {
            CommonTestSetupHelper.CommonTestClassSetup();
        }

        [AssemblyCleanup]
        public static void RunModuleCleanup()
        {
            Log.Comment("RunModuleCleanup");
            RpcServerStop();
        }
    }
}
