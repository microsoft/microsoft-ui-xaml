// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Dispatching;
using Microsoft.UI.Xaml.Hosting;
using Microsoft.UI.Xaml.Tests.Common;
using Private.Infrastructure.Hosting.WPF;
using System;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using Windows.Storage;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using static Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF.WindowPlacementPersistenceTests;

namespace Microsoft.UI.Xaml.Tests.Hosting.Win32.WPF
{
    // These tests run against the real packaged ApplicationData store. Nothing here substitutes a
    // fake backend: every placement under test is written by the product through a real window.
    [TestClass]
    public class WindowPlacementStorageIsolationTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("Hosting:Mode", "WPF")]
        [TestProperty("Classification", "Integration")]
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
        public void RecordWrittenUnderAnotherApplicationIdentityIsNotVisible()
        {
            UIExecutor.Execute(() =>
            {
                var settings = ApplicationData.Current.LocalSettings;
                var applicationId = GetApplicationId();
                var foreignApplicationId = applicationId + ".Foreign";
                var ownContainerName = ContainerName(applicationId);
                var foreignContainerName = ContainerName(foreignApplicationId);
                Verify.AreNotEqual(ownContainerName, foreignContainerName);

                var placementId = "WindowPlacementStorageIsolationTests." + Guid.NewGuid().ToString("N");
                var valueName = ValueName(placementId);
                Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));

                try
                {
                    // The product writes the record. The test never authors placement bytes.
                    var savedPlacement = SaveThroughClosedWindow(placementId, CreatePlacement(210, 160, 620, 460));

                    Verify.IsTrue(settings.Containers.TryGetValue(ownContainerName, out var ownContainer));
                    Verify.IsTrue(ownContainer.Values.TryGetValue(valueName, out var storedRecord));
                    var record = (string)storedRecord;
                    Verify.IsFalse(string.IsNullOrEmpty(record));

                    // Same placement id, same record bytes, different application identity.
                    var foreignContainer = settings.CreateContainer(
                        foreignContainerName,
                        ApplicationDataCreateDisposition.Always);
                    foreignContainer.Values[valueName] = record;

                    // The record belonging to this application is still readable, so a store that
                    // simply fails every load cannot pass the assertion below.
                    var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                    Verify.IsNotNull(loadedPlacement);
                    VerifyPlacementsMatch(savedPlacement, loadedPlacement);

                    // Remove only this application's copy. The identical foreign copy stays in
                    // LocalSettings and must remain invisible.
                    ownContainer.Values.Remove(valueName);
                    Verify.IsTrue(foreignContainer.Values.ContainsKey(valueName));
                    Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));
                }
                finally
                {
                    if (settings.Containers.TryGetValue(ownContainerName, out var ownContainer))
                    {
                        ownContainer.Values.Remove(valueName);
                    }

                    if (settings.Containers.ContainsKey(foreignContainerName))
                    {
                        settings.DeleteContainer(foreignContainerName);
                    }
                    Verify.IsFalse(settings.Containers.ContainsKey(foreignContainerName));                    Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));
                }
            });
        }

        [TestMethod]
        public void ConcurrentSavesForOneIdLeaveOneCoherentRecord()
        {
            var settings = ApplicationData.Current.LocalSettings;
            var applicationId = GetApplicationId();
            var containerName = ContainerName(applicationId);
            var placementId = "WindowPlacementStorageIsolationTests." + Guid.NewGuid().ToString("N");
            var valueName = ValueName(placementId);

            UIExecutor.Execute(() => Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId)));

            var first = CreatePlacement(120, 90, 500, 400);
            var second = CreatePlacement(340, 250, 700, 520);

            try
            {
                // Both windows stay open until the barrier releases, so the two saves overlap.
                using (var barrier = new Barrier(2))
                {
                    var firstSaver = new PlacementSaver(placementId, first, barrier);
                    var secondSaver = new PlacementSaver(placementId, second, barrier);

                    firstSaver.Start();
                    secondSaver.Start();
                    firstSaver.Join();
                    secondSaver.Join();

                    firstSaver.ThrowIfFailed();
                    secondSaver.ThrowIfFailed();

                    UIExecutor.Execute(() =>
                    {
                        var loadedPlacement = WindowPlacement.LoadForPersistPlacementId(placementId);
                        Verify.IsNotNull(loadedPlacement);

                        // Exactly one of the two snapshots, never a blend of the two.
                        var matchesFirst = PlacementsMatch(firstSaver.Saved, loadedPlacement);
                        var matchesSecond = PlacementsMatch(secondSaver.Saved, loadedPlacement);
                        Log.Comment(string.Format(
                            "Loaded placement {0},{1} {2}x{3} dpi {4}",
                            loadedPlacement.NormalRect.X,
                            loadedPlacement.NormalRect.Y,
                            loadedPlacement.NormalRect.Width,
                            loadedPlacement.NormalRect.Height,
                            loadedPlacement.Dpi));
                        Verify.IsTrue(matchesFirst || matchesSecond, "Loaded record matches one saved snapshot");

                        // A partially published snapshot would leave extra records for this id.
                        Verify.IsTrue(settings.Containers.TryGetValue(containerName, out var container));
                        var recordCount = 0;
                        foreach (var name in container.Values.Keys)
                        {
                            if (name == valueName)
                            {
                                recordCount++;
                            }
                        }
                        Verify.AreEqual(1, recordCount);
                    });
                }
            }
            finally
            {
                UIExecutor.Execute(() =>
                {
                    if (settings.Containers.TryGetValue(containerName, out var container))
                    {
                        container.Values.Remove(valueName);
                    }
                    Verify.IsNull(WindowPlacement.LoadForPersistPlacementId(placementId));
                });
            }
        }

        // Runs one enrolled window on its own UI thread and closes it when the barrier releases.
        private sealed class PlacementSaver
        {
            private readonly string m_placementId;
            private readonly WindowPlacement m_placement;
            private readonly Barrier m_barrier;
            private readonly Thread m_thread;
            private Exception m_failure;

            public PlacementSaver(string placementId, WindowPlacement placement, Barrier barrier)
            {
                m_placementId = placementId;
                m_placement = placement;
                m_barrier = barrier;
                m_thread = new Thread(Run);
                m_thread.SetApartmentState(ApartmentState.STA);
                m_thread.IsBackground = true;
            }

            public WindowPlacement Saved { get; private set; }

            public void Start() => m_thread.Start();

            public void Join() => Verify.IsTrue(m_thread.Join(TimeSpan.FromMinutes(2)), "Saver thread finished");

            public void ThrowIfFailed()
            {
                if (m_failure != null)
                {
                    throw new InvalidOperationException("Saver thread failed: " + m_failure, m_failure);
                }
            }

            private void Run()
            {
                try
                {
                    var controller = DispatcherQueueController.CreateOnCurrentThread();
                    using (WindowsXamlManager.InitializeForCurrentThread())
                    {
                        var window = CreateWindow();
                        try
                        {
                            window.PersistPlacementId = m_placementId;
                            window.UseAutomaticPlacementPersistence = true;
                            window.Show(new WindowShowOptions
                            {
                                Placement = m_placement,
                                CascadeBehavior = WindowCascadeBehavior.Disabled,
                                DoNotActivate = true
                            });

                            WindowPlacement saved;
                            Verify.IsTrue(window.TryGetPlacement(out saved));
                            Saved = saved;

                            // Release both threads together so the two saves race.
                            m_barrier.SignalAndWait();
                        }
                        finally
                        {
                            window.Close();
                        }

                        controller.DispatcherQueue.DoEvents();
                    }

                    controller.ShutdownQueue();
                }
                catch (Exception failure)
                {
                    m_failure = failure;
                    try
                    {
                        // Release the peer so it does not wait for a thread that already failed.
                        m_barrier.SignalAndWait(TimeSpan.Zero);
                    }
                    catch (Exception)
                    {
                    }
                }
            }
        }

        private static bool PlacementsMatch(WindowPlacement expected, WindowPlacement actual)
        {
            return expected != null &&
                actual != null &&
                expected.NormalRect.X == actual.NormalRect.X &&
                expected.NormalRect.Y == actual.NormalRect.Y &&
                expected.NormalRect.Width == actual.NormalRect.Width &&
                expected.NormalRect.Height == actual.NormalRect.Height &&
                expected.Dpi == actual.Dpi;
        }

        private static void VerifyPlacementsMatch(WindowPlacement expected, WindowPlacement actual)
        {
            Verify.AreEqual(expected.NormalRect.X, actual.NormalRect.X);
            Verify.AreEqual(expected.NormalRect.Y, actual.NormalRect.Y);
            Verify.AreEqual(expected.NormalRect.Width, actual.NormalRect.Width);
            Verify.AreEqual(expected.NormalRect.Height, actual.NormalRect.Height);
            Verify.AreEqual(expected.Dpi, actual.Dpi);
        }

        private static string ContainerName(string applicationId) => "app1_" + HashName(applicationId);

        private static string ValueName(string placementId) =>
            "wp1_" + placementId.Substring(0, 16) + "_" + HashName(placementId);

        // The public API has no delete operation and no way to name a foreign application's
        // container. Match the private UTF-16 SHA-256 / unpadded Base32 names for cleanup and for
        // planting the foreign copy, never to author the placement record under test.
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
