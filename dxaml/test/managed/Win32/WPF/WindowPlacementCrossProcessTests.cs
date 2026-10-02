// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure.Hosting.WPF;
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using Windows.Storage;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using static Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF.WindowPlacementPersistenceTests;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    // Run the whole class in declaration order using runtests -PreservePackageRegistration.
    // ExecutionGroup must not be used: it overrides method isolation.
    [TestClass]
    public class WindowPlacementCrossProcessTests : XamlTestsBase
    {
        private const string HandoffKey = "WindowPlacementCrossProcessTests.Handoff";

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("IsolationLevel", "Method")]
        [TestProperty("Ignore[not(@PreservePackageRegistration='true')]", "true")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void SaveInFirstProcess()
        {
            UIExecutor.Execute(() =>
            {
                var settings = ApplicationData.Current.LocalSettings;
                if (settings.Values.TryGetValue(HandoffKey, out var previous))
                {
                    Cleanup((ApplicationDataCompositeValue)previous);
                }

                var handoff = new ApplicationDataCompositeValue
                {
                    ["ApplicationId"] = GetApplicationId(),
                    ["ProcessId"] = Process.GetCurrentProcess().Id,
                    ["FirstId"] = Guid.NewGuid().ToString("N"),
                    ["SecondId"] = Guid.NewGuid().ToString("N")
                };
                var completed = false;
                try
                {
                    var firstId = (string)handoff["FirstId"];
                    var secondId = (string)handoff["SecondId"];
                    Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(firstId));
                    Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(secondId));

                    var first = SaveThroughClosedWindow(firstId, CreatePlacement(100, 100, 500, 400));
                    var second = SaveThroughClosedWindow(secondId, CreatePlacement(200, 150, 640, 480));
                    Verify.AreNotEqual(first.NormalRect.Width, second.NormalRect.Width);
                    Verify.AreNotEqual(first.NormalRect.Height, second.NormalRect.Height);
                    WriteExpected(handoff, "First", first);
                    WriteExpected(handoff, "Second", second);
                    VerifyExpected(handoff, "First", WindowPlacement.LoadForPersistPlacementId(firstId));
                    VerifyExpected(handoff, "Second", WindowPlacement.LoadForPersistPlacementId(secondId));
                    settings.Values[HandoffKey] = handoff;
                    Verify.IsTrue(settings.Values.ContainsKey(HandoffKey));
                    Log.Comment($"Saved both ids in process {handoff["ProcessId"]}, application {handoff["ApplicationId"]}.");
                    completed = true;
                }
                finally
                {
                    if (!completed)
                    {
                        Cleanup(handoff);
                    }
                }
            });
        }

        [TestMethod]
        public void RestoreInSecondProcess()
        {
            UIExecutor.Execute(() =>
            {
                var settings = ApplicationData.Current.LocalSettings;
                Log.Comment($"Reader process {Process.GetCurrentProcess().Id}, application {GetApplicationId()}.");
                Verify.IsTrue(settings.Values.TryGetValue(HandoffKey, out var value),
                    "Run the whole class with one package registration so SaveInFirstProcess publishes the expected captures.");
                var handoff = (ApplicationDataCompositeValue)value;
                try
                {
                    var processId = Process.GetCurrentProcess().Id;
                    Verify.AreNotEqual((int)handoff["ProcessId"], processId,
                        "The reader must run in a different process, not reload an in-memory cache.");
                    Verify.AreEqual((string)handoff["ApplicationId"], GetApplicationId());
                    Log.Comment($"Restoring in process {processId}; writer was {handoff["ProcessId"]}.");

                    var firstId = (string)handoff["FirstId"];
                    var secondId = (string)handoff["SecondId"];
                    var first = WindowPlacement.LoadForPersistPlacementId(firstId);
                    var second = WindowPlacement.LoadForPersistPlacementId(secondId);
                    VerifyExpected(handoff, "First", first);
                    VerifyExpected(handoff, "Second", second);
                    Verify.AreNotEqual(first.NormalRect.Width, second.NormalRect.Width);
                    Verify.AreNotEqual(first.NormalRect.Height, second.NormalRect.Height);

                    var window = CreateWindow();
                    try
                    {
                        window.PersistPlacementId = firstId;
                        window.UseAutomaticPlacementPersistence = true;
                        Verify.IsTrue(window.TryApplyInitialPlacement(new WindowShowOptions
                        {
                            CascadeBehavior = WindowCascadeBehavior.Disabled,
                            DoNotActivate = true
                        }));
                        Verify.IsFalse(window.Visible);
                        Verify.IsTrue(window.TryGetPlacement(out var restored));
                        VerifyExpected(handoff, "First", restored);

                        window.Show(new WindowShowOptions
                        {
                            SkipInitialPlacement = true,
                            DoNotActivate = true
                        });
                        Verify.IsTrue(window.Visible);
                        Verify.IsTrue(window.TryGetPlacement(out restored));
                        VerifyExpected(handoff, "First", restored);
                    }
                    finally
                    {
                        window.Close();
                    }
                }
                finally
                {
                    Cleanup(handoff);
                }
            });
        }

        private static void WriteExpected(ApplicationDataCompositeValue handoff, string prefix, WindowPlacement placement)
        {
            handoff[prefix + "X"] = placement.NormalRect.X;
            handoff[prefix + "Y"] = placement.NormalRect.Y;
            handoff[prefix + "Width"] = placement.NormalRect.Width;
            handoff[prefix + "Height"] = placement.NormalRect.Height;
            handoff[prefix + "Dpi"] = placement.Dpi;
            handoff[prefix + "State"] = (int)placement.State;
            Log.Comment($"{prefix}: ({placement.NormalRect.X}, {placement.NormalRect.Y}, " +
                $"{placement.NormalRect.Width}, {placement.NormalRect.Height}), dpi {placement.Dpi}, state {placement.State}; " +
                $"work area ({placement.WorkArea.X}, {placement.WorkArea.Y}, {placement.WorkArea.Width}, {placement.WorkArea.Height}).");
        }

        private static void VerifyExpected(ApplicationDataCompositeValue handoff, string prefix, WindowPlacement placement)
        {
            Verify.IsNotNull(placement);
            Log.Comment($"Captured {prefix}: ({placement.NormalRect.X}, {placement.NormalRect.Y}, " +
                $"{placement.NormalRect.Width}, {placement.NormalRect.Height}), dpi {placement.Dpi}, state {placement.State}; " +
                $"work area ({placement.WorkArea.X}, {placement.WorkArea.Y}, {placement.WorkArea.Width}, {placement.WorkArea.Height}).");
            Verify.AreEqual((int)handoff[prefix + "X"], placement.NormalRect.X);
            Verify.AreEqual((int)handoff[prefix + "Y"], placement.NormalRect.Y);
            Verify.AreEqual((int)handoff[prefix + "Width"], placement.NormalRect.Width);
            Verify.AreEqual((int)handoff[prefix + "Height"], placement.NormalRect.Height);
            Verify.AreEqual((int)handoff[prefix + "Dpi"], placement.Dpi);
            Verify.AreEqual((WindowPlacementState)(int)handoff[prefix + "State"], placement.State);
        }

        private static void Cleanup(ApplicationDataCompositeValue handoff)
        {
            var settings = ApplicationData.Current.LocalSettings;
            var containerName = "app1_" + HashName((string)handoff["ApplicationId"]);
            if (settings.Containers.TryGetValue(containerName, out var container))
            {
                foreach (var field in new[] { "FirstId", "SecondId" })
                {
                    var id = (string)handoff[field];
                    container.Values.Remove("wp1_" + id.Substring(0, 16) + "_" + HashName(id));
                }
            }
            settings.Values.Remove(HandoffKey);
            Verify.IsNull(WindowPlacement.LoadForPersistPlacementId((string)handoff["FirstId"]));
            Verify.IsNull(WindowPlacement.LoadForPersistPlacementId((string)handoff["SecondId"]));
            Verify.IsFalse(settings.Values.ContainsKey(HandoffKey));
        }

        // The public API has no delete operation. Match the private UTF-16 SHA-256 /
        // unpadded Base32 names only for cleanup, never to write the placement under test.
        private static string HashName(string text)
        {
            using var sha = SHA256.Create();
            var digest = sha.ComputeHash(Encoding.Unicode.GetBytes(text));
            const string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
            var result = new StringBuilder();
            uint accumulator = 0;
            var bits = 0;
            foreach (var value in digest)
            {
                accumulator = (accumulator << 8) | value;
                bits += 8;
                while (bits >= 5)
                {
                    bits -= 5;
                    result.Append(alphabet[(int)((accumulator >> bits) & 31)]);
                }
            }
            if (bits > 0)
            {
                result.Append(alphabet[(int)((accumulator << (5 - bits)) & 31)]);
            }
            return result.ToString();
        }

        private static string GetApplicationId()
        {
            uint length = 0;
            Verify.AreEqual(122, GetCurrentApplicationUserModelId(ref length, null));
            var result = new StringBuilder((int)length);
            Verify.AreEqual(0, GetCurrentApplicationUserModelId(ref length, result));
            return result.ToString();
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern int GetCurrentApplicationUserModelId(ref uint length, StringBuilder applicationId);
    }
}
