// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "INamescope.h"

#include <namescope\inc\NameScopeRoot.h>

class NameScopeHelper
    : public INameScope
{
private:
    XUINT32 m_cRef;
    CCoreServices* m_pCore;

    // Owner of the namescope, may be null until EnsureNamescopeOwner
    CDependencyObject* m_pNameScopeOwner;
    Jupiter::NameScoping::NameScopeType m_nameScopeType;

public:
    NameScopeHelper(
        _In_ CCoreServices* pCore,
        _In_ CDependencyObject* pNamescopeOwner,
        _In_ Jupiter::NameScoping::NameScopeType nameScopeType)
        : m_pCore(pCore)
        , m_pNameScopeOwner(pNamescopeOwner) // may be null
        , m_nameScopeType(nameScopeType)
        , m_cRef(1)
    {
        XCP_WEAK(&m_pCore);
    }

    _Check_return_ HRESULT RegisterName(
        _In_ const xstring_ptr& strName,
        const std::shared_ptr<XamlQualifiedObject>& qoScopedObject) final;

    void EnsureNamescopeOwner(
        const std::shared_ptr<XamlQualifiedObject>& qoScopedObject) final;

    Jupiter::NameScoping::NameScopeType GetNameScopeType() const final
    {
        return m_nameScopeType;
    }

    CDependencyObject* GetNamescopeOwner() const final
    {
        return m_pNameScopeOwner;
    }

    XUINT32 AddRef() override
    {
        return ++m_cRef;
    }

    XUINT32 Release() override
    {
        XUINT32 cRef = --m_cRef;
        if (!cRef) delete this;
        return cRef;
    }
};

class NullNameScopeHelper
    : public INameScope
{
public:
    NullNameScopeHelper() = default;

    _Check_return_ HRESULT RegisterName(
        _In_ const xstring_ptr& strName,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoScopedObject) final
    {
        return S_OK;
    }

    void EnsureNamescopeOwner(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoScopedObject) final
    {
    }

    CDependencyObject* GetNamescopeOwner() const final
    {
        return nullptr;
    }

    Jupiter::NameScoping::NameScopeType GetNameScopeType() const final
    {
        ASSERT(FALSE);
        return Jupiter::NameScoping::NameScopeType::TemplateNameScope;
    }

    XUINT32 AddRef() override
    {
        return ++m_cRef;
    }

    XUINT32 Release() override
    {
        XUINT32 cRef = --m_cRef;
        if (!cRef) delete this;
        return cRef;
    }

private:
    unsigned int m_cRef = 1;
};