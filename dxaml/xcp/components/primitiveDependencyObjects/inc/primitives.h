// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xtable.h>
#include <CDependencyObject.h>

//------------------------------------------------------------------------
//
//  Class:  CEnumerated
//
//  Synopsis:
//      Base enumerated type converter.  This class can't be created directly.
//
//------------------------------------------------------------------------

class CEnumerated : public CDependencyObject
{
protected:
    CEnumerated(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    static _Check_return_ HRESULT CreateEnumerateHelper(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate,
        _In_ XUINT32 cTable,
        _In_reads_(cTable) const XTABLE *pTable,
        _In_ KnownTypeIndex enumTypeIndex,
        _In_ bool IsFlagsEnumeration = false);

public:
    // Creation method

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetEnumTypeIndex() const
    {
        return m_enumTypeIndex;
    }

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override;

    bool DoesAllowMultipleAssociation() const final { return true; }

    // CEnumerated fields

    XINT32          m_nValue        = 0;
    KnownTypeIndex  m_enumTypeIndex = KnownTypeIndex::Enumerated;
};

//------------------------------------------------------------------------
//
//  Class:  CBoolean
//
//  Synopsis:
//      Created by XML parser to hold a True/False value
//
//------------------------------------------------------------------------

class CBoolean final : public CEnumerated
{
private:
    CBoolean(_In_ CCoreServices *pCore)
        : CEnumerated(pCore)
    {}

public:
    static _Check_return_ HRESULT CreateCValue(
        _In_ CREATEPARAMETERS* pCreate,
        _Inout_ CValue& value);

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);
};

//------------------------------------------------------------------------
//
//  Class:  CInt32
//
//  Synopsis:
//      Created by XML parser to hold generic int32 values
//
//------------------------------------------------------------------------

class CInt32 : public CDependencyObject
{
private:
    CInt32(_In_ CCoreServices* pCore)
        : CDependencyObject(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CInt32()  // !!! FOR UNIT TESTING ONLY !!!
        : CInt32(nullptr)
    {}
#endif

    static _Check_return_ HRESULT CreateCValue(
        _In_ CREATEPARAMETERS* pCreate,
        _Inout_ CValue& value);

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override;

    bool DoesAllowMultipleAssociation() const final { return true; }

    INT32 m_iValue = 0;
};
