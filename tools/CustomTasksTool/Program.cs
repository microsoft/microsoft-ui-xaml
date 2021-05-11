// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using CustomTasks;
using Microsoft.Build.Framework;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading.Tasks;

namespace CustomTasksTool
{
    class Program
    {
        static void Main(string[] args)
        {
            // This tool is here to let you F5 the tasks and debug them more easily than through MSBuild.
            string projectRoot = Path.GetFullPath(Path.Combine(Assembly.GetExecutingAssembly().Location, @"..\..\..\..\..\"));
            string windowsKitsFolder = (string)Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows Kits\Installed Roots").GetValue("KitsRoot10");
            string referenceWinMDFolder = $@"{windowsKitsFolder}References\10.0.18362.0\";

            // We'll retrieve the most recently built WinMD file and use that, under the assumption that that's the one we care about.
            string mostRecentProjectWinMDPath = string.Empty;
            string projectWinMDPathPattern = projectRoot + @"BuildOutput\{0}\{1}\Microsoft.UI.Xaml\Microsoft.winmd";
            DateTime mostRecentWriteTime = new DateTime();

            foreach (string configuration in new string[] { "Debug", "Release" })
            {
                foreach (string platform in new string[] { "x86", "x64" })
                {
                    string projectWinMDPath = string.Format(projectWinMDPathPattern, configuration, platform);
                    FileInfo projectWinMDFileInfo = new FileInfo(projectWinMDPath);

                    if (projectWinMDFileInfo.Exists && projectWinMDFileInfo.LastWriteTime > mostRecentWriteTime)
                    {
                        mostRecentWriteTime = projectWinMDFileInfo.LastWriteTime;
                        mostRecentProjectWinMDPath = projectWinMDFileInfo.FullName;
                    }
                }
            }

            var test = new DependencyPropertyCodeGen
            {
                WinMDInput = new string[] { mostRecentProjectWinMDPath },
                References = $@"{referenceWinMDFolder}Windows.AI.MachineLearning.MachineLearningContract\2.0.0.0\Windows.AI.MachineLearning.MachineLearningContract.winmd;{referenceWinMDFolder}Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract\2.0.0.0\Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.Calls.Background.CallsBackgroundContract\2.0.0.0\Windows.ApplicationModel.Calls.Background.CallsBackgroundContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.Calls.CallsPhoneContract\5.0.0.0\Windows.ApplicationModel.Calls.CallsPhoneContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.Calls.CallsVoipContract\4.0.0.0\Windows.ApplicationModel.Calls.CallsVoipContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract\2.0.0.0\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.SocialInfo.SocialInfoContract\2.0.0.0\Windows.ApplicationModel.SocialInfo.SocialInfoContract.winmd;{referenceWinMDFolder}Windows.ApplicationModel.StartupTaskContract\3.0.0.0\Windows.ApplicationModel.StartupTaskContract.winmd;{referenceWinMDFolder}Windows.Devices.Custom.CustomDeviceContract\1.0.0.0\Windows.Devices.Custom.CustomDeviceContract.winmd;{referenceWinMDFolder}Windows.Devices.DevicesLowLevelContract\3.0.0.0\Windows.Devices.DevicesLowLevelContract.winmd;{referenceWinMDFolder}Windows.Devices.Printers.PrintersContract\1.0.0.0\Windows.Devices.Printers.PrintersContract.winmd;{referenceWinMDFolder}Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract\3.0.0.0\Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract.winmd;{referenceWinMDFolder}Windows.Devices.SmartCards.SmartCardEmulatorContract\6.0.0.0\Windows.Devices.SmartCards.SmartCardEmulatorContract.winmd;{referenceWinMDFolder}Windows.Foundation.FoundationContract\3.0.0.0\Windows.Foundation.FoundationContract.winmd;{referenceWinMDFolder}Windows.Foundation.UniversalApiContract\8.0.0.0\Windows.Foundation.UniversalApiContract.winmd;{referenceWinMDFolder}Windows.Gaming.XboxLive.StorageApiContract\1.0.0.0\Windows.Gaming.XboxLive.StorageApiContract.winmd;{referenceWinMDFolder}Windows.Graphics.Printing3D.Printing3DContract\4.0.0.0\Windows.Graphics.Printing3D.Printing3DContract.winmd;{referenceWinMDFolder}Windows.Networking.Connectivity.WwanContract\2.0.0.0\Windows.Networking.Connectivity.WwanContract.winmd;{referenceWinMDFolder}Windows.Networking.Sockets.ControlChannelTriggerContract\3.0.0.0\Windows.Networking.Sockets.ControlChannelTriggerContract.winmd;{referenceWinMDFolder}Windows.Services.Maps.GuidanceContract\3.0.0.0\Windows.Services.Maps.GuidanceContract.winmd;{referenceWinMDFolder}Windows.Services.Maps.LocalSearchContract\4.0.0.0\Windows.Services.Maps.LocalSearchContract.winmd;{referenceWinMDFolder}Windows.Services.Store.StoreContract\4.0.0.0\Windows.Services.Store.StoreContract.winmd;{referenceWinMDFolder}Windows.Services.TargetedContent.TargetedContentContract\1.0.0.0\Windows.Services.TargetedContent.TargetedContentContract.winmd;{referenceWinMDFolder}Windows.System.Profile.ProfileHardwareTokenContract\1.0.0.0\Windows.System.Profile.ProfileHardwareTokenContract.winmd;{referenceWinMDFolder}Windows.System.Profile.ProfileSharedModeContract\2.0.0.0\Windows.System.Profile.ProfileSharedModeContract.winmd;{referenceWinMDFolder}Windows.System.Profile.SystemManufacturers.SystemManufacturersContract\3.0.0.0\Windows.System.Profile.SystemManufacturers.SystemManufacturersContract.winmd;{referenceWinMDFolder}Windows.System.SystemManagementContract\6.0.0.0\Windows.System.SystemManagementContract.winmd;{referenceWinMDFolder}Windows.UI.ViewManagement.ViewManagementViewScalingContract\1.0.0.0\Windows.UI.ViewManagement.ViewManagementViewScalingContract.winmd;{referenceWinMDFolder}Windows.UI.Xaml.Core.Direct.XamlDirectContract\2.0.0.0\Windows.UI.Xaml.Core.Direct.XamlDirectContract.winmd".Split(';'),
                OutputDirectory = $@"{projectRoot}dev\Generated"
            };

            test.Execute();
        }
    }
}
