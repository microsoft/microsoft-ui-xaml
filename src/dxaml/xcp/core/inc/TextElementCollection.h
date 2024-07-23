// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "DOCollection.h"

class CLocalValueEntry;

//---------------------------------------------------------------------------
//
//  TextElementCollection
//
//---------------------------------------------------------------------------

class CTextElementCollection : public CDOCollection
{
protected:
    CTextElementCollection (_In_ CCoreServices *pCore);
    ~CTextElementCollection() override;

public:
    DECLARE_CREATE(CTextElementCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextElementCollection>::Index;
    }

    _Check_return_ CDependencyObject* RemoveAtImpl(
        _In_ XUINT32 nIndex
        ) final;

    //////////////////////////////////////////////
    // CDOCollection Overrides
    //////////////////////////////////////////////
    bool ShouldEnsureNameResolution() final { return true; }

    _Check_return_ HRESULT AppendImpl(
        _In_ CDependencyObject *pObject,
        _Out_ XUINT32 *pnIndex = NULL
        ) final;

    _Check_return_ HRESULT InsertImpl(
        _In_ XUINT32 nIndex,
        _In_ CDependencyObject *pObject
        ) final;

    _Check_return_ HRESULT OnClear() final;

    virtual _Check_return_ HRESULT MarkDirty(_In_opt_ const CDependencyProperty *pdp);

    //////////////////////////////////////////////
    // CDependencyObject Overrides
    //////////////////////////////////////////////

    //////////////////////////////////////////////
    // Validation virtual
    //////////////////////////////////////////////
    virtual _Check_return_ HRESULT ValidateTextElement(_In_ CTextElement *pTextElement);
};
