// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ValidationErrorsCollection.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ValidationErrorsCollection)
    {
    public:
        // Override these methods so that we can raise vector changed events.
        IFACEMETHOD(SetAt)(_In_ uint32_t index, _In_ xaml_controls::IInputValidationError* item) final;
        IFACEMETHOD(InsertAt)(_In_ uint32_t index, _In_ xaml_controls::IInputValidationError* item) final;
        IFACEMETHOD(RemoveAt)(_In_ uint32_t index) final;
        IFACEMETHOD(Append)(_In_ xaml_controls::IInputValidationError* item) final;
        IFACEMETHOD(RemoveAtEnd)() final;
        IFACEMETHOD(Clear)() final;

    private:
        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange change, _In_ uint32_t changeIndex);
    };
}
