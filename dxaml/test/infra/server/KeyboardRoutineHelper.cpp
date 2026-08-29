// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "KeyboardRoutineHelper.h"
#include "UtilitiesRoutineHelper.h"

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        void KeyboardRoutineHelper::SendKeyInput(UINT16 keyCodeOrScanCode, BOOLEAN down, BOOLEAN isScanCode)
        {
            if (isScanCode)
            {
                LOG_OUTPUT(L"Calling IMInjectKeyInput for %ws with ScanCode: 0x%X", down ? L"KeyDown" : L"KeyUp", keyCodeOrScanCode);
            }
            else
            {
                LOG_OUTPUT(L"Calling IMInjectKeyInput for %ws with VK: 0x%X", down ? L"KeyDown" : L"KeyUp", keyCodeOrScanCode);
            }

            HRESULT hr = IMInjectKeyInput(keyCodeOrScanCode, down, isScanCode);
            if (FAILED(hr))
            {
                LOG_ERROR(L"IMInjectKeyInput failed with error 0x%X", ::GetLastError());
            }
            
            Throw::IfFailed(hr);
        }
    }
} } } }
