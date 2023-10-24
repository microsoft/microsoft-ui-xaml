// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using CustomTasks;
using Microsoft.Build.Framework;
using Microsoft.Build.Utilities;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace CustomTasksTool
{
    class Program
    {
        private static readonly string projectRoot = Path.GetFullPath(Path.Combine(Assembly.GetExecutingAssembly().Location, @"..\..\..\..\..\..\..")) + @"\";
        private static readonly string configuration = Environment.GetEnvironmentVariable("_BuildType") ?? "chk";
        private static readonly string platform = Environment.GetEnvironmentVariable("_BuildArch") ?? "x86";

        static void Main(string[] args)
        {
            // This tool is here to let you F5 the tasks and debug them more easily than through MSBuild.
            //TestBatchMergeXaml();
            //TestDependencyPropertyCodeGen();
            //TestTransformTemplate();
            TestGenerateWinRTClassRegistrations();
        }

        private static void TestBatchMergeXaml()
        {
            BatchMergeXaml test = GetBatchMergeXamlTaskFromParameters(Path.Combine(Environment.GetEnvironmentVariable("LOCALAPPDATA"), "Temp", "BatchMergeXamlParams.txt"));
            test.Execute();
        }

        private static void TestDependencyPropertyCodeGen()
        {
            string windowsKitsFolder = (string)Microsoft.Win32.Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows Kits\Installed Roots").GetValue("KitsRoot10");
            string referenceWinMDFolder = $@"{windowsKitsFolder}References\10.0.18362.0\";

            // We'll retrieve the most recently built WinMD file and use that, under the assumption that that's the one we care about.
            string mostRecentProjectWinMDPath = string.Empty;
            string projectWinMDPathPattern = projectRoot + @"BuildOutput\obj\{1}{0}\controls\dev\dll\Unmerged\Microsoft.UI.Xaml.Controls.g.winmd";
            DateTime mostRecentWriteTime = new DateTime();

            foreach (string configuration in new string[] { "chk", "fre" })
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

            var test = new DependencyPropertyCodeGen {
                WinMDInput = new string[] { mostRecentProjectWinMDPath },
                References = new string[] {
                    $@"{projectRoot}packages\Microsoft.Internal.WinUIDetails.0.1.0-ci195\lib\uap10.0\Microsoft.UI.Text.winmd",
                    $@"{projectRoot}packages\Microsoft.Web.WebView2.1.0.1823.32\lib\Microsoft.Web.WebView2.Core.winmd",
                    $@"{projectRoot}packages\Microsoft.WindowsAppSDK.Foundation.TransportPackage\1.1.0-20220628.1.develop.nightly\lib\uap10.0\Microsoft.Windows.ApplicationModel.Resources.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Foundation.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Graphics.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Xaml.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.AI.MachineLearning.MachineLearningContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ActivatedEventsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ActivationCameraSettingsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ContactActivatedEventsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Background.BackgroundAlarmApplicationContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.Background.CallsBackgroundContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.CallsPhoneContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.CallsVoipContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.LockScreenCallContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.FullTrustAppContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Preview.InkWorkspace.PreviewInkWorkspaceContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Preview.Notes.PreviewNotesContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Resources.Management.ResourceIndexerContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Search.Core.SearchCoreContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Search.SearchContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.SocialInfo.SocialInfoContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.StartupTaskContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Wallet.WalletContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Custom.CustomDeviceContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.DevicesLowLevelContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Portable.PortableDeviceContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Printers.Extensions.ExtensionsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Printers.PrintersContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Scanners.ScannerDeviceContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.SmartCards.SmartCardEmulatorContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Sms.LegacySmsApiContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Embedded.DeviceLockdown.DeviceLockdownContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Foundation.FoundationContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Foundation.UniversalApiContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.Input.GamingInputPreviewContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.Preview.GamesEnumerationContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.UI.GameChatOverlayContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.UI.GamingUIProviderContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.XboxLive.StorageApiContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Globalization.GlobalizationJapanesePhoneticAnalyzerContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Graphics.Printing3D.Printing3DContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Management.Deployment.Preview.DeploymentPreviewContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Management.Workplace.WorkplaceSettingsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.AppBroadcasting.AppBroadcastingContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.AppRecording.AppRecordingContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppBroadcastContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppCaptureContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppCaptureMetadataContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.CameraCaptureUIContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.GameBarContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Devices.CallControlContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.MediaControlContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Playlists.PlaylistsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Protection.ProtectionRenewalContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.Connectivity.WwanContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.NetworkOperators.LegacyNetworkOperatorsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.NetworkOperators.NetworkOperatorsFdnContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.Sockets.ControlChannelTriggerContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.XboxLive.XboxLiveSecureSocketsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Perception.Automation.Core.PerceptionAutomationCoreContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Phone.PhoneContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Phone.StartScreen.DualSimTileContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Security.EnterpriseData.EnterpriseDataContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Security.ExchangeActiveSyncProvisioning.EasContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Maps.GuidanceContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Maps.LocalSearchContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Store.StoreContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.TargetedContent.TargetedContentContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Storage.Provider.CloudFilesContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileHardwareTokenContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileRetailInfoContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileSharedModeContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.SystemManufacturers.SystemManufacturersContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.SystemManagementContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.UserProfile.UserProfileContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.UserProfile.UserProfileLockScreenContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.ApplicationSettings.ApplicationsSettingsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Core.AnimationMetrics.AnimationMetricsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Core.CoreWindowDialogsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Shell.SecurityAppManagerContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.ViewManagement.ViewManagementViewScalingContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.WebUI.Core.WebUICommandBarContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Xaml.Core.Direct.XamlDirectContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Xaml.Hosting.HostingContract.winmd",
                    $@"{projectRoot}packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Web.Http.Diagnostics.HttpDiagnosticsContract.winmd",
                    $@"{projectRoot}packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage\1.2.0-CI-25151.1000.220627-1000.0\lib\uap10.0.18362\Microsoft.Foundation.winmd",
                    $@"{projectRoot}packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage\1.2.0-CI-25151.1000.220627-1000.0\lib\uap10.0.18362\Microsoft.Graphics.winmd",
                    $@"{projectRoot}packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage\1.2.0-CI-25151.1000.220627-1000.0\lib\uap10.0.18362\Microsoft.UI.winmd",
                    $@"{projectRoot}packages\Microsoft.WindowsAppSDK.Foundation.TransportPackage\1.1.0-20220628.1.develop.nightly\lib\uap10.0\Microsoft.Windows.ApplicationModel.Resources.winmd",
                    $@"{projectRoot}packages\Microsoft.Internal.WinUIDetails.0.1.0-ci195\lib\uap10.0\Microsoft.UI.Text.winmd",
                    $@"{projectRoot}packages\Microsoft.Web.WebView2.1.0.1823.32\lib\Microsoft.Web.WebView2.Core.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Foundation.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Graphics.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.winmd",
                    $@"{projectRoot}BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Xaml.winmd"
                },

                OutputDirectory = $@"{projectRoot}controls\dev\Generated"
            };

            test.Execute();
        }

        // BatchMergeXaml has a *ton* of parameters, making defining it programmatically difficult.
        // To get around that, this provides a system where you can dump its parameters from a binlog
        // to a file and then read those parameters.  To do that, save a build to a binlog (msbuild /bl
        // via command line or via the Project System Tools extension for VS at
        // https://marketplace.visualstudio.com/items?itemName=VisualStudioProductTeam.ProjectSystemTools),
        // then open it up using the binlog viewer (https://msbuildlog.com/), search for "$task BatchMergeXaml",
        // click on "Parameters", Ctrl+C to copy the whole thing, paste that to a text file, and then point this
        // at that text file.
        private static BatchMergeXaml GetBatchMergeXamlTaskFromParameters(string parametersFilePath)
        {
            BatchMergeXaml task = new BatchMergeXaml();

            string[] fileLines = File.ReadAllLines(parametersFilePath);

            for (int i = 0; i < fileLines.Length; i++)
            {
                if (fileLines[i].StartsWith("    MergedXamlFile"))
                {
                    task.MergedXamlFile = fileLines[i].Split('=')[1].Trim();
                }
                else if (fileLines[i].StartsWith("    PagesFilteredBy"))
                {
                    task.PagesFilteredBy = fileLines[i].Split('=')[1].Trim();
                }
                else if (fileLines[i].StartsWith("    TlogReadFilesOutputPath"))
                {
                    task.TlogReadFilesOutputPath = fileLines[i].Split('=')[1].Trim();
                }
                else if (fileLines[i].StartsWith("    TlogWriteFilesOutputPath"))
                {
                    task.TlogWriteFilesOutputPath = fileLines[i].Split('=')[1].Trim();
                }
                else if (fileLines[i] == "    Pages")
                {
                    task.Pages = GetTaskItemsFromLines(fileLines, ref i);
                }
            }

            return task;
        }

        private static TaskItem[] GetTaskItemsFromLines(string[] lines, ref int index)
        {
            List<TaskItem> taskItems = new List<TaskItem>();
            index++;

            while (lines[index].StartsWith("        "))
            {
                TaskItem pageItem = new TaskItem(lines[index].Trim());
                index++;

                while (lines[index].StartsWith("            "))
                {
                    Match match = Regex.Match(lines[index].Trim(), @"(\w+)\s*=\s*(\w+)");

                    if (match.Success)
                    {
                        pageItem.SetMetadata(match.Groups[1].Value, match.Groups[2].Value);
                    }

                    index++;
                }

                taskItems.Add(pageItem);
            }

            // The for loop will increment the index, so we want to back up.
            index--;
            return taskItems.ToArray();
        }

        private static void TestTransformTemplate()
        {
            TransformTemplate test = new TransformTemplate();
            test.OutputFile = $@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\controls\dev\dll\XamlMetadataProviderGenerated.h";
            test.Template = $@"{projectRoot}\controls\dev\dll\XamlMetadataProviderGenerated.tt";

            var parameterValues = new Dictionary<string, string>();
            parameterValues.Add("MetadataWinmdPaths", $@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\controls\dev\dll\XamlMetadataProviderGenerated.h");
            parameterValues.Add("PropertiesHeadersDirectory", $@"{projectRoot}\controls\dev\Generated");
            parameterValues.Add("ReferenceWinmds", $@"{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.AI.MachineLearning.MachineLearningContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.AI.MachineLearning.Preview.MachineLearningPreviewContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ActivatedEventsContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ActivationCameraSettingsContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.ContactActivatedEventsContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Activation.WebUISearchActivatedEventsContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Background.BackgroundAlarmApplicationContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.Background.CallsBackgroundContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.CallsPhoneContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.CallsVoipContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Calls.LockScreenCallContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.CommunicationBlocking.CommunicationBlockingContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.FullTrustAppContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Preview.InkWorkspace.PreviewInkWorkspaceContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Preview.Notes.PreviewNotesContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Resources.Management.ResourceIndexerContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Search.Core.SearchCoreContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Search.SearchContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.SocialInfo.SocialInfoContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.StartupTaskContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.ApplicationModel.Wallet.WalletContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Custom.CustomDeviceContract.winmd;{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.DevicesLowLevelContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Portable.PortableDeviceContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Printers.Extensions.ExtensionsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Printers.PrintersContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Scanners.ScannerDeviceContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.SmartCards.SmartCardBackgroundTriggerContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.SmartCards.SmartCardEmulatorContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Devices.Sms.LegacySmsApiContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Embedded.DeviceLockdown.DeviceLockdownContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Foundation.FoundationContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Foundation.UniversalApiContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.Input.GamingInputPreviewContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.Preview.GamesEnumerationContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.UI.GameChatOverlayContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.UI.GamingUIProviderContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Gaming.XboxLive.StorageApiContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Globalization.GlobalizationJapanesePhoneticAnalyzerContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Graphics.Printing3D.Printing3DContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Management.Deployment.Preview.DeploymentPreviewContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Management.Workplace.WorkplaceSettingsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.AppBroadcasting.AppBroadcastingContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.AppRecording.AppRecordingContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppBroadcastContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppCaptureContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.AppCaptureMetadataContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.CameraCaptureUIContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Capture.GameBarContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Devices.CallControlContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.MediaControlContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Playlists.PlaylistsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Media.Protection.ProtectionRenewalContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.Connectivity.WwanContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.NetworkOperators.LegacyNetworkOperatorsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.NetworkOperators.NetworkOperatorsFdnContract.WinMD;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.Sockets.ControlChannelTriggerContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Networking.XboxLive.XboxLiveSecureSocketsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Perception.Automation.Core.PerceptionAutomationCoreContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Phone.PhoneContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Phone.StartScreen.DualSimTileContract.WinMD;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Security.EnterpriseData.EnterpriseDataContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Security.ExchangeActiveSyncProvisioning.EasContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Maps.GuidanceContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Maps.LocalSearchContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.Store.StoreContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Services.TargetedContent.TargetedContentContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Storage.Provider.CloudFilesContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileHardwareTokenContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileRetailInfoContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.ProfileSharedModeContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.Profile.SystemManufacturers.SystemManufacturersContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.SystemManagementContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.UserProfile.UserProfileContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.System.UserProfile.UserProfileLockScreenContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.ApplicationSettings.ApplicationsSettingsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Core.AnimationMetrics.AnimationMetricsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Core.CoreWindowDialogsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.Shell.SecurityAppManagerContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.ViewManagement.ViewManagementViewScalingContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.UI.WebUI.Core.WebUICommandBarContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Microsoft.UI.Xaml.Core.Direct.XamlDirectContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Microsoft.UI.Xaml.Hosting.HostingContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0\Windows.Web.Http.Diagnostics.HttpDiagnosticsContract.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.Foundation.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.Graphics.DirectX.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.UI.Composition.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.UI.Input.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.UI.Hosting.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.UI.Dispatching.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage.0.9.0-CI-22435.1000.210807-1000.0\lib\uap10.0\Microsoft.UI.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.ProjectReunion.Foundation.TransportPackage.0.5.0-CI-20210408-224233\lib\uap10.0\Microsoft.ApplicationModel.Resources\Microsoft.ApplicationModel.Resources.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Internal.WinUIDetails.0.1.0-ci158\lib\uap10.0\Microsoft.UI.Text.winmd;D:\microsoft-ui-xaml-lift\packages\Microsoft.Web.WebView2.1.0.1823.32\lib\Microsoft.Web.WebView2.Core.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Foundation.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.Graphics.DirectX.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Composition.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Dispatching.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Hosting.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Input.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.winmd;D:\microsoft-ui-xaml-lift\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\merged\private\Merged\Microsoft.UI.Xaml.winmd");

            test.ParameterValues = new ITaskItem[]
                {
                    new TaskItem("ParameterValues", parameterValues)
                };

            test.Execute();
        }

        private static void TestGenerateWinRTClassRegistrations()
        {
            GenerateWinRTClassRegistrations test = new GenerateWinRTClassRegistrations {
                DefaultImplementationDll = "Microsoft.UI.Xaml.dll",
                OutputFilename = $@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\MergedWinMD\LiftedWinRTClassRegistrations.xml",
                Metadata_Dirs = new ITaskItem[] {
                    new TaskItem($@"{projectRoot}\packages\Microsoft.Windows.SDK.cpp.10.0.18362.5\c\References\10.0.18362.0"),
                    new TaskItem($@"{projectRoot}\packages\Microsoft.ProjectReunion.InteractiveExperiences.TransportPackage\1.3.2-CI-22623.1044.230301-1533.0\lib\uap10.0.18362")
                }
            };

            List<TaskItem> taskItemList = new List<TaskItem>();

            void AddTaskItem(string itemSpec, string implementationDll = "", bool merge = true)
            {
                var itemMetadata = new Dictionary<string, string>();

                if (!string.IsNullOrEmpty(implementationDll))
                {
                    itemMetadata.Add("ImplementationDll", implementationDll);
                }

                if (!merge)
                {
                    itemMetadata.Add("Merge", "False");
                }

                taskItemList.Add(new TaskItem(itemSpec, itemMetadata));
            }

            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\controls\microsoft.ui.xaml.controls.controls.winmd");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\controls\microsoft.ui.xaml.controls.controls2.winmd");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\controls\idl\Unmerged\Microsoft.UI.Xaml.Controls.g.winmd", "Microsoft.UI.Xaml.Controls.dll");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\core\microsoft.ui.xaml.coretypes.winmd");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\core\microsoft.ui.xaml.coretypes2.winmd");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\phone\idl\Microsoft.UI.Xaml.Phone.winmd", "Microsoft.UI.Xaml.Phone.dll");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\main\microsoft.ui.xaml.winmd");
            AddTaskItem($@"{projectRoot}\packages\Microsoft.Internal.WinUIDetails.1.7.0\lib\uap10.0\Microsoft.UI.Text.winmd", "WinUIEdit.dll");
            AddTaskItem($@"{projectRoot}\packages\Microsoft.Web.WebView2.1.0.1823.32\lib\Microsoft.Web.WebView2.Core.winmd", "Microsoft.Web.WebView2.Core.dll");
            AddTaskItem($@"{projectRoot}\BuildOutput\obj\{platform}{configuration}\dxaml\xcp\dxaml\idl\winrt\main\microsoft.ui.xaml.private.winmd");

            test.WinMDs = taskItemList.ToArray();

            test.Execute();
        }
    }
}
