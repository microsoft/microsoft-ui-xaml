// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>

using namespace Jupiter::NameScoping;

std::tuple<CDependencyObject*, NameScopeType> CCoreServices::GetAdjustedReferenceObjectAndNamescopeType(
    _In_ CDependencyObject* referenceObject)
{
    // Case 1: TemplateParent(ro) & !NameScopeOwner(ro) => TemplateParent as NameScopeOwner
    CDependencyObject* templatedParent = referenceObject->GetTemplatedParent();
    // In Threshold if a reference object is a NameScopeOwner, the case of a UserControl as part of a Template,
    // we adjust the NameScope owner to the UserControl, even if it is also a TemplateNameScope member.
    // Prior to Threshold we would simply use the TemplateNameScope, which would result in FindName calls
    // using a UserControl as a reference object that one would expect to work failing.
    bool useTemplatedParent = templatedParent && 
                              referenceObject->IsTemplateNamescopeMember() &&
                              !referenceObject->IsStandardNameScopeOwner();
    if (useTemplatedParent) return std::make_tuple(templatedParent, NameScopeType::TemplateNameScope);

    // Case 2: Default => StandardNameScopeOwner tree walk.
    return std::make_tuple(referenceObject->GetStandardNameScopeOwner(), NameScopeType::StandardNameScope);
}

_Check_return_
xref_ptr<CDependencyObject> CCoreServices::TryGetElementByName(
    _In_ const xstring_ptr_view& strName,
    _In_ CDependencyObject *pReferenceObject)
{
    auto namescopeInfo = GetAdjustedReferenceObjectAndNamescopeType(pReferenceObject);
    return xref_ptr<CDependencyObject>(GetNamedObject(
        strName, std::get<0>(namescopeInfo), std::get<1>(namescopeInfo)));
}

_Check_return_ HRESULT
CCoreServices::SetNamedObject(
    _In_ const xstring_ptr_view& strName,
    _In_ const CDependencyObject *pNamescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType,
    _In_ CDependencyObject* pObject)
{
    IFCPTR_RETURN(pNamescopeOwner);
    ASSERT(nameScopeType == Jupiter::NameScoping::NameScopeType::TemplateNameScope || pNamescopeOwner->IsStandardNameScopeOwner());

    if (nameScopeType == Jupiter::NameScoping::NameScopeType::TemplateNameScope)
    {
        m_nameScopeRoot.SetNamedObject(strName, pNamescopeOwner, nameScopeType, xref::get_weakref(pObject));
    }
    else
    {
        // Is this needed / could it be just an ASSERT?
        IFCEXPECT_RETURN(pNamescopeOwner->IsStandardNameScopeOwner());
        m_nameScopeRoot.EnsureNameScope(pNamescopeOwner, nullptr);

        // SetNamedObject on ourselves should only register a weakref to avoid a circular reference.
        if (pNamescopeOwner == pObject)
        {
            m_nameScopeRoot.SetNamedObject(strName, pNamescopeOwner, nameScopeType, xref::get_weakref(pObject));
        }
        else
        {
            // In the future we should be able to switch these over to weakrefs too perhaps?
            m_nameScopeRoot.SetNamedObject(strName, pNamescopeOwner, nameScopeType, xref_ptr<CDependencyObject>(pObject));
        }
    }
    return S_OK;
}

_Check_return_ CDependencyObject *
CCoreServices::GetNamedObject(
    _In_ const xstring_ptr_view& strName,
    _In_opt_ const CDependencyObject *pNamescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType)
{
    // If we're destroying the core, then there's no point in looking up the named object;
    // the namescope tables are going to be destroyed, if they haven't already been, and
    // looking up stuff in them is only asking for trouble.
    if (m_bIsDestroyingCoreServices || !pNamescopeOwner)
    {
        return nullptr;
    }

    ASSERT(nameScopeType == Jupiter::NameScoping::NameScopeType::TemplateNameScope || pNamescopeOwner->IsStandardNameScopeOwner());

    auto instance = m_nameScopeRoot.GetNamedObjectIfExists(strName, pNamescopeOwner, nameScopeType);

    // FUTURE: Move DeferredElement to the much more compact INameScopeDeferredElementCreator pattern.
    // FUTURE: This method doens't propagate error information. Deferred creation is a rich operation that
    // has the potential to fail. We need to propagate this information somehow.
    // FUTURE: Stop dropping the ref here and returning an unsafe pointer
    if (instance && instance->OfTypeByIndex<KnownTypeIndex::DeferredElement>())
    {
        // GetNamedObject should not AddRef the object - this is only a lookup.
        // CDeferredElement::Realize follows the general pattern of adding a reference, but it is dropped here on purpose.
        // Breaking ownership semantics is a hack, but this doesn't crash since the object is kept alive by the tree.
        xref_ptr<CDependencyObject> spResult;
        IGNOREHR(static_cast<CDeferredElement*>(instance.get())->Realize(spResult.ReleaseAndGetAddressOf()));
        return spResult.get();
    }
    else
    {
        return instance.get();
    }
}

_Check_return_ HRESULT
CCoreServices::ClearNamedObject(
    _In_ const xstring_ptr_view& strName,
    _In_opt_ const CDependencyObject *pNamescopeOwner,
    _In_opt_ CDependencyObject* originalEntry)
{
    // If we're destroying the core, then there's no point in clearing the named object;
    // the namescope tables are going to be destroyed, if they haven't already been, and
    // looking up stuff in them is only asking for trouble.
    if (m_bIsDestroyingCoreServices)
    {
        return S_OK;
    }

    IFCPTR_RETURN(pNamescopeOwner);
    ASSERT(pNamescopeOwner->IsStandardNameScopeOwner());

    if (!m_nameScopeRoot.HasStandardNameScopeTable(pNamescopeOwner)) IFC_RETURN(E_POINTER);
    auto existingEntry = m_nameScopeRoot.GetNamedObjectIfExists(strName, pNamescopeOwner, Jupiter::NameScoping::NameScopeType::StandardNameScope);

    // The callers of this method all have a very bad smell to them. We silently allow duplicate
    // names to be registered and overwrite each other, we shouldn't be using an error to signal when
    // the name has been overridden, it's by design. This method would better be named EnsureObjectUnregistered
    // and would simply silently complete when the instance was no longer present in the table, that would
    // avoid all the IGNOREHR wraps this guy gets.
    //
    // One common pattern here will be that for weakrefs the weakref will fail to resolve as we're
    // unregistering ourselves from the dtor, that will return a null entry and calling ClearNamedObjectIfExists
    // will simply remove the table entry, freeing up a little bit of space.
    IFCEXPECT_NOTRACE_RETURN(existingEntry.get() == nullptr ||
        existingEntry.get() == originalEntry);
    m_nameScopeRoot.ClearNamedObjectIfExists(strName, pNamescopeOwner);

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::RemoveNameScope(
    _In_ const CDependencyObject *pNamescopeOwner, Jupiter::NameScoping::NameScopeType nameScopeType)
{
    // If core is being destroyed, the namescopes will be destroy along with it, so we don't need to worry about
    // removing them here.  This will avoid reentrance issues.
    if (m_bIsDestroyingCoreServices) return S_OK;

    // If the core has been partially torn down, then some DO may
    // be trying to clear a namescope after the namescope tables has gone away.
    IFCEXPECT_RETURN(!m_nameScopeRoot.HasBeenReleased());
    m_nameScopeRoot.RemoveNameScopeIfExists(pNamescopeOwner, nameScopeType);

    return S_OK;
}

_Check_return_ bool
CCoreServices::HasRegisteredNames(_In_ const CDependencyObject *pNamescopeOwner) const
{
    // If we're destroying the core, then there's no point in looking up the namescope owner;
    // the namescope tables are going to be destroyed, if they haven't already been, and
    // looking up stuff in them is only asking for trouble.
    if (m_bIsDestroyingCoreServices)
    {
        return false;
    }

    return m_nameScopeRoot.HasStandardNameScopeTable(pNamescopeOwner);
}

Jupiter::NameScoping::NameScopeRoot& CCoreServices::GetNameScopeRoot()
{
    ASSERT(!m_nameScopeRoot.HasBeenReleased());
    return m_nameScopeRoot;
}

// Do a first-stage Enter on pDependencyObject to verify that the
// there are no Name collisions.
//
// A better long-term implementation would be to provisionally create
// a Valuestore for the namescope, and change the parser to validate
// the names.  That would allow line & col to be set with the parse
// error.  This is going to have to do for now.
_Check_return_ HRESULT
CCoreServices::PostParseRegisterNames(_In_ CDependencyObject *pDependencyObject)
{
    EnterParams enterParams(
        /*isLive*/                FALSE,
        /*skipNameRegistration*/  FALSE,
        /*coercedIsEnabled*/      TRUE,
        /*useLayoutRounding*/     EnterParams::UseLayoutRoundingDefault,
        /*visualTree*/            VisualTree::GetForElementNoRef(pDependencyObject, LookupOptions::NoFallback)
        );

    IFCPTR_RETURN(pDependencyObject);
    pDependencyObject->SetIsStandardNameScopeOwner(TRUE);
    pDependencyObject->SetIsStandardNameScopeMember(TRUE);

    m_nameScopeRoot.EnsureNameScope(pDependencyObject, nullptr);
    IFC_RETURN(pDependencyObject->Enter(pDependencyObject, enterParams));

    return S_OK;
}

// Get the namescope owner set by the parser in a call to SetParserNamescope
CDependencyObject* CCoreServices::GetParserNamescopeOwner()
{
    if (m_parserNamescope)
    {
        return m_parserNamescope->GetNamescopeOwner();
    }
    else
    {
        return nullptr;
    }
}

CDeferredElement* CCoreServices::GetDeferredElementIfExists(
    _In_ const xstring_ptr_view& strName,
    _In_ const CDependencyObject *pNamescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType)
{
    // If we're destroying the core, then there's no point in looking up the named object;
    // the namescope tables are going to be destroyed, if they haven't already been, and
    // looking up stuff in them is only asking for trouble.
    if (m_bIsDestroyingCoreServices)
    {
        return nullptr;
    }

    auto instance = m_nameScopeRoot.GetNamedObjectIfExists(strName, pNamescopeOwner, nameScopeType);

    if (instance && instance->OfTypeByIndex<KnownTypeIndex::DeferredElement>())
    {
        xref_ptr<CDeferredElement> retval;
        IGNOREHR(DoPointerCast(retval, instance));
        return retval;
    }
    else
    {
        return nullptr;
    }
}
