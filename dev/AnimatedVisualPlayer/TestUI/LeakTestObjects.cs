// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;

#if !BUILD_WINDOWS
using AnimatedVisualPlayer = Microsoft.UI.Xaml.Controls.AnimatedVisualPlayer;
#endif

namespace MUXControlsTestApp
{
    namespace AnimatedVisualPlayer_TestUI
    {
        public static class LeakTestObjects
        {
            public static WeakReference<AnimatedVisualPlayer> PlayerWeakRef { get; set; }
        }
    }
}