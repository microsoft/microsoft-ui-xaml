// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.ApplicationModel;

namespace Private.Infrastructure.Hosting
{
    internal static class SystemCompositionEngineTestSupport
    {
        private const string FeatureId = "com.microsoft.windows.composition.engine";
        private const string Attestation =
            "8wekyb3d8bbwe has registered their use of com.microsoft.windows.composition.engine " +
            "with Microsoft and agrees to the terms of use.";

        private static readonly object SyncRoot = new object();
        private static bool systemCompositionEngineConfigured;
        private static bool systemCompositorVerified;
        private static bool xamlSystemCompositorVerified;

        internal static bool IsConfigured
        {
            get
            {
                lock (SyncRoot)
                {
                    return systemCompositionEngineConfigured;
                }
            }
        }

        internal static void Configure(string lafToken)
        {
            lock (SyncRoot)
            {
                if (systemCompositionEngineConfigured)
                {
                    return;
                }

                if (string.IsNullOrEmpty(lafToken))
                {
                    throw new InvalidOperationException(
                        "System composition engine tests require a non-empty LAF token.");
                }

                LogDiagnostic("managed host is requesting LAF authorization.");
                var unlockResult = LimitedAccessFeatures.TryUnlockFeature(
                    FeatureId,
                    lafToken,
                    Attestation);
                LogDiagnostic(
                    $"managed LAF authorization returned status={unlockResult.Status}.");
                if (unlockResult.Status != LimitedAccessFeatureStatus.Available)
                {
                    throw new InvalidOperationException(
                        "CompositionEngine LAF authorization failed " +
                        $"(status={unlockResult.Status}).");
                }

                LogDiagnostic("managed host is selecting the system composition engine.");
                if (!Microsoft.UI.Composition.CompositionEngine.TrySetProcessEngine(
                    Microsoft.UI.Composition.CompositionEngineType.System))
                {
                    throw new InvalidOperationException(
                        "TrySetProcessEngine(System) did not engage in the Win32 XAML host process.");
                }

                systemCompositionEngineConfigured = true;
                LogDiagnostic("managed host selected the system composition engine.");
            }
        }

        internal static void Verify()
        {
            lock (SyncRoot)
            {
                if (systemCompositorVerified)
                {
                    return;
                }

                if (!systemCompositionEngineConfigured)
                {
                    throw new InvalidOperationException(
                        "The system composition engine must be configured before it can be verified.");
                }

                LogDiagnostic("managed host is verifying the system compositor.");
                using (var compositor = new Microsoft.UI.Composition.Compositor())
                {
                    Verify(compositor);
                }
                systemCompositorVerified = true;
                LogDiagnostic("managed host verified the system compositor.");
            }
        }

        internal static void VerifyXaml()
        {
            lock (SyncRoot)
            {
                if (xamlSystemCompositorVerified)
                {
                    return;
                }

                if (!systemCompositionEngineConfigured)
                {
                    throw new InvalidOperationException(
                        "The system composition engine must be configured before it can be verified.");
                }

                LogDiagnostic("managed XAML host is verifying the system compositor.");
                var xamlElement = new Microsoft.UI.Xaml.Controls.Grid();
                Verify(
                    Microsoft.UI.Xaml.Hosting.ElementCompositionPreview
                        .GetElementVisual(xamlElement)
                        .Compositor);
                systemCompositorVerified = true;
                xamlSystemCompositorVerified = true;
                LogDiagnostic("managed XAML host verified the system compositor.");
            }
        }

        private static void Verify(Microsoft.UI.Composition.Compositor compositor)
        {
            var systemCompositor =
                Microsoft.UI.Composition.CompositionEngine.GetForSystemEngine(
                    compositor);
            if (!(systemCompositor is Windows.UI.Composition.Compositor))
            {
                throw new InvalidOperationException(
                    "GetForSystemEngine did not return a Windows.UI.Composition.Compositor.");
            }
        }

        private static void LogDiagnostic(string message)
        {
            var diagnostic = "CompositionEngineType.System: " + message;
            Console.WriteLine(diagnostic);
            System.Diagnostics.Trace.WriteLine(diagnostic);
        }
    }
}
