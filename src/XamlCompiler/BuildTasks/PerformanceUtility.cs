// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler.Tracing
{
    internal static class PerformanceUtility
    {
        static ILog logger = null;

        internal static void Initialize(ILog logger)
        {
            PerformanceUtility.logger = logger;
        }

        internal static void Shutdown()
        {
            logger = null;
        }

        internal static void FireCodeMarker(CodeMarkerEvent marker, string additionalInformation = null)
        {
            var now = DateTime.Now;
            var message = $"Xaml Compiler Marker: {now.ToString("T")}:{now.Millisecond,04} {marker.ToString()}";
            if (!string.IsNullOrEmpty(additionalInformation))
            {
                message += $", {additionalInformation}";
            }
            logger.LogDiagnosticMessage(message);
        }
    }
}
