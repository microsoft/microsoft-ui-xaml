// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "docollection.h"

class CValidationErrorsCollection final : public CDOCollection
{
private:
    CValidationErrorsCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

// Creation method
public:
    DECLARE_CREATE(CValidationErrorsCollection);

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const final;

// CDOCollection overrides
    _Check_return_ HRESULT OnAddToCollection(_In_ CDependencyObject *obj) final;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ CDependencyObject *obj, _In_ int32_t prreviousIndex) final;
    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject *object) final;
    _Check_return_ HRESULT Neat(bool) final;

private:
    bool TryGetValidationError(_In_ CDependencyObject* obj, _COM_Outptr_ xaml_controls::IInputValidationError** error) const;
};
