// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using System.Threading.Tasks;

#if USING_TAEF
using WEX.Logging.Interop;
using WEX.TestExecution;
#else
using Common;
#endif
using Windows.Foundation;
using Windows.Management.Deployment;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra
{
    using System = System;

    public class TestAppInstallHelper
    {
        private static HashSet<string> TestAppxInstalled = new HashSet<string>();

        /// <summary>
        /// Installs the unit test app from a package
        /// </summary>
        public static void InstallTestAppFromPackageIfNeeded(string deploymentDir, string packageName, string packageFamilyName, string appInstallerName)
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

                var packageFile = FirstFileWithExtension("appx", "appxbundle", "msix");

                if (packageFile?.Exists == true)
                {
                    PackageManager packageManager = new PackageManager();
                    DeploymentResult result = null;
                    
                    var installedPackages = packageManager.FindPackagesForUser(string.Empty, packageFamilyName);
                    foreach (var installedPackage in installedPackages)
                    {
                        Log.Comment("Test AppX package already installed. Removing existing package by name: {0}", installedPackage.Id.FullName);

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
                else
                {
                    Log.Comment("Test Appx Package was not found in {0}.", deploymentDir);
                }

                TestAppxInstalled.Add(packageFamilyName);
            }
        }

        /// <summary>
        /// Installs the unit test app from a package
        /// </summary>
        public static void InstallTestAppFromDirectoryIfNeeded(string testAppDirectory, string packageFamilyName)
        {
            if (!TestAppxInstalled.Contains(packageFamilyName))
            {
                var createAppxDirectoryScript = Path.Combine(testAppDirectory, "CreateAppxDirectory.ps1");

                if (File.Exists(createAppxDirectoryScript))
                {
                    // To install from a directory, we have a script that does two things: first, it runs an MSBuild script
                    // that generates the AppX directory, and then second, it registers the AppX manifest placed in that directory.
                    ProcessStartInfo powershellStartInfo = new ProcessStartInfo(
                        Path.Combine(Environment.GetEnvironmentVariable("SystemRoot"), @"system32\windowspowershell\v1.0\powershell.exe"),
                        "-NoLogo -NonInteractive -ExecutionPolicy Unrestricted -File \"" + createAppxDirectoryScript + "\"");

                    powershellStartInfo.UseShellExecute = false;
                    powershellStartInfo.RedirectStandardOutput = true;

                    Log.Comment("Registering test app...");

                    Process powershellProcess = Process.Start(powershellStartInfo);
                    powershellProcess.WaitForExit();

                    if (powershellProcess.ExitCode != 0)
                    {
                        Log.Error("Failed to register test app: {0}", powershellProcess.StandardOutput.ReadToEnd());
                    }

                    TestAppxInstalled.Add(packageFamilyName);
                }
                else
                {
                    Log.Comment("Create AppX directory script was not found at {0}.", createAppxDirectoryScript);
                }
            }
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
            RegistryKey registryKeyReadOnly = Registry.LocalMachine.OpenSubKey(@"Software\Policies\Microsoft\Windows", writable: false);
            RegistryKey subkeyReadOnly = registryKeyReadOnly.OpenSubKey("AppX", writable: false);

            if (subkeyReadOnly == null || (int)subkeyReadOnly.GetValue("AllowAllTrustedApps") != 1)
            {
                Log.Comment("Enabling sideloading...");
                RegistryKey registryKey = Registry.LocalMachine.OpenSubKey(@"Software\Policies\Microsoft\Windows", writable: true);
                RegistryKey subkey = registryKey.OpenSubKey("AppX", writable: true) ?? registryKey.CreateSubKey("AppX");
                subkey.SetValue("AllowAllTrustedApps", 1, RegistryValueKind.DWord);
                subkey.Flush();
                subkey.Dispose();
            }

            Log.Comment("Sideloading is enabled.");
        }
    }
}
