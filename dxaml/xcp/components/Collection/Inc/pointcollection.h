// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <collectionbase.h>
#include <point.h>

class CPointCollection final : public CCollection
{
private:
    CPointCollection(_In_ CCoreServices *pCore)
        : CCollection(pCore)
    {}

public:
    ~CPointCollection() override
    {
        //we are not allowed to call directly abstract members
        VERIFYHR(Destroy());
    }

// Creation method

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override;

// CCollection overrides

    _Check_return_ HRESULT Append(CValue& value, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, CValue& value) override;
    _Check_return_ void *RemoveAt(_In_ XUINT32 nIndex) override;
    _Check_return_ void *GetItemWithAddRef(_In_ XUINT32 nIndex) override;
    _Check_return_ HRESULT IndexOf(CValue& value, _Out_ XINT32 *pIndex) override;
    _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) override
    {
        return E_NOTIMPL;
    }

    XUINT32 GetCount() const override { return m_items.size(); }
    _Check_return_ HRESULT Neat(_In_ bool bBreak = false) override { m_items.clear(); RRETURN(S_OK); }
    const xvector<XPOINTF>& GetCollection() { return m_items; }

protected:

// CCollection overrides
    _Check_return_ HRESULT OnAddToCollection(_In_ const CValue& value) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ const CValue& value, _In_ XINT32 iPreviousIndex) override;
    _Check_return_ HRESULT OnClear() override;

    xvector<XPOINTF> m_items;

// CPointCollection methods

public:
    _Check_return_ HRESULT InitFromArray(
        _In_ XUINT32 cPoints,
        _In_reads_(cPoints) XPOINTF *pPoints);

private:
    static _Check_return_ HRESULT EnsureValue(_In_ const CValue& vOriginal, _Out_ CValue& vCoerced);
    void OnPointsChanged();
};
