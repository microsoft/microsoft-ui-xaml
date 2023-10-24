// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcplist.h"
#include "xstringmap.h"
#include <pal.h>
#include "xmap.h"
#include "xref_ptr.h"
#include <vector_map.h>

class CDependencyObject;

//------------------------------------------------------------------------
//
//  Class:  CValueStore
//
//  Synopsis:
//      The value store utilizes a trie structure to store lists of strings
//  that often have similar prefixes and associates a data element with each
//  string.
//
//------------------------------------------------------------------------

typedef void (*ValueStoreTraversalCallback)(const xstring_ptr& strKey, _In_ XHANDLE hValue, XHANDLE extraData);

class CValueStore final : public IValueStore
{
public:
    CValueStore(bool bObjectBase = false, _In_opt_ CDependencyObject* pOwner = nullptr);
   ~CValueStore();

// IValueStore methods

    XUINT32 Release() override;

    _Check_return_ HRESULT PutValue(_In_ const xstring_ptr_view& strName, _In_ XHANDLE hValue) override;
    _Check_return_ HRESULT GetValue(_In_ const xstring_ptr_view& strName, _Out_ XHANDLE *phValue) override;
    HRESULT SetCleanupNotifier(_In_ ValueStoreCleanupCallback pCallBack) override
        {
            m_pCleanupCallback = pCallBack;
            return S_OK;
        }
    _Check_return_ HRESULT Traverse(_In_ ValueStoreTraversalCallback pCallback, _In_ XHANDLE pExtraData);

// CValueStore fields

    XUINT32 m_cRef;
    bool m_bObjectBase;
private:
    ValueStoreCleanupCallback m_pCleanupCallback;
    containers::vector_map<xstring_ptr, XHANDLE> m_map;
    CDependencyObject* m_pOwner;
};