// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AccessKeysEvents.h"
#include "Mocks.h"


using namespace ::Windows::UI::Xaml::Tests::AccessKeys;
template <>
inline bool AccessKeys::AKOwnerEvents::InvokeEvent<MockCDO> (_In_ MockCDO* const pElement)
{    
    return pElement->RaiseAccessKeyInvoked();
} 


