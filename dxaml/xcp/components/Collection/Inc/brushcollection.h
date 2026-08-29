// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <collectionbase.h>
#include <point.h>

class CBrushCollection final
    : public CDOCollection
{
private:
    CBrushCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:
    ~CBrushCollection() override
    {
        //we are not allowed to call directly abstract members
        VERIFYHR(Destroy());
    }

    // Creation method

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBrushCollection>::Index;
    }

    // CCollection overrides...
public:
    _Check_return_ HRESULT Clear() override;
    _Check_return_ HRESULT MoveInternal(_In_ XINT32 nIndex, _In_ XINT32 nPosition) override {
        return E_NOTIMPL;
    }

    void SetChangeCallback(
        _In_ const std::weak_ptr<ICollectionChangeCallback>& callback) override
    {
        m_wrChangeCallback = callback;
    }

    // CDOCollectionOverrides ...
public:
    _Check_return_ HRESULT Append(_In_ CDependencyObject *pObject, _Out_opt_ XUINT32 *pnIndex = NULL) override;
    _Check_return_ HRESULT Insert(_In_ XUINT32 nIndex, _In_ CDependencyObject *pObject) override;
    _Check_return_ void * RemoveAt(_In_ XUINT32 nIndex) override;

// CBrushCollection methods

private:
    std::weak_ptr<ICollectionChangeCallback> m_wrChangeCallback;
};
