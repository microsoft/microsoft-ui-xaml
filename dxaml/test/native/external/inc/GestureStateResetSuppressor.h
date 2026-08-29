// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <Closable.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

// NB: THIS SHOULD NOT BE USED IN NEW TESTS!
// Some tests relied on us not resetting the state of the gesture recognizer before injecting a tap,
// likely due to timing or something similarly.  This provides the ability to suppress that behavior
// in tests that have that reliance for passing.  Any new tests should avoid taking a dependency
// on this behavior.
class GestureStateResetSuppressor
{
public:
    GestureStateResetSuppressor()
    {
        auto suppressor = test_infra::TestServices::InputHelper->SuppressGestureStateReset();
        auto suppressorAsObj = dynamic_cast<Platform::Object^>(suppressor);
        wrl::ComPtr<IUnknown> unknown(reinterpret_cast<IUnknown*>(suppressorAsObj));
        LogThrow_IfFailed(unknown.As(&m_closable));
    }

    ~GestureStateResetSuppressor()
    {
        LogThrow_IfFailed(m_closable->Close());
    }

    // Disallow copying/moving
    GestureStateResetSuppressor(GestureStateResetSuppressor&& other) = delete;
    GestureStateResetSuppressor(const GestureStateResetSuppressor&) = delete;
    GestureStateResetSuppressor& operator=(const GestureStateResetSuppressor&) = delete;
    GestureStateResetSuppressor& operator=(GestureStateResetSuppressor&&) = delete;

private:
    wrl::ComPtr<ABI::Windows::Foundation::IClosable> m_closable;
};

} } } } }
