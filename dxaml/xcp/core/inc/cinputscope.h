// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Use of this source code is subject to the terms of the Microsoft
// premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license
// agreement, you are not authorized to use this source code.
// For the terms of the license, please see the license agreement
// signed by you and Microsoft.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//  Abstract:
//      Declaration of InputScope class and related classes.
//  Class:  CInputScopeName
//
//  Synopsis:
//      Holds an input scope name value.

class CInputScopeName : public CDependencyObject
{
private:
    CInputScopeName(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:

    // Creation method

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInputScopeName>::Index;
    }

    // CInputScopeNameValue field

    DirectUI::InputScopeNameValue m_nameValue{};
};

//------------------------------------------------------------------------
//
//  Class:  CInputScopeNameCollection
//
//  Synopsis:
//      Created by XAML parser to hold a list of input scope names
//
//------------------------------------------------------------------------

class CInputScopeNameCollection : public CDOCollection
{
private:
    CInputScopeNameCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore)
    {}

public:

    ~CInputScopeNameCollection() override
    {
        Destroy();
    }

    // Creation method

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInputScopeNameCollection>::Index;
    }
};

//------------------------------------------------------------------------
//
//  Class:  CInputScope
//
//  Synopsis:
//      Holds a collection of input scope names.
//
//------------------------------------------------------------------------

class CInputScope : public CDependencyObject
{
private:
    CInputScope(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CInputScope() override
    {
        ReleaseInterface(m_pNames);
    }

public:

    // Creation method

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    // CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInputScope>::Index;
    }

    // CInputScopeName collection

    CInputScopeNameCollection *m_pNames = nullptr;
};
