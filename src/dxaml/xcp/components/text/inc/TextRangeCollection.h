// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <vector>
#include <minxcptypes.h>

#include <DOCollection.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

// This implemention was based heavily off of CPointCollection
class CTextRangeCollection final : public CCollection
{
public:
    DECLARE_CREATE(CTextRangeCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextRangeCollection>::Index;
    }

    _Check_return_ bool NeedsOwnerInfo() override { return true; }

    // CCollection overrides

    _Check_return_ HRESULT Append(_In_ CValue& value, _Out_opt_ uint32_t* indexOut = nullptr) override;
    _Check_return_ HRESULT Insert(_In_ uint32_t index, _In_ CValue& value) override;
    _Check_return_ void* RemoveAt(_In_ uint32_t index) override;
    _Check_return_ void* GetItemWithAddRef(_In_ uint32_t index) override;
    _Check_return_ HRESULT IndexOf(_In_ CValue& value, _Out_ int32_t* indexOut) override;
    _Check_return_ HRESULT MoveInternal(_In_ int32_t index, _In_ int32_t nPosition) override
    {
        return E_NOTIMPL;
    }

    uint32_t GetCount() const override { return static_cast<uint32_t>(m_items.size()); }
    _Check_return_ HRESULT Neat(bool) override
    {
        m_items.clear();
        return S_OK;
    }

    const std::vector<TextRangeData>& GetCollection() const { return m_items; }


protected:
    // CCollection overrides
    _Check_return_ HRESULT OnAddToCollection(_In_ const CValue& value) override;
    _Check_return_ HRESULT OnRemoveFromCollection(_In_ const CValue& value, int32_t previousIndex) override;
    _Check_return_ HRESULT OnClear() override;

private:
    explicit CTextRangeCollection(_In_ CCoreServices* core)
        : CCollection(core)
    {}

    static _Check_return_ HRESULT EnsureValue(_In_ const CValue& originalValue, _Outref_ CValue& coercedValue);
    void OnCollectionChanged();

    std::vector<TextRangeData> m_items;
};