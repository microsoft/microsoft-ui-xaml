// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//     GridLength, for specifying the row height and column width of a Grid
//      Typeconverter for RowDefinition.Height and ColumnDefinition.Width properties

#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CGridLength final : public CDependencyObject
{

private:

    CGridLength(_In_ CCoreServices *pCore);

    _Check_return_ HRESULT FromString(_In_ const xstring_ptr_view& inString);

    _Check_return_ HRESULT FromValueGridLength(_In_ CREATEPARAMETERS *pCreate);

public:

// Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );
// Validation method
    static _Check_return_ HRESULT Validate(
        const XGRIDLENGTH& gridLength
        );

    bool DoesAllowMultipleAssociation() const override { return true; }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGridLength>::Index;
    }

    XGRIDLENGTH m_gridLength;
};
