// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;

using WEX.Logging.Interop;
using WEX.TestExecution;
using Windows.Foundation;
using Windows.Management.Deployment;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    using System = System;

    public class TestAppInstallHelper
    {
        private static HashSet<string> TestAppxInstalled = new HashSet<string>();

        /// <summary>
        /// Installs the unit test app from a package
        /// </summary>
        public static bool InstallTestAppFromPackageIfNeeded(string deploymentDir, string packageName, string packageFamilyName, string appInstallerName, string appProcessName)
        {
            if (!TestAppxInstalled.Contains(packageFamilyName))
            {
                FileInfo FirstFileWithExtension(params string[] extensions)
                {
                    Log.Comment("Searching for Package file. Base dir: {0}", deploymentDir);
                    FileInfo fileInfo = null;
                    foreach (var ext in extensions)
                    {
                        fileInfo = new FileInfo(Path.Combine(deploymentDir, $"{appInstallerName}.{ext}"));
                        if (fileInfo.Exists)
                        {
                            Log.Comment("File '{0}' found!", fileInfo.FullName);
                            break;
                        }
                        else
                        {
                            Log.Comment("File '{0}' not found.", fileInfo.FullName);
                        }
                    }

                    return fileInfo;
                }

                var packageFile = FirstFileWithExtension("appx", "appxbundle", "msix", "msixbundle");

                if (packageFile?.Exists == true)
                {
                    PackageManager packageManager = new PackageManager();
                    DeploymentResult result = null;

                    var installedPackages = packageManager.FindPackagesForUser(string.Empty, packageFamilyName);
                    bool installationNeeded = true;

                    foreach (var installedPackage in installedPackages)
                    {
                        // For MUXControlsTestApp we have the loose test binaries to compare against the binaries of the installed app.
                        // This lets us tell if we can skip the install step.
                        if (installedPackage.Id.Name == TestApplicationInfo.MUXControlsTestApp.TestAppPackageName)
                        {
                            string installDirectory = installedPackage.InstalledLocation.Path;
                            string hashCheckDirectory = Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location), "HashCheck");

                            string uwpEntryPointPath = Path.Combine(installDirectory, "entrypoint", appProcessName);
                            string dotNetEntryPointPath = Path.Combine(installDirectory, Path.ChangeExtension(appProcessName, ".dll"));
                            string entryPointPath = File.Exists(uwpEntryPointPath) ? uwpEntryPointPath : dotNetEntryPointPath;

                            bool installedEntrypointIsSame = InstalledFileIsSameAsDeployed(entryPointPath, Path.Combine(hashCheckDirectory, Path.GetFileName(entryPointPath)));
                            bool installedMUXIsSame = InstalledFileIsSameAsDeployed(Path.Combine(installDirectory, "Microsoft.UI.Xaml.dll"), Path.Combine(hashCheckDirectory, "Microsoft.UI.Xaml.dll"));
                            bool installedMUXPIsSame = InstalledFileIsSameAsDeployed(Path.Combine(installDirectory, "Microsoft.UI.Xaml.Phone.dll"), Path.Combine(hashCheckDirectory, "Microsoft.UI.Xaml.Phone.dll"));
                            bool installedMUXCIsSame = InstalledFileIsSameAsDeployed(Path.Combine(installDirectory, "Microsoft.UI.Xaml.Controls.dll"), Path.Combine(hashCheckDirectory, "Microsoft.UI.Xaml.Controls.dll"));
                            bool installedMUX19h1ResourcesIsSame = InstalledFileIsSameAsDeployed(Path.Combine(installDirectory, "Microsoft.UI.Xaml.Resources.19h1.dll"), Path.Combine(hashCheckDirectory, "Microsoft.UI.Xaml.Resources.19h1.dll"));
                            bool installedMUXCommonResourcesIsSame = InstalledFileIsSameAsDeployed(Path.Combine(installDirectory, "Microsoft.UI.Xaml.Resources.Common.dll"), Path.Combine(hashCheckDirectory, "Microsoft.UI.Xaml.Resources.Common.dll"));

                            installationNeeded =
                                !installedEntrypointIsSame ||
                                !installedMUXIsSame ||
                                !installedMUXPIsSame ||
                                !installedMUXCIsSame ||
                                !installedMUX19h1ResourcesIsSame ||
                                !installedMUXCommonResourcesIsSame;

                            if(installationNeeded)
                            {
                                List<string> changedFiles = new List<string>();

                                if (!installedEntrypointIsSame) changedFiles.Add(Path.GetFileName(entryPointPath));
                                if (!installedMUXIsSame) changedFiles.Add("Microsoft.UI.Xaml.dll");
                                if (!installedMUXPIsSame) changedFiles.Add("Microsoft.UI.Xaml.Phone.dll");
                                if (!installedMUXCIsSame) changedFiles.Add("Microsoft.UI.Xaml.Controls.dll");
                                if (!installedMUX19h1ResourcesIsSame) changedFiles.Add("Microsoft.UI.Xaml.Resources.19h1.dll");
                                if (!installedMUXCommonResourcesIsSame) changedFiles.Add("Microsoft.UI.Xaml.Resources.Common.dll");

                                Log.Comment("Test AppX package already installed, but the following files have changed:" + Environment.NewLine);

                                foreach (string changedFile in changedFiles)
                                {
                                    Log.Comment(changedFile);
                                }
                            }

                        }
                        // For app packages used within the MUX Controls test suite we will assume that if they are already installed
                        // then they are valid.  This is considered safe because runtests.ps1 and/or TestPass-OneTimeMachineSetup.ps1
                        // will remove them before test passes run.  That will get rid of stale versions and we will never reach here.
                        // Thus if this code path is hit then they were just installed and are fine.
                        //
                        // Skipping the uninstall/reinstall for each test method speeds up the test pass by 2-3x.
                        //
                        // Note - Make sure that this list stays in sync with test\scripts\runtests.ps1 and
                        //        Helix\scripts\TestPass-OneTimeMachineSetup.ps1
                        else if ((installedPackage.Id.Name == "WinUICppDesktopSampleApp") ||
                            (installedPackage.Id.Name == "WinUICsDesktopSampleApp") ||
#if DEBUG
                            (installedPackage.Id.Name == "Microsoft.WinUI3ControlsGallery.Debug"))
#else
                            (installedPackage.Id.Name == "Microsoft.WinUI3ControlsGallery"))
#endif
                        {
                            installationNeeded = false;
                        }
                        // For all other app packages we cannot tell whether they are stale or valid so we assume they are stale and
                        // reinstall them.  This test helper is used by tests outside the MUX Controls code base so it is not safe
                        // to assume that runtests.ps1 or TestPass-OneTimeMachineSetup.ps1 ran.
                        else
                        {
                            installationNeeded = true;
                        }

                        if (!installationNeeded)
                        {
                            Log.Comment("Test AppX package already installed and is up to date. No installation needed.");
                        }
                        else
                        {
                            Log.Comment(Environment.NewLine + "Removing existing package by name: {0}", installedPackage.Id.FullName);

                            AutoResetEvent removePackageCompleteEvent = new AutoResetEvent(false);
                            var removePackageOperation = packageManager.RemovePackageAsync(installedPackage.Id.FullName);
                            removePackageOperation.Completed = (operation, status) =>
                            {
                                if (status != AsyncStatus.Started)
                                {
                                    result = operation.GetResults();
                                    removePackageCompleteEvent.Set();
                                }
                            };
                            removePackageCompleteEvent.WaitOne();

                            if (!string.IsNullOrEmpty(result.ErrorText))
                            {
                                Log.Error("Removal failed!");
                                Log.Error("Package removal ActivityId = {0}", result.ActivityId);
                                Log.Error("Package removal ErrorText = {0}", result.ErrorText);
                                Log.Error("Package removal ExtendedErrorCode = {0}", result.ExtendedErrorCode);
                            }
                            else
                            {
                                Log.Comment("Removal successful.");
                            }
                        }
                    }

                    if (installationNeeded)
                    {
                        // The test app has not been installed yet. Install it so tests can pass
                        List<Uri> depsPackages = new List<Uri>();
                        FileInfo dependenciesTextFile = new FileInfo(Path.Combine(deploymentDir, packageName + ".dependencies.txt"));
                        if (dependenciesTextFile.Exists)
                        {
                            Log.Comment("Including dependencies from {0}", dependenciesTextFile.FullName);
                            foreach (string line in File.ReadAllLines(dependenciesTextFile.FullName))
                            {
                                var dependencyPackageUri = new Uri(Path.Combine(deploymentDir, line));

                                Log.Comment("Adding dependency package: {0}", dependencyPackageUri.AbsolutePath);
                                depsPackages.Add(dependencyPackageUri);
                            }
                        }

                    var packageUri = new Uri(Path.Combine(deploymentDir, packageFile.FullName));

                        Log.Comment("Installing Test Appx Package: {0}", packageUri.AbsolutePath);

                        AutoResetEvent addPackageCompleteEvent = new AutoResetEvent(false);
                        var addPackageOperation = packageManager.AddPackageAsync(packageUri, depsPackages, DeploymentOptions.ForceApplicationShutdown);
                        addPackageOperation.Completed = (operation, status) =>
                        {
                            if (status != AsyncStatus.Started)
                            {
                                result = operation.GetResults();
                                addPackageCompleteEvent.Set();
                            }
                        };
                        addPackageCompleteEvent.WaitOne();

                        if (!string.IsNullOrEmpty(result.ErrorText))
                        {
                            Log.Error("Installation failed!");
                            Log.Error("Package installation ActivityId = {0}", result.ActivityId);
                            Log.Error("Package installation ErrorText = {0}", result.ErrorText);
                            Log.Error("Package installation ExtendedErrorCode = {0}", result.ExtendedErrorCode);
                            throw new Exception("Failed to install Test Appx Package: " + result.ErrorText);
                        }
                        else
                        {
                            Log.Comment("Installation successful.");
                        }
                    }
                }
                else
                {
                    Log.Error("Test Appx Package was not found in {0}.", deploymentDir);
                    return false;
                }

                TestAppxInstalled.Add(packageFamilyName);
            }
            
            return true;
        }

        /// <summary>
        /// Installs the unit test app from a package
        /// </summary>
        public static bool InstallTestAppFromDirectoryIfNeeded(string testAppDirectory, string packageFamilyName)
        {
            if (!TestAppxInstalled.Contains(packageFamilyName))
            {
                var installAppScript = Path.Combine(testAppDirectory, "InstallAppFromLayout.ps1");

                if (File.Exists(installAppScript))
                {
                    // To install from a directory, we have a script that registers the AppX manifest placed in the AppX layout directory.
                    ProcessStartInfo powershellStartInfo = new(
                        Path.Combine(Environment.GetEnvironmentVariable("SystemRoot"), "system32", "windowspowershell", "v1.0", "powershell.exe"),
                        "-NoLogo -NonInteractive -ExecutionPolicy Unrestricted -File \"" + installAppScript + "\"");

                    powershellStartInfo.UseShellExecute = false;
                    powershellStartInfo.RedirectStandardOutput = true;

                    Log.Comment("Installing test app...");

                    Process powershellProcess = Process.Start(powershellStartInfo);
                    powershellProcess.WaitForExit();

                    if (powershellProcess.ExitCode != 0)
                    {
                        Log.Error("Failed to install test app: {0}", powershellProcess.StandardOutput.ReadToEnd());
                    }

                    TestAppxInstalled.Add(packageFamilyName);
                }
                else
                {
                    Log.Error("Install app script was not found at {0}.", installAppScript);
                    return false;
                }
            }

            return true;
        }

        public static void InstallCert(string certFilePath)
        {
            Log.Comment("Installing cert: {0}", certFilePath);
            FileInfo certFile = new FileInfo(certFilePath);
            if (certFile.Exists)
            {
                X509Certificate2 certificate = new X509Certificate2(certFile.FullName);
                X509Store store = new X509Store(StoreName.Root, StoreLocation.LocalMachine);
                store.Open(OpenFlags.MaxAllowed);
                store.Add(certificate);
                Log.Comment("Test Appx Cert installed successfully.");
            }
            else
            {
                Log.Comment("The cert file '{0}' was not found", certFilePath);
            }
        }

        /// <summary>
        /// Installs the cert file for the appx so that it can be installed on the desktop environment
        /// </summary>
        public static void InstallAppxCert(string deploymentDir, string certFileName)
        {
            InstallCert(Path.Combine(deploymentDir, certFileName));
        }

        public static void EnableSideloadingApps()
        {
            RegistryKey rk = Registry.LocalMachine.OpenSubKey(@"Software\Policies\Microsoft\Windows", true);
            RegistryKey subkey = rk.OpenSubKey("AppX", true) ?? rk.CreateSubKey("AppX");
            subkey.SetValue("AllowAllTrustedApps", 1, RegistryValueKind.DWord);
            subkey.Flush();
            subkey.Dispose();
            Log.Comment("Sideloading is enabled");
        }

        private static bool InstalledFileIsSameAsDeployed(string installedFilePath, string deployedFilePath)
        {
            using (var sha256 = SHA256.Create())
            {
                string installedHash = string.Empty;
                string deployedHash = string.Empty;

                using (var stream = File.OpenRead(installedFilePath))
                {
                    installedHash = BitConverter.ToString(sha256.ComputeHash(stream)).Replace("-", string.Empty).ToLowerInvariant();
                }

                using (var stream = File.OpenRead(deployedFilePath))
                {
                    deployedHash = BitConverter.ToString(sha256.ComputeHash(stream)).Replace("-", string.Empty).ToLowerInvariant();
                }

                return installedHash == deployedHash;
            }
        }
    }
}
