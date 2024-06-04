// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <collectionbase.h>
#include <double.h>

class CDoubleCollection
    : public CCollection
{
protected:
    _Check_return_ HRESULT InitFromString(
        _In_ UINT32 cString,
        _In_reads_(cString) const WCHAR *pString
        );

public:
    ~CDoubleCollection() override
    {
        VERIFYHR(Destroy());
    }

    CDoubleCollection(_In_ CCoreServices *pCore)
        : CCollection(pCore)
    {}

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT Append(CValue& value, _Out_opt_ UINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ UINT32 nIndex, CValue& value) override;
    _Check_return_ void *RemoveAt(_In_ UINT32 nIndex) override;
    _Check_return_ void *GetItemWithAddRef(_In_ UINT32 nIndex) override;
    _Check_return_ HRESULT IndexOf(CValue& value, _Out_ INT32 *pIndex) override;
    _Check_return_ HRESULT MoveInternal(_In_ INT32 nIndex, _In_ INT32 nPosition) override
    {
        return E_NOTIMPL;
    }

    UINT32 GetCount() const override { return static_cast<UINT32>(m_items.size()); }
    _Check_return_ HRESULT Neat(_In_ bool bBreak = false) override { m_items.clear(); RRETURN(S_OK); }
    const std::vector<float>& GetCollection() const { return m_items; }

protected:
    _Check_return_ HRESULT OnAddToCollection(_In_ const CValue& value) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ const CValue& value, _In_ INT32 iPreviousIndex) override;
    _Check_return_ HRESULT OnClear() override;

    std::vector<float> m_items;

private:
    static _Check_return_ HRESULT EnsureValue(_In_ const CValue& vOriginal, _Out_ CValue& vCoerced);
    void SetChangedFlags();
};

// Only exists for compat reasons... Not actively used by anything.
class CFloatCollection final
    : public CDoubleCollection
{
private:
    CFloatCollection(_In_ CCoreServices *pCore)
        : CDoubleCollection(pCore)
    {}

public:
    DECLARE_CREATE(CFloatCollection);
};
