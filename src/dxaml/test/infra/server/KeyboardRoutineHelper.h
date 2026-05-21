// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <InputManager.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class KeyboardRoutineHelper
        {
        public:
            static void SendKeyInput(UINT16 keyCodeOrScanCode, BOOLEAN down, BOOLEAN isScanCode);
        };
    }
} } } }
