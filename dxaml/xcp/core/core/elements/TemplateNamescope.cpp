// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Defines the namescope used for control templates.  Control namescopes
// encompass the contents of the control's instantiated template and are
// used to register and resolve names in isolation from the global
// namescope or other instantiations.

#include "precomp.h"

#include "TemplateNamescope.h"
#include "XamlQualifiedObject.h"

using namespace Jupiter::NameScoping;

_Check_return_ HRESULT NameScopeHelper::RegisterName(
    _In_ const xstring_ptr& strName,
    const std::shared_ptr<XamlQualifiedObject>& qoScopedObject)
{
    CDependencyObject* pScopedObject = NULL;

    // By now we should have an owner, either from the Create or from SetNamescopeOwner
    ASSERT(m_pNameScopeOwner);
    
    ASSERT(!!qoScopedObject);
    pScopedObject = qoScopedObject->GetDependencyObject();
    IFCEXPECT_RETURN(pScopedObject);

    IFC_RETURN(m_pCore->SetNamedObject(
        strName,
        m_pNameScopeOwner,
        m_nameScopeType,
        pScopedObject));

    return S_OK;
}

// Receive a namescope owner to use when talking to the core, if we don't already
// have one.  If we didn't have one, then create the name scope.
void NameScopeHelper::EnsureNamescopeOwner(
    const std::shared_ptr<XamlQualifiedObject>& qoOwner)
{
    // Ignore this if we already have an owner... the typical TemplateContent
    // scneario.
    if (m_pNameScopeOwner != nullptr) return;

    ASSERT(m_nameScopeType == NameScopeType::StandardNameScope);

    m_pNameScopeOwner = qoOwner->GetDependencyObject();
    ASSERT(m_pNameScopeOwner);

    m_pCore->GetNameScopeRoot().EnsureNameScope(m_pNameScopeOwner, nullptr);
    m_pNameScopeOwner->SetIsStandardNameScopeOwner(TRUE);
    m_pNameScopeOwner->SetIsStandardNameScopeMember(TRUE);
}




