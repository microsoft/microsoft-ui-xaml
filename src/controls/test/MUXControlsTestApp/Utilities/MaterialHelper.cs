// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Private.Media;

namespace MUXControlsTestApp.Utilities
{
    public class MaterialSetupHelper : IDisposable
    {
        public MaterialSetupHelper(bool ignoreAreEffectsFast = true, bool simulateDisabledByPolicy = false)
        {
            MaterialHelperTestApi.IgnoreAreEffectsFast = ignoreAreEffectsFast;
            MaterialHelperTestApi.SimulateDisabledByPolicy = simulateDisabledByPolicy;
        }

        public void Dispose()
        {
            // Unset all override flags to avoid impacting subsequent tests
            MaterialHelperTestApi.IgnoreAreEffectsFast = false;
            MaterialHelperTestApi.SimulateDisabledByPolicy = false;
        }
    }

}
