// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "enumdefs.g.h"
#include "Indexes.g.h"

namespace AccessKeys
{
    namespace AKOwnerEvents
    {   
        template<class CDObject>
        bool InvokeEvent(_In_ CDObject* pElement);

        template<class CDObject>
        void RaiseAccessKeyShown(_In_ CDObject* pElement, _In_ const wchar_t* const pressedKeys);
 
        template<class CDObject>
        void RaiseAccessKeyHidden(_In_ CDObject* pElement);
    }
}
