// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <MetadataAPI.h>
#include <CDependencyObject.h>
#include <UIElement.h>
#include <dopointercast.h>
#include <TypeTableStructs.h>
#include <GridLength.h>
#include <UIAEnums.h>
#include <EnumDefs.h>
#include <primitives.h>
#include <CString.h>
#include <Point.h>
#include <Rect.h>
#include <Size.h>
#include <Double.h>
#include <InheritedProperties.h>
#include <DOCollection.h>
#include <Type.h>
#include <ModifiedValue.h>
#include <ThemeResourceExtension.h>
#include <ThemeResource.h>
#include "DependencyObjectDCompRegistry.h"
#include <corep.h>
#include <DeferredElement.h>
#include "theming\inc\Theme.h"
#include "CDOAssociativeImpl.h"
#include "Framework.h"
#include <FxCallbacks.h>

using namespace DirectUI;

CDependencyObject::CDependencyObject(_In_opt_ CCoreServices* pCore)
    : m_sharedState(
        Flyweight::Create<CDOSharedState>(
              pCore,
              pCore,
              CDOSharedState::s_defaultRenderChangedHandler,
              CDOSharedState::s_defaultBaseUri,
              xref::weakref_ptr<VisualTree>()))

    , m_theme(Theming::Theme::None)
    , m_isProcessingInheritanceContextChanged(false)
    , m_requiresThreadSafeAddRefRelease(false)
    , m_requiresReleaseOverride(false)
    , m_objectStrictness(ObjectStrictness::Agnostic)
    , m_checkForResourceOverrides(false)
    , m_canParserOverwriteBaseUri(true)
{}

KnownTypeIndex CDependencyObject::GetTypeIndex() const
{
    return DependencyObjectTraits<CDependencyObject>::Index;
}

bool CDependencyObject::IsSimpleSparseSet(KnownPropertyIndex pid) const
{
    ASSERT(MetadataAPI::IsKnownSimplePropertyIndex(pid));

    return GetPropertyBitField(
        m_valid,
        MetadataAPI::GetPropertySlotCount(GetTypeIndex()),
        MetadataAPI::GetPropertySlot(pid));
}

void CDependencyObject::SetSimpleSparseFlag(KnownPropertyIndex pid, bool value)
{
    ASSERT(MetadataAPI::IsKnownSimplePropertyIndex(pid));

    if (value)
    {
        SetPropertyBitField(
            m_valid,
            MetadataAPI::GetPropertySlotCount(GetTypeIndex()),
            MetadataAPI::GetPropertySlot(pid));
    }
    else
    {
        ClearPropertyBitField(
            m_valid,
            MetadataAPI::GetPropertySlotCount(GetTypeIndex()),
            MetadataAPI::GetPropertySlot(pid));
    }
}

#pragma region DComp

IUnknown* CDependencyObject::GetDCompAnimation(KnownPropertyIndex propertyIndex)
{
    CValue result;
    IFCFAILFAST(GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(propertyIndex),
        &result));
    return result.AsIUnknown();
}

void CDependencyObject::SetDCompAnimation(_In_opt_ IUnknown* animation, KnownPropertyIndex propertyIndex)
{
    if (animation == nullptr) 
    {
        IFCFAILFAST(ClearEffectiveValueInSparseStorage(MetadataAPI::GetDependencyPropertyByIndex(propertyIndex)));
        return;
    }

    // DComp objects have tear-off interfaces, so merely casting a pointer isn't enough. Actually QI to IUnknown.
    xref_ptr<IUnknown> unknown;
    VERIFYHR(animation->QueryInterface(IID_PPV_ARGS(unknown.ReleaseAndGetAddressOf())));

    CValue v;
    v.SetIUnknownAddRef(unknown);
    IFCFAILFAST(SetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(propertyIndex),
        v));
}

xref_ptr<WUComp::ICompositionAnimation> CDependencyObject::GetWUCDCompAnimation(KnownPropertyIndex propertyIndex)
{
    xref_ptr<WUComp::ICompositionAnimation> animation;
    CValue result;

    IFCFAILFAST(GetEffectiveValueInSparseStorage(
        MetadataAPI::GetDependencyPropertyByIndex(propertyIndex),
        &result));

    IUnknown* unknown = result.AsIUnknown();
    if (unknown != nullptr)
    {
        VERIFYHR(unknown->QueryInterface(IID_PPV_ARGS(animation.ReleaseAndGetAddressOf())));
    }

    return animation;
}

#pragma endregion

void CDependencyObject::ActivateImpl()
{
    ASSERT(m_bitFields.fLive == false);
    m_bitFields.fLive = true;
}

void CDependencyObject::DeactivateImpl()
{
    ASSERT(m_bitFields.fLive == true);
    m_bitFields.fLive = false;
}

// Gets the DO that has a namescope store associated with it.
CDependencyObject* CDependencyObject::GetStandardNameScopeOwner()
{
    return GetStandardNameScopeOwnerInternal(nullptr);
}

CDependencyObject* CDependencyObject::GetParent() const
{
    CDependencyObject* pParent = GetParentInternal(/* publicParentOnly */ true);

    // If we're not active OR (we're active AND our parent has a parent)
    // return OUR public parent, otherwise return nullptr.
    //
    // This bit of logic ensures we always hide the real visual root from
    // the public API.
    if (!this->IsActive() || (pParent && pParent->GetParentInternal(/* publicParentOnly */ false)))
    {
        return pParent;
    }
    else
    {
        return nullptr;
    }
}

CDependencyObject* CDependencyObject::GetParentInternal(_In_ bool publicParentOnly /* default = true */) const
{
    // If the parent is for the inheritance context only the m_pParent field is being repurposed to store
    // a weakref and is invalid.
    // If we're asking for the public parent we don't return a parent if it's marked nonpublic
    if (m_bitFields.fParentIsInheritanceContextOnly
        || (publicParentOnly && !m_bitFields.fParentIsPublic))
    {
        return nullptr;
    }
    else
    {
        return m_pParent;
    }
}

// Gets the parent of an element in the tree if it is a UIElement.
CUIElement* CDependencyObject::GetUIElementParentInternal(_In_ bool publicParentOnly /* default = true */) const
{
    bool shouldConvert = false;
    if (!publicParentOnly || m_bitFields.fParentIsPublic)
    {
        shouldConvert = true;
    }
    if (shouldConvert && !m_bitFields.fParentIsInheritanceContextOnly)
    {
        return do_pointer_cast<CUIElement>(m_pParent);
    }
    return nullptr;
}

bool CDependencyObject::IsParentAware() const
{
    return (!DoesAllowMultipleAssociation() || DoesAllowMultipleParents());
}

// Walks up the tree till it finds a UIElement to propagate layout dirty flags.
void CDependencyObject::PropagateLayoutDirty(bool affectsMeasure, bool affectsArrange)
{
    CDependencyObject* parent = GetParentInternal(/* publicParentOnly */ false);
    if (parent != nullptr)
    {
        parent->PropagateLayoutDirty(affectsMeasure, affectsArrange);
    }
}

// Return whether a given property index requires parent lookup to use the logical tree.
bool CDependencyObject::UseLogicalParent(_In_ KnownPropertyIndex propertyIndex) const
{
    return propertyIndex == KnownPropertyIndex::FrameworkElement_DataContext;
}

void CDependencyObject::ReleaseDCompResources()
{
    // Do nothing
}

std::size_t CDependencyObject::GetParentCount() const
{
    return(m_pParent != nullptr && !m_bitFields.fParentIsInheritanceContextOnly) ? 1 : 0;
}

_Ret_notnull_ CDependencyObject* CDependencyObject::GetParentItem(std::size_t index) const
{
    // This invariant is established by GetParentCount, which should be called before this method
    ASSERT(index == 0 && m_pParent != nullptr && !m_bitFields.fParentIsInheritanceContextOnly);

    return m_pParent;
}

#if DBG

// Returns true if the argument DO is a parent of this DO. Available
// in DBG only for invariant assertion.
bool CDependencyObject::HasParent(_In_ CDependencyObject *pCandidate) const
{
    return pCandidate == GetParentInternal(false);
}

#endif


// Gets the mentor by walking up the InheritanceContext (parent) tree.
// Returns null if no mentor exists. CMultiParentShareableDependencyObject
// has an override to handle multiple parents
_Ret_maybenull_ CDependencyObject* CDependencyObject::GetMentor()
{
    CDependencyObject* pParent = m_bitFields.fParentIsInheritanceContextOnly ? m_pMentor->lock() : m_pParent;
    if (pParent)
    {
        return pParent->GetMentor();
    }
    else
    {
        return nullptr;
    }
}

// Sets m_pParent but only for use with InheritanceContext and not for GetParent
void CDependencyObject::SetParentForInheritanceContextOnly(_In_opt_ CDependencyObject *pDO)
{
    if (m_bitFields.fParentIsInheritanceContextOnly)
    {
        // Clean up the existing mentor
        ASSERT(m_pMentor);
        delete m_pMentor;
        m_pMentor = nullptr;
        m_bitFields.fParentIsInheritanceContextOnly = FALSE;
    }

    if (pDO != nullptr && m_pParent == nullptr)
    {
        // Create a WeakRef to the parent for use with InheritanceContext only
        m_pMentor = new xref::weakref_ptr<CDependencyObject>(xref::get_weakref(pDO));
        m_bitFields.fParentIsInheritanceContextOnly = TRUE;
    }
}


// Add an expected reference on the peer (if there is one).  That will ensure that it stays alive as long
// as the core object stays alive, but won't protect the pair if GC decides to remove them.
// Note that unlike SetExpectedReferenceOnPeer, there can be multiple Add calls, and must be matched
// by an equal number of Clear calls.  This allows for multiple independent references to the peer,
// whereas with SetExpectedReferenceOnPeer only allows for one.
_Check_return_ HRESULT CDependencyObject::AddExtraExpectedReferenceOnPeer()
{
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_SetExpectedReferenceOnPeer(this));
    }

    return S_OK;

}

_Check_return_ HRESULT CDependencyObject::ClearExtraExpectedReferenceOnPeer()
{
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_ClearExpectedReferenceOnPeer(this));
    }

    return S_OK;
}

void CDependencyObject::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    // Do nothing. Subclasses that can be targeted by WUC animations will override this.
}

//------------------------------------------------------------------------
//
//  Method:   IsAncestorOf
//
//  Synopsis:
//      TRUE iff the given UI element is a child of this element.
//      'this' element can be the internal root ScrollViewer.
//
//------------------------------------------------------------------------
bool CDependencyObject::IsAncestorOf(_In_ CDependencyObject* object)
{
    CDependencyObject* pDO = object->GetParentInternal(false /*fPublic*/);

    while(pDO && pDO != this)
    {
        pDO = pDO->GetParentInternal(false /*fPublic*/);
    }

    return this == pDO;
}

_Check_return_ HRESULT CDependencyObject::GetRealizingProxy(
    _Outptr_ CDeferredElement** proxy)
{
    *proxy = nullptr;

    CValue realizingProxy;

    IFC_RETURN(GetEffectiveValueInSparseStorage(
        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_RealizingProxy),
        &realizingProxy));

    *proxy = reinterpret_cast<CDeferredElement*>(realizingProxy.As<valuePointer>());

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetRealizingProxy(
    _In_ CDeferredElement* proxy)
{
    CValue realizingProxy;

    // Use untyped pointer (valuePointer).  Using valueObject will try to do all kinds of unnecessary things (e.g. creating peers, etc.).
    realizingProxy.Set<valuePointer>(proxy);

    IFC_RETURN(SetEffectiveValueInSparseStorage(
        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_RealizingProxy),
        realizingProxy));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ClearRealizingProxy()
{
    IFC_RETURN(ClearEffectiveValueInSparseStorage(
        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_RealizingProxy)));

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::ValidateAndInit(_In_ CDependencyObject *pDO, _Out_ CDependencyObject **ppDO)
{
    IFC_RETURN(pDO->InitInstance());
   *ppDO = pDO;

    return S_OK;
}

RENDERCHANGEDPFN CDependencyObject::NWGetRenderChangedHandlerInternal() const
{
    return m_sharedState->Value().GetRenderChangedHandler();
}

void CDependencyObject::NWSetRenderChangedHandlerInternal(RENDERCHANGEDPFN handler)
{
    const auto& current = m_sharedState->Value();

    if (handler != current.GetRenderChangedHandler())
    {
        auto core = GetContext();

        m_sharedState = Flyweight::Create<CDOSharedState>(
            core,
            core,
            handler,
            current.GetBaseUri(),
            current.GetVisualTree());
    }
}

IPALUri* CDependencyObject::GetPreferredBaseUri() const
{
    return m_sharedState->Value().GetBaseUri();
}

xstring_ptr CDependencyObject::GetUINameForTracing()
{
    XStringBuilder nameBuilder;

    // Find the named UI and framework element parents (or self!)
    CUIElement* pUIElement = GetNamedSelfOrParent<CUIElement>();
    if (pUIElement != nullptr)
    {
        CFrameworkElement* pFMElement = pUIElement->GetNamedSelfOrParent<CFrameworkElement>();

        // If they are not the same, first emit the UI element part.
        if (pUIElement != pFMElement)
        {
            IGNOREHR(nameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+UI:")));
            IGNOREHR(nameBuilder.Append(pUIElement->m_strName));
        }

        // Now emit the framework element name, if we got one.
        if (pFMElement != nullptr)
        {
            IGNOREHR(nameBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+FM:")));
            IGNOREHR(nameBuilder.Append(pFMElement->m_strName));
        }
    }

    xstring_ptr name;
    IGNOREHR(nameBuilder.DetachString(&name));
    return name;
}

xstring_ptr CDependencyObject::GetUIPathForTracing(bool followDOParentChain)
{
    XStringBuilder pathBuilder;

    // Walk up the logical tree and emit the names into the buffer.
    // Limit the number of iterations for perf and cycle concerns (e.g. with namescopes).
    XUINT32 cIterations = 0;
    CFrameworkElement* pCurrent = GetNamedSelfOrParent<CFrameworkElement>();
    while ((cIterations < 32) && (pCurrent != nullptr))
    {
        IGNOREHR(pathBuilder.Append(XSTRING_PTR_EPHEMERAL(L"+P:")));
        IGNOREHR(pathBuilder.Append(pCurrent->m_strName));

        if (followDOParentChain)
        {
            CDependencyObject* pParent = pCurrent->GetParent();
            pCurrent = pParent->GetNamedSelfOrParent<CFrameworkElement>();
        }
        else
        {
            // If we are a template member, jump to our template owner.
            // Otherwise, jump to our namescope owner.
            CDependencyObject* pParent = pCurrent->GetTemplatedParent();
            if (pParent == nullptr)
            {
                pParent = pCurrent->GetStandardNameScopeOwner();
                if (pParent == nullptr)
                {
                    break;
                }
            }

            pCurrent = pParent->GetNamedSelfOrParent<CFrameworkElement>();
        }

        cIterations++;
    }

    xstring_ptr path;
    IGNOREHR(pathBuilder.DetachString(&path));
    return path;
}

template <typename T>
T*
CDependencyObject::GetNamedSelfOrParent()
{
    T* pFound = nullptr;

    for (CDependencyObject* pDO = this; pDO != nullptr;  pDO = pDO->GetParentInternal(/*fPublic*/FALSE))
    {
        if (!pDO->m_strName.IsNull())
        {
            T* pCurrent = do_pointer_cast<T>(pDO);
            if (pCurrent)
            {
                pFound = pCurrent;
                break;
            }
        }
    }

    return pFound;
}

CDependencyObject* CDependencyObject::GetParentFollowPopups()
{
    CDependencyObject* parent = GetParentInternal();

    if (parent == nullptr || !parent->OfTypeByIndex<KnownTypeIndex::PopupRoot>())
    {
        return parent;
    }
    else
    {
        CXamlIslandRoot* xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(GetContext()->GetRootForElement(this));
        CPopupRoot* popupRoot = xamlIslandRoot ? xamlIslandRoot->GetPopupRootNoRef() : GetContext()->GetMainPopupRoot();

        // We're at the child of a popup. Find the popup and go to it. When we started the walk we could have been on
        // something like a Hyperlink that's not a UIElement, but at this point we must be at a UIElement.
        FAIL_FAST_ASSERT(OfTypeByIndex<KnownTypeIndex::UIElement>());
        CUIElement* thisAsUIE = static_cast<CUIElement*>(this);
        // We're interested in the current state of the tree, so ignore any unloading child of a popup.
        CPopup* popup = popupRoot->GetOpenPopupWithChild(thisAsUIE, false /* checkUnloadingChildToo */);

        if (popup != nullptr)
        {
            // We found an open popup. Return it.
            return popup;
        }
        else
        {
            // There's no open popup with this element set as the child. Return the visual parent. Maybe this
            // element is in the popup root's unloading storage.
            return parent;
        }
    }
}

CPopup* CDependencyObject::GetFirstAncestorPopup(bool windowedPopupOnly)
{
    CDependencyObject* ancestor = GetParentFollowPopups();
    while (ancestor)
    {
        if (ancestor->OfTypeByIndex(KnownTypeIndex::Popup)
            && (!windowedPopupOnly || static_cast<CPopup*>(ancestor)->IsWindowed()) )
        {
            return static_cast<CPopup*>(ancestor);
        }
        ancestor = ancestor->GetParentFollowPopups();
    }
    return nullptr;
}

namespace AssociativeStorage
{
    template <> Detail::VariableStorageBlock<Detail::Bitfield<CDOFields>>::OffsetLookupTable Detail::VariableStorageBlock<Detail::Bitfield<CDOFields>>::s_lookupTable;
}

ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(CDependencyObject, ModifiedValuesList, CDOFields, ModifiedValues);
ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(CDependencyObject, ThemeResourceMap, CDOFields, ThemeResources);
ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(CDependencyObject, xref_ptr<CDependencyObject>, CDOFields, AutomationAnnotations);
ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(CDependencyObject, SetterValueChangedNoficationSubscribersList, CDOFields, SetterValueChangedNoficationSubscribers);
ASSOCIATIVE_STORAGE_ACCESSORS_IMPL(CDependencyObject, Resources::ScopedResources::OverrideInfo, CDOFields, OverrideResourceKey);