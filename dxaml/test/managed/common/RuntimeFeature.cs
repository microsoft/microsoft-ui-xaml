// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;
using Windows.ApplicationModel.Core;
using Microsoft.UI.Xaml.Markup;

using Private.Infrastructure;
using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.Common
{
    public sealed class RuntimeFeature : IDisposable
    {
        private readonly int featureId;
        private readonly bool enableDisable;
        private RuntimeFeature(int featureId, bool enableDisable)
        {
            this.featureId = featureId;
            this.enableDisable = enableDisable;
            bool wasPreviouslyEnabled = false;
            TestServices.Utilities.SetRuntimeEnabledFeatureOverride(
                this.featureId,
                this.enableDisable,
                out wasPreviouslyEnabled);
        }

        void IDisposable.Dispose()
        {
            bool wasPreviouslyEnabled = false;
            TestServices.Utilities.SetRuntimeEnabledFeatureOverride(
                this.featureId,
                !this.enableDisable,
                out wasPreviouslyEnabled);
        }

        public static IDisposable Disable(int featureId)
        {
            return new RuntimeFeature(featureId, false);
        }

        public static IDisposable Enable(int featureId)
        {
            return new RuntimeFeature(featureId, true);
        }
    }
}

