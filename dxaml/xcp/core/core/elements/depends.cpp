// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "strsafe.h"
#include "MetadataAPI.h"
#include "CustomDependencyProperty.h"
#include "EnterParams.h"
#include "Animation.h"
#include <XamlTraceLogging.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "ThemeResource.h"
#include <stack_allocator.h>
#include "DeferredMapping.h"
#include "AutomationAnnotationCollection.h"
#include <CValueUtil.h>
#include "theming\inc\Theme.h"
#include "CDOAssociativeImpl.h"
#include <ErrorHelper.h>
#include <xcperrorresource.h>
#include <SimplePropertiesHelpers.h>
#include <UIAWrapper.h>
#include <string>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;

bool TryPegManagedPeer(_In_ CDependencyObject* element, _In_ bool isShutdownException);
void TryUnpegManagedPeer(_In_ CDependencyObject* element, _In_ bool isShutdownException);

#define TRACE_DIRTY 0

CDependencyObject::~CDependencyObject()
{
#if DBG
    InheritedProperties::RecordTextPropertyUsage(this);
#endif

    ClearThemeResources();

    auto core = GetContext();

    if (core)
    {
        // If this core object was marked as a GC root, unmark it,
        core->UnpegNoRefCoreObjectWithoutPeer(this);

        // Remove any remaining requests from the event manager.  (Even if all the requests have been
        // removed already, there will still be an empty entry for this object in the map.)
        CEventManager* pEventManager = core->GetEventManager();
        if (pEventManager)
        {
            IGNOREHR(pEventManager->RemoveObject(this));
        }

        if (HasDeferred())
        {
            IGNOREHR(CDeferredMapping::NotifyDestroyed(this));
        }

        // If this assert fails, it means that the class declaring simple properties did not remove sparse property entries.
        // Add this call in class' dtor:
        // SimpleProperty::Property::NotifyDestroyed<CLASSNAME>(this);

        ASSERT(SimpleProperty::Property::AreAllCleanedUp(this));
    }

    DisconnectInheritedProperties();

    if (IsStandardNameScopeOwner())
    {
        if (!m_strName.IsNull() && !ShouldRegisterInParentNamescope())
        {
            IGNOREHR(core->ClearNamedObject(m_strName, this, this));
        }

        IGNOREHR(core->RemoveNameScope(this, Jupiter::NameScoping::NameScopeType::StandardNameScope));
    }

    m_ref_count.end_destroying();

    if (m_bitFields.fIsValidAPointer)
    {
        delete [] m_valid.pointer;
    }

    if (m_bitFields.fParentIsInheritanceContextOnly && m_pMentor != nullptr)
    {
        delete m_pMentor;
        m_pMentor = nullptr;
        m_bitFields.fParentIsInheritanceContextOnly = FALSE;
    }

    ClearExpectedReferenceOnPeer();

    // This will call release on the core if we need to based on the fNeedToReleaseCore flag.
    ContextRelease();

    // Back pointers are not ref-counted to avoid introducing cycles.  If any parent pointer
    // was set on this DO, it should have been removed at some point before the DO was deleted
    // or this object might have been left with a dangling pointer.
    ASSERT(m_pParent == nullptr);
}

// Temporarily exists to facilitate unit tests that don't work with framework peers.
bool CDependencyObject::ShouldReleaseCoreObjectWhenTrackingPeerReference() const
{
    return true;
}

bool CDependencyObject::ShouldFrameworkClearCoreExpectedReference()
{
    // Indicate if the core DO was holding an expected reference to the framework object
    // For multi associated DOs, when OnMultipleAssociationChanged() triggers and the
    // share count drops to 0, only then should we clear the expected reference.
    bool hasMultipleAssociation = (DoesAllowMultipleAssociation() && !IsParentAware() && IsAssociated());

    if (HasExpectedReferenceOnPeer() && !hasMultipleAssociation)
    {
        return true;
    }

    return false;
}

void CDependencyObject::DisconnectManagedPeer()
{
    ClearPeerReferences();
    SetHasManagedPeer(FALSE, FALSE);
}

bool CDependencyObject::GetParticipatesInManagedTreeDefault()
{
    return m_bitFields.fParticipatesInManagedTreeDefault;
}

_Check_return_ HRESULT CDependencyObject::AddParent(
    _In_ CDependencyObject *pNewParent,
    bool fPublic,
    _In_opt_ RENDERCHANGEDPFN pfnParentRenderChangedHandler
    )
{
    RRETURN(SetParent(pNewParent, fPublic, pfnParentRenderChangedHandler));
}

_Check_return_ HRESULT CDependencyObject::RemoveParent(_In_ CDependencyObject *pDO)
{
    (pDO); // Ignore the parameter.
    RRETURN(SetParent(NULL));
}

bool CDependencyObject::ParticipatesInManagedTree()
{
    // Call the virtual to find out if the type wants
    // to participate.  If the answer is "optionally",
    // then return the runtime flag.

    XUINT32 participates = ParticipatesInManagedTreeInternal();

    if( participates == DOESNT_PARTICIPATE_IN_MANAGED_TREE )
        return false;

    else if( participates == OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE )
        return m_bitFields.fParticipatesInManagedTreeDefault;

    else
    {
        ASSERT( participates == PARTICIPATES_IN_MANAGED_TREE );
        return true;
    }
}

// Indicates if the framework peer of this DO has state. If the peer
// has state, this DO should participate in managing the peer's
// lifetime, and protect its peer from GC by setting an expected
// reference on it. If the peer does not have state, this DO need
// not participate and the peer can be released and resurrected
// as needed.
//
// By default, many non-shareable DOs have opt-in participation
// (they don't participate by default, but that can be changed at run-time
// on an object-by-object basis by calling SetParticipatesInManagedTreeDefault).
// Override this method to change the default.
//
// To reduce the number of peers, avoid returning PARTICIPATES_IN_MANAGED_TREE
// for commonly used base types like DependencyObject, UIElement & FrameworkElement.
// Instead, those types should optionally participate, and call
// SetParticipatesInManagedTreeDefault/MarkHasState only when they
// gain state. Less commonly used types can return PARTICIPATES_IN_MANAGED_TREE
// if their peers are typically expected to have state.
//
XUINT32 CDependencyObject::ParticipatesInManagedTreeInternal()
{
    if( IsCustomType() )
        return PARTICIPATES_IN_MANAGED_TREE;
    else if( DoesAllowMultipleAssociation() )
        return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
    else
        return OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE;
}

HRESULT CDependencyObject::UpdatePeerReferenceToProperty(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& newValue,
    _In_ bool bOldValueIsCached,
    _In_ bool bNewValueNeedsCaching,
    _In_opt_ IInspectable* pNewValueOuter,
    _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter
    )
{
    if (bOldValueIsCached || bNewValueNeedsCaching)
    {
        // Make a managed reference from this object to the managed peer
        // of pNewValue, so that that peer isn't GC-ed.  (Note that if this value wasn't
        // a DoesAllowMultipleAssociation, we'd handle this lifetime inside AddParent).
        IFC_RETURN(SetPeerReferenceToProperty(pDP, newValue, /* bPreservePegNoRef */ false, pNewValueOuter, ppOldValueOuter));
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::GetTextFormatting(const TextFormatting **ppTextFormatting)
{
    IFC_RETURN(EnsureTextFormattingForRead());
    *ppTextFormatting = *GetTextFormattingMember();

    return S_OK;
}

void CDependencyObject::DisconnectInheritedProperties()
{
    if (m_pInheritedProperties != NULL)
    {
        m_pInheritedProperties->DisconnectFrom(this);
    }
    m_pInheritedProperties = NULL;
}

_Check_return_ HRESULT CDependencyObject::GetInheritedProperties(const InheritedProperties **ppInheritedProperties)
{
    IFC_RETURN(EnsureInheritedPropertiesForRead());
    *ppInheritedProperties = m_pInheritedProperties;

    return S_OK;
}

bool CDependencyObject::GetUseLayoutRounding() const
{
    return EnterParams::UseLayoutRoundingDefault;
}

bool CDependencyObject::IsFiringEvents()
{
    return IsActive();
}

CDependencyObject* CDependencyObject::GetStandardNameScopeParent()
{
    return GetParentInternal(false);
}

IPALUri* CDependencyObject::GetBaseUri()
{
    IPALUri* preferredBaseURI = GetPreferredBaseUri();

    if (preferredBaseURI)
    {
        AddRefInterface(preferredBaseURI);
        return preferredBaseURI;
    }
    else if (IsParentValid())
    {
        return m_pParent->GetBaseUri();
    }
    else
    {
        return nullptr;
    }
}

void CDependencyObject::SetBaseUri(IPALUri *uri)
{
    const auto& current = m_sharedState->Value();

    if (uri != current.GetBaseUri())
    {
        auto core = GetContext();

        m_sharedState = Flyweight::Create<CDOSharedState>(
            core,
            core,
            current.GetRenderChangedHandler(),
            uri,
            current.GetVisualTree());
    }
}

void CDependencyObject::SetHasUsageName(XUINT32 fHasUsageName)
{
    // Usage name is registered in parent namescope
    ASSERT(ShouldRegisterInParentNamescope());
    m_bitFields.fHasUsageName = fHasUsageName;
}

// Get element's window. This can be different from the main Jupiter window for an element
// in a windowed popup.
HWND CDependencyObject::GetElementInputWindow()
{
    xref_ptr<CPopup> spPopup;
    CUIElement *pUIElementNoRef = do_pointer_cast<CUIElement>(this);

    if (pUIElementNoRef)
    {
        // If the element is in an open popup, get that popup.
        IGNOREHR(CPopupRoot::GetOpenPopupForElement(
            pUIElementNoRef,
            spPopup.ReleaseAndGetAddressOf()));

        // If the popup is windowed, return its window
        if (spPopup && spPopup->IsWindowed())
        {
            return spPopup->GetPopupWindow();
        }

        if (GetContext()->HasXamlIslands())
        {
            // If this element is in a XamlIsland tree, return the HWND for that island
            auto root = GetContext()->GetRootForElement(this);
            auto xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(root);
            if (xamlIslandRoot)
            {
                return xamlIslandRoot->GetInputHWND();
            }
        }
    }

    // Default return value is the Jupiter window
    if (GetContext()->GetHostSite())
    {
        return static_cast<HWND>(GetContext()->GetHostSite()->GetXcpControlWindow());
    }

    return nullptr;
}

HWND CDependencyObject::GetElementPositioningWindow()
{
    xref_ptr<CPopup> spPopup;
    CUIElement *pUIElementNoRef = do_pointer_cast<CUIElement>(this);

    if (pUIElementNoRef)
    {
        // If the element is in an open popup, get that popup.
        IGNOREHR(CPopupRoot::GetOpenPopupForElement(
            pUIElementNoRef,
            spPopup.ReleaseAndGetAddressOf()));

        // If the popup is windowed, return its window
        if (spPopup && spPopup->IsWindowed())
        {
            return spPopup->GetPopupWindow();
        }

        if (GetContext()->HasXamlIslands())
        {
            // If this element is in a XamlIsland tree, return the HWND for that island
            auto root = GetContext()->GetRootForElement(this);
            auto xamlIslandRoot = do_pointer_cast<CXamlIslandRoot>(root);
            if (xamlIslandRoot)
            {
                return xamlIslandRoot->GetPositioningHWND();
            }
        }
    }

    // Default return value is the Jupiter window
    return static_cast<HWND>(GetContext()->GetHostSite()->GetXcpControlWindow());
}

void CDependencyObject::AddRefImpl(UINT32 cRef)
{
    // If we have 2 refs, and we have a managed peer, that means
    // that we now have a native ref, and might need to control the peer's
    // lifetime.  So if this type controls its managed peer lifetime,
    // we'll make the references to the managed peer strong to prevent GC.

    if (cRef == 2 && HasManagedPeer() && ControlsManagedPeerLifetime())
    {
        PegManagedPeerNoRef();
    }
}

void CDependencyObject::ReleaseImpl(UINT32 cRef)
{
    // If the only remaining reference is from the managed peer, then
    // we might need to ensure we're only holding a weak reference to that peer.
    // NOTE: We don't depend on this Unpeg call during shutdown. Instead, we will
    // unpeg right before it disconnects the core object.

    if (1 == cRef && HasManagedPeer())
    {
        // Otherwise, if this type controls its managed peer's lifetime,
        // weaken the reference to the managed peer from the ManagedPeerTable.

        if (ControlsManagedPeerLifetime())
        {
            UnpegManagedPeerNoRef();
        }
    }
    else if (0 == cRef)
    {
        m_ref_count.start_destroying();
        //Reset the parent pointer on property values that are DOs.
        ResetReferencesFromChildren();

        // popup may request that when its ref count goes down to zero, then it should be deleted
        // async. this is because popup lifetime is governed by its IsOpen property. If popup has no other
        // references and its IsOpen property is set to FALSE, then it needs to self-destruct. But it cannot
        // do that from within its instance method (the IsOpen property setter).
        // Hence, it requests that it should be deleted async.
        if (GetTypeIndex() == KnownTypeIndex::Popup && (static_cast<CPopup*>(this))->m_fAsyncQueueOnRelease)
        {
            // increment the ref to 1 so that we don't underflow on final release
            m_ref_count.AddRefDuringDuringDestroy();
            GetContext()->CLR_AsyncReleaseNativeObject(this);
        }
        else
        {
            delete this;
        }
    }
}

HRESULT CDependencyObject::SetAndOriginateError(
    _In_ HRESULT hrToOriginate,
    _In_ ::ErrorType eType,
    _In_ XUINT32 iErrorCode,
    _In_opt_ XUINT32 cParams,
    _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams)
{
    HRESULT hr = S_OK;

    IErrorService *pErrorService = NULL;
    IFC(GetContext()->getErrorService(&pErrorService));

    if(pErrorService)
    {
        IFC(pErrorService->ReportGenericError(hrToOriginate, eType, iErrorCode, 1, 0, 0, pParams, cParams));
    }
    else
    {
        IFC(E_FAIL);
    }

Cleanup:
    // Always return hrToOriginate, so callers can IFC() this function call.
    RRETURN(hrToOriginate);
}

void CDependencyObject::SetParentInternal(
    _In_opt_ CDependencyObject *pNewParent,
    bool fFireChanged,
    _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler /*= NULL */)
{
    RENDERCHANGEDPFN renderChangedHandler = NWGetRenderChangedHandlerInternal();

    // When the parent is removed, propagate the rendering change to it one last time.
    if (fFireChanged && m_pParent && !m_bitFields.fParentIsInheritanceContextOnly && renderChangedHandler)
    {
        NotifyParentChange(m_pParent, renderChangedHandler);
    }

    if (m_bitFields.fParentIsInheritanceContextOnly && m_pMentor)
    {
        delete m_pMentor;
        m_pMentor = nullptr;
    }

    m_pParent = pNewParent;
    m_bitFields.fParentIsInheritanceContextOnly = FALSE;
    NWSetRenderChangedHandlerInternal(pfnNewParentRenderChangedHandler);

    // Notify the new parent of the rendering change.
    if (fFireChanged && pNewParent && pfnNewParentRenderChangedHandler)
    {
        NotifyParentChange(pNewParent, pfnNewParentRenderChangedHandler);
    }
}

// Notify the parent object that this object is being reparented.
void CDependencyObject::NotifyParentChange(
    _In_ CDependencyObject *pNewParent,
    _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
    )
{
    pfnNewParentRenderChangedHandler(pNewParent, DirtyFlags::None);
}

// Returns the name of the class from its metadata
_Check_return_ xstring_ptr CDependencyObject::GetClassName() const
{
    return GetClassInformation()->GetName();
}


//------------------------------------------------------------------------
//
//  Method: GetDebugLabel
//
//  Synopsis:
//      Gets the most readable label for this object.
//
//------------------------------------------------------------------------

_Check_return_ xstring_ptr
CDependencyObject::GetDebugLabel() const
{
    return !m_strName.IsNull() ? m_strName : GetClassName();
}

//------------------------------------------------------------------------
//
//  Method: IsCustomType
//
//  Synopsis:
//      Returns true if the managed peer of this object is not a known type.
//
//------------------------------------------------------------------------
bool CDependencyObject::IsCustomType() const
{
    return !!m_bitFields.fIsCustomType;
}

//------------------------------------------------------------------------
//
//  Method: GetNamescopeOwnerInternal
//
//  Synopsis:
//      Gets the DO that has a Namescope store associated with it.
//
//------------------------------------------------------------------------

CDependencyObject*
CDependencyObject::GetStandardNameScopeOwnerInternal(_In_ CDependencyObject* pFirstOwner)
{
    CDependencyObject *pNamescopeOwner = NULL;
    if (IsStandardNameScopeOwner())
    {
        pNamescopeOwner = this;
    }
    else if (IsActive() && !IsStandardNameScopeMember())
    {
        // get the real root, not the "Dummy Root"
        pNamescopeOwner = GetPublicRootVisual();
    }
    else
    {
        pNamescopeOwner = this->GetStandardNameScopeParent();
        if ( pNamescopeOwner )
        {
            // In some rare cases where we build a cyclic tree outside the live tree we could be
            // stuck in an endless loop here, so prevent that. See bug# 3106
            if ( pNamescopeOwner == pFirstOwner )
            {
                pNamescopeOwner = NULL;
            }
            else
            {
                // get the namescope owner of the namescope parent
                pNamescopeOwner = pNamescopeOwner->GetStandardNameScopeOwnerInternal(pFirstOwner ? pFirstOwner : pNamescopeOwner);
            }
        }
    }

    return pNamescopeOwner;
}

//------------------------------------------------------------------------
//
//  Method: SetName
//
//  Synopsis:
//      Sets this element's name in the correct current namescope, and un-registers
//  the old name.
//  This should only be used in the case that the name is not being Registered
//  in a *new* namescope, but rather the new name is being registered in the
//  current namescope.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::SetName(_In_ const xstring_ptr& strName)
{
    if (!strName.Equals(m_strName))
    {
        CDependencyObject *pNamescopeOwner = NULL;

        if (EventEnabledElementSetNameInfo() && OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            TraceElementSetNameInfo((XUINT64)this, !strName.IsNullOrEmpty() ? strName.GetBuffer() : L"");
        }

        // Named objects in a template never support dynamic name table updates.
        // When a tree is being parsed, the parser puts the name/value pairs into the name scope table.
        // Named objects outside of a template and outside of parsing need to update the name scope table.
        if (!IsTemplateNamescopeMember() && !ParserOwnsParent())
        {
            CDependencyObject *pNamescopeStartingPoint = this;

            //
            // Find the correct NamescopeOwner.
            //
            if (IsStandardNameScopeOwner() && ShouldRegisterInParentNamescope())
            {
                //
                // Elements that were initialized from xaml will have their own namescope,
                // but after initialization, their names should be registered in the containing
                // namescope.
                //
                // If we find ourselves in the position that we are trying to modify the name
                // of such an element, we will have to take one extra step in finding the namescope
                // owner:  start with the namescope-parent.
                //
                pNamescopeStartingPoint = GetStandardNameScopeParent();
            }

            if (pNamescopeStartingPoint)
            {
                pNamescopeOwner = pNamescopeStartingPoint->GetStandardNameScopeOwner();
            }

        }

        auto core = GetContext();

        // During parse, the parser sets names.  But it's still possible for an app to be setting names during the parse.
        // (This isn't desired behavior, but it's behavior that's shipped.)  If that's the case here, we'll use the namescope
        // owner that was stored away by the parser to allow for it.
        if( !pNamescopeOwner && ParserOwnsParent() && core->IsSettingValueFromManaged(this))
        {
            pNamescopeOwner = core->GetParserNamescopeOwner();
        }

        xstring_ptr strOldName;

        ASSERT(strOldName.IsNullOrEmpty());
        std::swap(strOldName, m_strName);

        if (pNamescopeOwner)
        {
            // For DependencyObject.NameProperty, it is necessary to unregister the old name
            // prior to setting the new value.
            IFC_RETURN(UnregisterName(pNamescopeOwner));

            m_strName = strName;

            HRESULT hrRegister = RegisterName(pNamescopeOwner);
            if (FAILED(hrRegister))
            {
                std::swap(m_strName, strOldName);

                // best effort at re-registering the old name.
                IGNOREHR(RegisterName(pNamescopeOwner));
                IFC_RETURN(hrRegister);
            }
        }
        else
        {
            // since there is no NamescopeOwner, just replace the old name
            // with the new.
            m_strName = strName;
        }
    }

    // Any name set after initial definition is a usage name.
    if (ShouldRegisterInParentNamescope() )
    {
        SetHasUsageName(TRUE);
    }

    return S_OK;
}



// Registers this element's name with the namescope of the passed NamescopeOwner
_Check_return_ HRESULT CDependencyObject::RegisterName( _In_ CDependencyObject *pNamescopeOwner, XUINT32 bTemplateNamescope)
{
    if (pNamescopeOwner == this && ShouldRegisterInParentNamescope())
    {
        return S_OK;
    }

    ASSERT(bTemplateNamescope == (XUINT32)IsTemplateNamescopeMember());
    auto core = GetContext();

    if (!m_strName.IsNull() &&
        pNamescopeOwner &&

        // During parse, the parser handles name registration, except when the app actually calls put_Name
        (!ParserOwnsParent() || core->IsSettingValueFromManaged(this)) &&

        // Elements which should register in the parent namescope, should
        // register only the usage name, not definition name, because
        // the definition name should be in the element's namescope.
        (!ShouldRegisterInParentNamescope() || HasUsageName()))
    {
        HRESULT xr = core->SetNamedObject(m_strName,
                                          pNamescopeOwner,
                                          bTemplateNamescope ?
                                              Jupiter::NameScoping::NameScopeType::TemplateNameScope :
                                              Jupiter::NameScoping::NameScopeType::StandardNameScope,
                                          this);

        if (FAILED(xr))
        {
            xephemeral_string_ptr parameters[1];
            m_strName.Demote(&parameters[0]);
            IGNOREHR(SetAndOriginateError(xr, ParserError, AG_E_PARSER_DUPLICATE_NAME, 1, parameters));
        }

        IFC_RETURN(xr);
    }

    return S_OK;
}

_Check_return_ HRESULT
CDependencyObject::UnregisterName( _In_ CDependencyObject *pNamescopeOwner)
{
    // This condition will be reached from LeaveImpl after we set the adjusted namescope
    // member to ourselves. Because we've already registered ourselves in our own namescope
    // table at definition time and because we can't ever change that registration we simple
    // do nothing.
    if (pNamescopeOwner == this && ShouldRegisterInParentNamescope()) return S_OK;

    ASSERT(!IsTemplateNamescopeMember());

    // If we've set a usage name on top of the definition name then we do indeed want to Unregister here.
    if (!m_strName.IsNull() && pNamescopeOwner && ShouldParticipateInParentNameScope())
    {
        // Try to un-register the name.  We ignore any errors, because there are some known scenarios where the
        // name won't have been registered (see bug 15737).  The risk we take by ignoring the error
        // is that it might actually be an indication of a problem, and won't get reported until
        // later at a less debuggable point.  But by ignoring it we prevent valid scenarios from
        // turning into errors.
        IGNOREHR(GetContext()->ClearNamedObject(m_strName, pNamescopeOwner, this));
    }
    return S_OK;
}

bool CDependencyObject::IsSettingValueFromManaged(_In_ CDependencyObject* obj) const
{
    return !!GetContext()->IsSettingValueFromManaged(obj);
}

_Check_return_ HRESULT CDependencyObject::TryReCreateUIAWrapper()
{
    // It's possible that a CUIAWrapper was already created for this element, before we determined the correct tree to place the element. Check
    // for this case and invalidate the CUIAWrapper if that's the case.
    if (CAutomationPeer* currentAP = GetAutomationPeer())
    {
        if (auto wrapper = currentAP->GetUIAWrapper())
        {
            if (VisualTree* visualTree = GetVisualTree())
            {
                CUIAWindow* oldUIAWindow = static_cast<CUIAWrapper*>(wrapper)->GetUIAWindow();
                CUIAWindow* uiaWindow = GetContext()->GetUIAWindowForElementRootNoRef(visualTree->GetRootElementNoRef());

                // This indicates that the UIAWindows are different. Invalidate the current UIAWrapper and create a new one with the correct UIAWindow
                if (oldUIAWindow != uiaWindow)
                {
                     currentAP->InvalidateUIAWrapper();
                     wrapper->Invalidate();

                     if (uiaWindow)
                     {
                         xref_ptr<CUIAWrapper> spWrapper;
                         IFC_RETURN(uiaWindow->CreateProviderForAP(currentAP, spWrapper.ReleaseAndGetAddressOf()));
                     }
                }
            }
        }
    }

    return S_OK;
}

// Causes the object and its "children" to enter scope. If bLive,
// then the object can now respond to OM requests and perform actions
// like downloads and animation.
_Check_return_ HRESULT CDependencyObject::Enter(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    // If IsProcessingEnterLeave is true, then this element is already part of the
    // Enter/Leave walk. This can happen, for instance, if a custom DP's value has
    // been set to some ancestor of this node.
    if (IsProcessingEnterLeave())
    {
        return S_OK;
    }
    else
    {
        SetIsProcessingEnterLeave(TRUE);
    }

    auto guard = wil::scope_exit([this]()
    {
        SetIsProcessingEnterLeave(FALSE);
    });

    if (CXamlIslandRoot* xamlIsland = do_pointer_cast<CXamlIslandRoot>(this))
    {
        // The CXamlIslandRoot can enter the tree in a few different ways, and we need to make sure
        // that however it enters, we override the params.visualTree with the one from the CXamlIslandRoot.
        // CXamlIslandRoot always defines its own visual tree, so we must set it here.  Note that after
        // the tree refactoring, this won't be necessary because the XamlIslandRoot won't be in the tree
        // anymore.
        params.visualTree = xamlIsland->GetVisualTreeNoRef();
    }

    if (params.fIsLive && params.visualTree)
    {
        // As the DO enters the live tree, we call SetVisualTree to remember which one it's associated with
        SetVisualTree(params.visualTree);
    }

    CDependencyObject *pAdjustedNamescopeOwner = pNamescopeOwner;

    // When we copy the EnterParams, reset the pointer to the resource dictionary
    // parent so that descendants don't think they are the direct child of one.
    EnterParams enterParams(params);
    enterParams.pParentResourceDictionary = nullptr;

    if (m_pInheritedProperties != NULL)
    {
        if (m_pInheritedProperties->m_pWriter == this)
        {
            // This DO owns this m_pInheritedProperties. Mark it as out of date
            // so that subsequent property accesses get updated property values.
            m_pInheritedProperties->m_cGenerationCounter = 0;
        }
        else
        {
            // This DO's inherited properties are a read only reference to some
            // parent, that reference is no longer valid, and we need to release
            // it.
            DisconnectInheritedProperties();
        }
    }

    if (this->IsStandardNameScopeOwner())
    {
        pAdjustedNamescopeOwner = this;

        // If we are entering some other scope
        if (pAdjustedNamescopeOwner != pNamescopeOwner)
        {
            // If this is a permanent namescopeOwner, but its names are not registered
            // in its own namescope, then the optimization further down about skipping name registration
            // wouldn't apply.
            if (!params.fSkipNameRegistration &&
                this->ShouldRegisterInParentNamescope() &&
                pNamescopeOwner != this &&
                !IsTemplateNamescopeMember())
            {
                // not using the "adjusted" namescope owner
                IFC_RETURN(RegisterName(pNamescopeOwner));
            }

            // regarding condition below: The only element that is a Permanent
            // Namescope owner, but not a Namescope member is the Root Visual;
            // and it isn't necessary to try to defer name registration for the root visual.
            if (this->IsStandardNameScopeMember() && GetContext()->HasRegisteredNames(this))
            {
                ASSERT(this->IsStandardNameScopeOwner());

                if (params.fIsLive)
                {
                    if (!IsActive())
                    {
                        IFC_RETURN(TryReCreateUIAWrapper());
                    }

                    // pass TRUE for bSkipRegistration:  The names have already been
                    // registered, and being a Permanent Namescope Owner, we aren't
                    // expected to have to merge Namescopes with a parent Namescope.
                    enterParams.fSkipNameRegistration = TRUE;
                    IFC_RETURN(this->EnterImpl(pAdjustedNamescopeOwner, enterParams));
                    return S_OK;
                }
                else
                {
                    // Skipping the non-live Enter walk here, as it should only be propagating
                    // name information, which we already have. This is kind of an odd part of
                    // the non-live enter. You can never rely on anything except your direct parent
                    // remaining unchanged because this optimization will terminate non-live Enters
                    // at NameScope boundaries when manipulating XAML fragments.
                    return S_OK;
                }
            }
        }
    }

    if(params.fSkipNameRegistration && do_pointer_cast<CPopupRoot>(GetParentInternal()))
    {
        // Popup's child receives Enter from two different namescopes - namescope of its logical
        // parent and that of its visual parent. This Enter is from the visual parent of popup's child.
        // It should have a valid namescope owner by now since name registration is done
        // before the popup is opened. Use the namescope in which name registration is done
        // to ensure the correct IsNamescopeMember flag.
        pAdjustedNamescopeOwner = GetStandardNameScopeOwner();
        ASSERT(pAdjustedNamescopeOwner);

        if (CUIElement* uiElementThis = do_pointer_cast<CUIElement>(this))
        {
            if (CPopup* popup = do_pointer_cast<CPopup>(uiElementThis->GetLogicalParentNoRef()))
            {
                // See comment in CDependencyObject::Leave
                popup->SetCachedStandardNamescopeOwner(pAdjustedNamescopeOwner);
            }
        }
    }

    // [Blue Compat]: When parsing App.xaml, skip entering any tree children of the Application object.
    bool isNamescopeOwnerApplicationDuringParse = pNamescopeOwner && pNamescopeOwner->ParserOwnsParent() && pNamescopeOwner->OfTypeByIndex<KnownTypeIndex::Application>();

    // MultiParentShareableDependencyObjects may not have a namescope owner (e.g. when it has multiple parents). But we
    // still need to make sure we do a live enter, so that types such as BitmapImage can still do work upon entering/
    // leaving the tree.
    bool liveEnterOnMultiParentShareableDO = params.fIsLive && DoesAllowMultipleParents();

    if ((pAdjustedNamescopeOwner || liveEnterOnMultiParentShareableDO)
        && !isNamescopeOwnerApplicationDuringParse)
    {
        if (params.fIsLive && !IsActive())
        {
            IFC_RETURN(TryReCreateUIAWrapper());
        }

        if (pAdjustedNamescopeOwner)
        {
            SetIsStandardNameScopeMember(pAdjustedNamescopeOwner->IsStandardNameScopeMember());
        }
        IFC_RETURN(EnterImpl(pAdjustedNamescopeOwner, params));
    }
    else if (params.fIsForKeyboardAccelerator)
    {
        // This is dead enter to register any keyboard accelerators collection to the list of live accelerators
        params.fIsLive = FALSE;
        params.fSkipNameRegistration = TRUE;
        params.fUseLayoutRounding = FALSE;
        params.fCoercedIsEnabled = FALSE;
        IFC_RETURN(EnterImpl(pAdjustedNamescopeOwner, params));
    }

    return S_OK;
}

// Causes the object and its "children" to enter scope. If bLive,
// then the object can now respond to OM requests and perform actions
// like downloads and animation.
//
// Derived classes are expected to first call <base>::EnterImpl, and
// then call Enter on any "children".
_Check_return_ HRESULT CDependencyObject::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    // Mark the object as in the live tree
    // Enter cannot make a Live object non-live
    // that can only happen in Leave.
    if (params.fIsLive)
    {
        if (!IsActive())
        {
            ActivateImpl();
        }

        m_checkForResourceOverrides = !!params.fCheckForResourceOverrides;
    }

    if (!params.fSkipNameRegistration)
    {
        if (!IsTemplateNamescopeMember())
        {
            IFC_RETURN(RegisterName(pNamescopeOwner));
            RegisterDeferredStandardNameScopeEntries(pNamescopeOwner);
        }
    }

    if (HasDeferred())
    {
        IFC_RETURN(CDeferredMapping::NotifyEnter(
            pNamescopeOwner,
            this,
            params.fSkipNameRegistration));
    }

    // Nothing else to do for value types and control/data templates.
    const CClassInfo* pClassInfo = GetClassInformation();

    if (pClassInfo->IsValueType()
        || pClassInfo->m_nIndex == KnownTypeIndex::ControlTemplate
        || pClassInfo->m_nIndex == KnownTypeIndex::DataTemplate)
    {
        return S_OK;
    }

    // Enumerate all the field-backed properties and enter/invoke as needed.
    const CEnterDependencyProperty* pNullEnterProperty = MetadataAPI::GetNullEnterProperty();
    for (const CEnterDependencyProperty* pEnterProperty = pClassInfo->GetFirstEnterProperty(); pEnterProperty != pNullEnterProperty; pEnterProperty = pEnterProperty->GetNextProperty())
    {
        if (!pEnterProperty->DoNotEnterLeave())
        {
            if (pEnterProperty->IsObjectProperty())
            {
                CDependencyObject *pDO = MapPropertyAndGroupOffsetToDO(pEnterProperty->m_nOffset, pEnterProperty->m_nGroupOffset);
                if (pDO != nullptr)
                {
                    IFC_RETURN(EnterObjectProperty(pDO, pNamescopeOwner, params));
                }
            }
        }
        if (pEnterProperty->NeedsInvoke())
        {
            IFC_RETURN(Invoke(MetadataAPI::GetDependencyPropertyByIndex(pEnterProperty->m_nPropertyIndex), pNamescopeOwner, params.fIsLive));
        }
    }

    IFC_RETURN(EnterSparseProperties(pNamescopeOwner, params));

    if (params.fIsLive && m_bitFields.fWantsInheritanceContextChanged)
    {
        // We only raise this InheritanceContextChanged if we're entering the live tree because the
        // event also acts like a DO.Loaded event for BindingExpression.  This keeps us from adding
        // a new internal event
        IFC_RETURN(NotifyInheritanceContextChanged());
    }

    if (m_bitFields.fLive)
    {
        // If our theme is different from the parent, make sure we walk the subtree.
        CDependencyObject* pParent = nullptr;
        auto thisAsFe = do_pointer_cast<CFrameworkElement>(this);

        if (thisAsFe)
        {
            // Get logical parent so popups and flyouts inherit theme changes
            pParent = GetInheritanceParentInternal(TRUE /* fLogicalParent */);
        }
        else
        {
            pParent = GetParentInternal(false /* public */);
        }
        
        if (pParent && pParent->GetTheme() != Theming::Theme::None && pParent->GetTheme() != m_theme)
        {
            IFC_RETURN(NotifyThemeChanged(pParent->GetTheme()));
        }
        else
        {
            // Update theme references to account for new ancestor theme dictionaries.
            IFC_RETURN(UpdateAllThemeReferences());
        }
    }

    return S_OK;
}


// Causes the object and its properties to leave scope. If bLive,
// then the object is leaving the "Live" tree, and the object can no
// longer respond to OM requests related to being Live.   Actions
// like downloads and animation will be halted.
_Check_return_ HRESULT CDependencyObject::Leave(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    // If IsProcessingEnterLeave is true, then this element is already part of the
    // Enter/Leave walk.  This can happen, for instance, if a custom DP's value has
    // been set to some ancestor of this node.
    if (IsProcessingEnterLeave())
    {
        return S_OK;
    }
    else
    {
        SetIsProcessingEnterLeave(TRUE);
    }

    auto guard = wil::scope_exit([this]()
    {
        SetIsProcessingEnterLeave(FALSE);
    });

    CDependencyObject *pAdjustedNamescopeOwner = pNamescopeOwner;

    // When we copy the LeaveParams, reset the pointer to the resource dictionary
    // parent so that descendants don't think they are the direct child of one.
    LeaveParams leaveParams(params);
    leaveParams.pParentResourceDictionary = nullptr;

    // It only makes sense to leave a live tree if you are currently live.
    // If this happens, we are likely recovering from a situation where this
    // tree has only partially entered the live tree.
    const bool bAdjustedLive = params.fIsLive && IsActive();

    if (m_pInheritedProperties != NULL)
    {
        if (m_pInheritedProperties->m_pWriter == this)
        {
            // This DO owns this m_pInheritedProperties. Mark it as out of date
            // so that subsequent property accesses get updated property values.
            m_pInheritedProperties->m_cGenerationCounter = 0;
        }
        else
        {
            // This DO's inherited properties are a read only reference to some
            // parent, that reference is no longer valid, and we need to release
            // it.
            DisconnectInheritedProperties();
        }
    }

    if (this->IsStandardNameScopeOwner())
    {
        pAdjustedNamescopeOwner = this;

        // If this is a permanent namescopeOwner, but its names are not registered
        // in its own namescope, then the optimization further down about skipping name registration
        // wouldn't apply.
        if (!params.fSkipNameRegistration && this->ShouldRegisterInParentNamescope() && pNamescopeOwner != this &&
            !IsTemplateNamescopeMember())
        {
            // not using the "adjusted" namescope owner
            IFC_RETURN(UnregisterName(pNamescopeOwner));
        }

        if (HasDeferred())
        {
            // Either of code-paths below will leave the method, so NotifyLeave should not be called twice
            IFC_RETURN(CDeferredMapping::NotifyLeave(
                pNamescopeOwner,
                this,
                params.fSkipNameRegistration));
        }

        if (!bAdjustedLive && !GetContext()->HasRegisteredNames(this))
        {
            // If we are removing a Namescope Owner from a non-live tree,
            // and it hasn't had gone through registration of the names, then
            // there is nothing to clean up.  Score.
            return S_OK;
        }
        else
        {
            // ensure that the fNamescopeMember is set, (as this may not be
            // the case if this is the rootVisual leaving the tree)
            SetIsStandardNameScopeMember(TRUE);

            // If this is a permanent NamescopeOwner, then he will be taking his
            // names with him en masse, so pass TRUE for bSkipRegistration
            leaveParams.fIsLive = bAdjustedLive;
            leaveParams.fSkipNameRegistration = TRUE;

            IFC_RETURN(LeaveImpl(pAdjustedNamescopeOwner, leaveParams));
            return S_OK;
        }
    }

    if (params.fSkipNameRegistration
        && NULL != do_pointer_cast<CPopupRoot>(GetParentInternal()))
    {
        // Popup's child receives Leave from two different namescopes - namescope of its logical
        // parent and that of its visual parent. This Leave is from the visual parent of popup's child.
        // Use the namescope in which name registration is done to ensure the correct
        // IsNamescopeMember flag.
        pAdjustedNamescopeOwner = GetStandardNameScopeOwner();

        if (pAdjustedNamescopeOwner == nullptr && IsActive())
        {
            if (CUIElement* uiElementThis = do_pointer_cast<CUIElement>(this))
            {
                if (CPopup* popup = do_pointer_cast<CPopup>(uiElementThis->GetLogicalParentNoRef()))
                {
                    // This is a temporary fix for the case where a Popup's child is leaving the tree, but we can't find
                    // the namescope owner (because the owner isn't live anymore, so GetStandardNameScopeOwnerInternal
                    // skips it).  We hit this case in the CommandBarFlyoutCommandBar, which contains a parented popup
                    // called "OverflowPopup".  When the flyout closes, the children begin the process of leaving the
                    // tree.  As part of this process, the OverflowPopup is closed, and as its children process Leave,
                    // pAdjustedNamescopeOwner comes back as nullptr here because the namescope owner has already left
                    // the "live" state.  We've made the change this way to minimize risk for a WinAppSDK 1.0 update.
                    // An easy way to repro this case is to right-click a TextBox in two different XAML Windows on
                    // the same thread.
                    // Find a better, more comprehensive fix with http://osgvsowi/19548424
                    pAdjustedNamescopeOwner = popup->GetCachedStandardNamescopeOwnerNoRef();
                    popup->SetCachedStandardNamescopeOwner(nullptr);
                }
            }
        }
    }

    // MultiParentShareableDependencyObjects may not have a namescope owner (e.g. when it has multiple parents). But we
    // still need to make sure we do a live enter, so that types such as BitmapImage can still do work upon entering/
    // leaving the tree.
    bool liveLeaveOnMultiParentShareableDO = bAdjustedLive && DoesAllowMultipleParents();
    if (pAdjustedNamescopeOwner || liveLeaveOnMultiParentShareableDO)
    {
        if (pAdjustedNamescopeOwner)
        {
            SetIsStandardNameScopeMember(pAdjustedNamescopeOwner->IsStandardNameScopeMember());
        }
        leaveParams.fIsLive = bAdjustedLive;
        leaveParams.fSkipNameRegistration = params.fSkipNameRegistration;
        IFC_RETURN(LeaveImpl(pAdjustedNamescopeOwner, leaveParams));
    }
    else if (leaveParams.fIsForKeyboardAccelerator)
    {
        // This is dead leave to register any keyboard accelerators collection to the list of live accelerators
        leaveParams.fIsLive = FALSE;
        leaveParams.fSkipNameRegistration = TRUE;
        leaveParams.fUseLayoutRounding = FALSE;
        leaveParams.fCoercedIsEnabled = FALSE;
        IFC_RETURN(LeaveImpl(pAdjustedNamescopeOwner, leaveParams));
    }

    if (HasDeferred())
    {
        IFC_RETURN(CDeferredMapping::NotifyLeave(
            pNamescopeOwner,
            this,
            params.fSkipNameRegistration));
    }

    return S_OK;
}

// Causes the object and its properties to leave scope. If bLive,
// then the object is leaving the "Live" tree, and the object can no
// longer respond to OM requests related to being Live.   Actions
// like downloads and animation will be halted.
//
// Derived classes are expected to first call <base>::LeaveImpl, and
// then call Leave on any "children".
//
// Objects are expected to cleanup all the unshared device resources on their leave from live tree
// i.e., when params.fIsLive = TRUE. This include primitives,
// composition nodes, visuals, shape realizations and cache realizations for UIElements.
// And it includes textures etc, for other objects like brushes and image sources.
// The recursive leave call for children elements and children properties
// would do similar cleanup on their final leave. This enables appropriate sharing.
// Hence an element should not cleanup resources for its
// child/property in its leave.
_Check_return_ HRESULT CDependencyObject::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    // Raise InheritanceContextChanged for the live leave.  We need to do this before m_bitFields.fLive is updated.
    // params.fIsLive cannot be used because it is updated before we get here.
    if (m_bitFields.fLive && m_bitFields.fWantsInheritanceContextChanged)
    {
        IFC_RETURN(NotifyInheritanceContextChanged());
    }

    // Mark the object as out of tree if the intention of this walk is to notify
    // the element that it is leaving the live tree (as indicated by the bLive parameter.)
    if (params.fIsLive)
    {
        m_checkForResourceOverrides = !!params.fCheckForResourceOverrides;
        DeactivateImpl();
    }

    // Enumerate all the properties in its class

    if (!params.fSkipNameRegistration &&
        !IsTemplateNamescopeMember())
    {
        IFC_RETURN(UnregisterName(pNamescopeOwner));
        UnregisterDeferredStandardNameScopeEntries(pNamescopeOwner);
    }

    const CClassInfo* pClassInfo = GetClassInformation();

    // Nothing else to do for value types and control/data templates.
    if (pClassInfo->IsValueType()
        || pClassInfo->m_nIndex == KnownTypeIndex::ControlTemplate
        || pClassInfo->m_nIndex == KnownTypeIndex::DataTemplate)
    {
        return S_OK;
    }

    // Enumerate all the field-backed properties and leave as needed.
    const CEnterDependencyProperty* pNullEnterProperty = MetadataAPI::GetNullEnterProperty();
    for (const CEnterDependencyProperty* pEnterProperty = pClassInfo->GetFirstEnterProperty(); pEnterProperty != pNullEnterProperty; pEnterProperty = pEnterProperty->GetNextProperty())
    {
        if (pEnterProperty->DoNotEnterLeave())
        {
            continue;
        }

        if (pEnterProperty->IsObjectProperty())
        {
            CDependencyObject *pDO = MapPropertyAndGroupOffsetToDO(pEnterProperty->m_nOffset, pEnterProperty->m_nGroupOffset);
            if (pDO != nullptr)
            {
                IFC_RETURN(LeaveObjectProperty(pDO, pNamescopeOwner, params));
            }
        }
    }

    IFC_RETURN(LeaveSparseProperties(pNamescopeOwner, params));
     if (params.fIsLive)
     {
        // If we're currently the focused element, remove ourselves from being focused
        const auto contentRoot = VisualTree::GetContentRootForElement(this, LookupOptions::NoFallback);
        if (contentRoot != nullptr)
        {
            CFocusManager * pFocusManager = contentRoot->GetFocusManagerNoRef();

            if (pFocusManager && pFocusManager->GetFocusedElementNoRef() == this)
            {
                pFocusManager->ClearFocus();
            }

            const auto& akExport = contentRoot->GetAKExport();

            if (akExport.IsActive())
            {
                IFC_RETURN(akExport.RemoveElementFromAKMode(this));
            }
        }
     }

    CInputServices *inputServices = GetContext()->GetInputServices();

    if (inputServices != nullptr)
    {
        IFC_RETURN(inputServices->ObjectLeavingTree(this));
    }

    return S_OK;
}

// Activate side-effects of a given property.
_Check_return_ HRESULT CDependencyObject::InvokeImpl(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner)
{
    HRESULT hr = S_OK;

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::Glyphs_FontUri:
        hr = (((CGlyphs *) this)->EnsureFontFace());
        // E_PENDING means we found the font just fine, it's just not ready
        // yet. Here we don't need it to be ready.
        if (hr == E_PENDING)
        {
            hr = S_OK;
        }
        IFC(hr);
        break;
    }

Cleanup:
    return hr;
}

// The virtual method which does the tree walk to clean up all
// the device related resources like brushes, textures,
// primitive composition data etc. in this subgraph.
void CDependencyObject::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    // If IsProcessingDeviceRelatedCleanup is true, then this element is already part of the
    // cleanup walk.  This can happen, for instance, if a custom DP's value has
    // been set to some ancestor of this node.
    if (IsProcessingDeviceRelatedCleanup())
    {
       return;
    }

    if (cleanupDComp)
    {
        SetIsProcessingDeviceRelatedCleanup(true);
        auto cleanupGuard = wil::scope_exit([this]{ SetIsProcessingDeviceRelatedCleanup(false); });

        ReleaseDCompResources();
    }

    auto pClassInfo = GetClassInformation();

    // Enumerate all the render properties and cleanup as needed.
    const CRenderDependencyProperty* pNullRenderProperty = MetadataAPI::GetNullRenderProperty();
    for (const CRenderDependencyProperty* pRenderProperty = pClassInfo->GetFirstRenderProperty(); pRenderProperty != pNullRenderProperty; pRenderProperty = pRenderProperty->GetNextProperty())
    {
        CDependencyObject *pDO = MapPropertyAndGroupOffsetToDO(pRenderProperty->m_nOffset, pRenderProperty->m_nGroupOffset);
        if (pDO)
        {
            pDO->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
        }
    }
    if (m_pValueTable)
    {
        // To protect against reentrancy, copy into a temporary vector before iterating.
        Jupiter::arena<DefaultSparseArenaSize> localArena;
        auto tempValues = GetSparseValueEntries(localArena);

        for (auto& entry : tempValues)
        {
            auto pDO = entry.second.value.AsObject();
            if (pDO && MetadataAPI::GetDependencyPropertyByIndex(entry.first)->IsVisualTreeProperty())
            {
                pDO->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
            }
        }
    }
}

//------------------------------------------------------------------------
//
//  Method: IsAmbientPropertyValueAvailable
//
//  Synopsis:
//
//      ObjectWriterContext::FindNextAmbientValue will attempt to
//      find instances of ambient properties. Typically this is
//      Resources or TargetType.
//
//      A direct GetValue() call will fault in on demand properties
//      resulting in unnecessary object creation throughout the tree.
//
//      FindNextAmbientValue will call this method to ensure the GetValue
//      doesn't fault in on demand properties.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CDependencyObject::IsAmbientPropertyValueAvailable(
    _In_  const CDependencyProperty *pdp,
    _Out_ bool               *pbAvailable
)
{
    ASSERT(pdp);

    if (pdp->IsInheritedAttachedPropertyInStorageGroup() ||
        (pdp->IsInherited() && !pdp->IsStorageGroup()))
    {
        // unexpected as ambient properties shouldn't cause this path to trigger
        ASSERT(false);
        IFC_RETURN(E_FAIL);
    }

    if (pdp->IsSparse())
    {
        // If an on-demand property hasn't been brought into existence yet, it's not available
        // Non-on-demand properties are always available
        *pbAvailable = !pdp->IsOnDemandProperty() ||
            (m_pValueTable && (m_pValueTable->find(pdp->GetIndex()) != m_pValueTable->end()));
    }
    else
    {
        *pbAvailable = true;
        // Prop method calls are always available, they aren't able to be on-demand
        if (!pdp->IsPropMethodCall())
        {
            XHANDLE pOffset = GetPropertyOffset(pdp, /* forGetValue */ true);
            IFCPTR_RETURN(pOffset);

            if (pdp->GetStorageType() == valueObject && pdp->IsOnDemandProperty())
            {
                CDependencyObject* pObjectTemp = *((CDependencyObject**)pOffset);
                if (!pObjectTemp)
                {
                    *pbAvailable = false;
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::GetDefaultInheritedPropertyValue(_In_ const  CDependencyProperty* dp, _Out_ CValue* value)
{
    IFC_RETURN(GetContext()->GetDefaultInheritedPropertyValue(dp->GetIndex(), value));
    return S_OK;
}

// Store theme reference for a property.
void CDependencyObject::SetThemeResource(
    _In_ const CDependencyProperty *pdp,
    _In_ CThemeResource* pThemeResource)
{
    ThemeResourceMap& themeResourceMap = EnsureThemeResourcesStorage();

#ifdef DEBUG
    // Caller must ensure that any previous theme resource extension set to this property
    // was removed.
    auto it = themeResourceMap.find(pdp->GetIndex());
    ASSERT(it == themeResourceMap.end());
#endif

    themeResourceMap.emplace(pdp->GetIndex(), xref_ptr<CThemeResource>(pThemeResource));
}

// Get theme reference for a property.
CThemeResource* CDependencyObject::GetThemeResourceNoRef(_In_ KnownPropertyIndex ePropertyIndex)
{
    ThemeResourceMap* themeResourceMap = GetThemeResourcesStorage();

    if (themeResourceMap)
    {
        auto it = themeResourceMap->find(ePropertyIndex);
        return it == themeResourceMap->end() ? nullptr : it->second.get();
    }

    return nullptr;
}

// Clears theme reference for a property.
void CDependencyObject::ClearThemeResource(_In_ const CDependencyProperty *pdp, _Out_opt_ xref_ptr<CThemeResource>* themeResource)
{
    if (themeResource)
    {
        themeResource->reset();
    }

    ThemeResourceMap* themeResourceMap = GetThemeResourcesStorage();

    if (themeResourceMap)
    {
        auto it = themeResourceMap->find(pdp->GetIndex());

        if (it != themeResourceMap->end())
        {
            ASSERT(it->second != nullptr);

            if (themeResource)
            {
                *themeResource = std::move(it->second);
            }

            themeResourceMap->erase(it);
        }
    }
}

// Clears all the theme references.
void CDependencyObject::ClearThemeResources()
{
    ThemeResourceMap* themeResourceMap = GetThemeResourcesStorage();

    if (themeResourceMap)
    {
        themeResourceMap->clear();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CDependencyObject::ClonePropertySetField
//
//  Synopsis:
//      Copy PropertySet field.
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::ClonePropertySetField(
    _In_ const CDependencyObject *pdoOriginal)
{
    // Original and cloned object must be of same type
    IFCEXPECT_RETURN(GetTypeIndex() == pdoOriginal->GetTypeIndex());

    if (pdoOriginal->m_bitFields.fIsValidAPointer)
    {
        // Count of properties for this class exceeds 32, so
        // m_valid is a pointer to a bit array. Clone the array.

        // Clone should not yet have a bit array in m_valid
        ASSERT(!m_bitFields.fIsValidAPointer);

        XINT32 uSize = (MetadataAPI::GetPropertySlotCount(GetTypeIndex()) + 31) / 32;

        m_valid.pointer = new XUINT32[uSize];
        memcpy(m_valid.pointer, pdoOriginal->m_valid.pointer,
                    uSize * sizeof(XUINT32));

        m_bitFields.fIsValidAPointer = TRUE;
    }
    else
    {
        m_valid = pdoOriginal->m_valid;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT CDependencyObject::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue
    )
{
    RRETURN(E_NOTIMPL);
}

// Gets the root parent of an element or in the case that the parent is nullptr returns itself, since it is the root.
CDependencyObject* CDependencyObject::GetTreeRoot(_In_ bool publicParentOnly /* default = FALSE */)
{
    CDependencyObject * pBase = this;
    CDependencyObject * pParent = pBase->GetParentInternal(publicParentOnly);
    bool parentIsRootVisual = false;
    auto core = GetContext();

    if (pParent)
    {
        VERIFYHR(core->IsObjectAnActiveRootVisual(pParent, &parentIsRootVisual));
    }
    while (pParent && (!publicParentOnly || !parentIsRootVisual))
    {
        pBase = pParent;
        pParent = pParent->GetParentInternal(publicParentOnly);
        if (pParent)
        {
            VERIFYHR(core->IsObjectAnActiveRootVisual(pParent, &parentIsRootVisual));
        }
    }
    return pBase;
}


//-------------------------------------------------------------------------------------
//
//  When this object gets a new parent, an expected reference needs to be added to the
//  peer for GC lifetime management, if the peer is stateful. Also, the peer needs
//  to be notified of the new parent. Even if there is no peer, descendants need
//  to be notified of the DataContext change.
//
//  Details about why an expected reference is added to the peer:
//  A peer keeps a ref on the corresponding CDO when the peer is created -- see
//  DependencyObject::Initialize's call to DependencyObject_AddRef(GetHandle()).
//  Initially the CDO does not keep a ref on its peer. As long as the app has a ref
//  on the peer, the peer keeps its ref on the CDO. If the app puts the DO in the tree
//  and releases its ref to the DO, the CDO & stateful peer need to be kept alive because
//  they are in the tree. That's why CDO sets/clears expected ref on the stateful peer when
//  it enters/leaves the tree. The CDO is kept alive by the tree, and the peer is kept
//  alive by the CDO's expected ref. If CDO didn't have this expected ref on the peer,
//  the peer would be deleted when the app released its reference. CDO needs to set the
//  expected ref only on a stateful peer because stateless peers can be re-created when
//  needed.
//
//-------------------------------------------------------------------------------------
_Check_return_ HRESULT
CDependencyObject::OnParentChange(
    _In_ CDependencyObject *pOldParent,
    _In_ CDependencyObject *pParent,
    _In_ bool hasAtLeastOneParent)
{
    bool parentIsRootVisual = false;
    bool oldParentIsRootVisual = false;
    auto core = GetContext();

    if (pParent != nullptr)
    {
        IFC_RETURN(core->IsObjectAnActiveRootVisual(pParent, &parentIsRootVisual));
    }

    if (pOldParent != nullptr)
    {
        IFC_RETURN(core->IsObjectAnActiveRootVisual(pOldParent, &oldParentIsRootVisual));
    }

    // Determine if this is entering the live tree
    bool isEnteringLiveTree = pParent != nullptr && (pParent->IsActive() || parentIsRootVisual);

    // We ignore the internal root visual in the managed peer tree.
    if (parentIsRootVisual)
    {
        pParent = nullptr;
    }
    if (oldParentIsRootVisual)
    {
        pOldParent = nullptr;
    }

    // Ensure that an ancestor is pegged as GC root. This ensures that a stateful child tree will
    // not be GC'd if the child tree is parented to an orphaned core object parent.
    if (pParent != nullptr)
    {
        EnsureGCRootPegOnAncestor(pParent);
    }

    // If the peer is stateful (participates), it must be protected from GC.
    // If the peer doesn't exist yet, the expected reference will be added by
    // OnManagedPeerCreated when the peer is created.
    // If the peer is stateless, the expected reference will be added by
    // CDependencyObject::SetParticipatesInManagedTreeDefault if the peer becomes stateful.
    if (ParticipatesInManagedTree() && HasManagedPeer())
    {
        // Protect peer from GC by setting an expected reference on it, because it is being
        // added to the tree.
        if (pParent != nullptr)
        {
            IFC_RETURN(SetExpectedReferenceOnPeer());
        }

        // Remove previously added expected reference on the peer, if there are no parents left.
        if (!hasAtLeastOneParent)
        {
            IFC_RETURN(ClearExpectedReferenceOnPeer());
        }
    }

    // Notify peer that the parent has changed
    if (HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_OnParentUpdated(
                this,
                pOldParent,
                pParent,
                isEnteringLiveTree));
    }
    else if (isEnteringLiveTree && do_pointer_cast<CFrameworkElement>(this))
    {
        // Even if there is no peer, descendants with bindings need to be notified of the DataContext
        // change, when the parent has changed and element is entering the live tree. If there
        // is a peer, FrameworkCallbacks_OnParentUpdated will notify.
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_PropagateDataContextChange(static_cast<CFrameworkElement*>(this)));
    }

    // If this DO has a parent, it need not be pegged anymore because the parent will protect it.
    // Do this after FrameworkCallbacks_OnParentUpdated because ResourceDictionary::OnParentUpdated pegs the
    // App's RD, and we don't want this Unpeg to clear m_bReferenceTrackerPeg before that.
    if (pParent != nullptr)
    {
        UnpegManagedPeerNoRef();
    }

    return S_OK;
}


// Ensure that given parent/association owner or an ancestor is pegged as GC root, so that its
// subtree can be marked as reachable during GC walk.
//
// Peers are pegged at creation, (see DXamlCore::GetPeerPrivate's call to SetReferenceTrackerPeg,)
// because they will be walked for GC from m_Peers. However, a core object without a peer
// need not be pegged at creation, because it cannot be be walked for GC until it is added
// to m_PegNoRefCoreObjectsWithoutPeers or to a subtree. When a core object is added
// to a subtree, this method needs to be called because the subtree may have peers in m_Peers,
// so the subtree needs to be marked as reachable during that GC walk. This method essentially moves
// the peg from the child to the subtree root -- note that the caller unpegs the child after calling
// this method.
void CDependencyObject::EnsureGCRootPegOnAncestor(
    _In_ CDependencyObject *pParentOrAssociationOwner)
{
    bool pegParent = false;

    // If this parent/association owner has a peer, it is either reachable from a pegged
    // ancestor or pegged as a GC root because of parsing (see DependencyObject_ShouldCreatePeerWithStrongRef)
    // or GetPeer's SetReferenceTrackerPeg(). So there is nothing more to do.
    if (!pParentOrAssociationOwner->HasManagedPeer())
    {
        if (!pParentOrAssociationOwner->IsParentAware())
        {
            // AssociationOwner is not parent aware. If it is not associated, it needs to
            // be pegged as a GC root, because its subtree may have peers that will be
            // walked for GC from m_Peers. Otherwise subroot will not be marked as reachable
            // during GC walk. If it is associated, it is already part of a subtree whose
            // root has been pegged.
            if (!pParentOrAssociationOwner->IsAssociated())
            {
                pegParent = true;
            }
        }
        else
        {
            // Parent is parent aware. If the parent doesn't have a parent, it needs to
            // be pegged as a GC root, because the the subtree may have peers that will be
            // walked for GC from m_Peers. Otherwise subroot will not be marked as reachable
            // during GC walk. If it has a parent, it is already part of a subtree whose
            // root has been pegged.
            if (!pParentOrAssociationOwner->GetParentInternal(false /*publicParentOnly*/))
            {
                pegParent = true;
            }
        }

        // Peg parent as a GC root using m_PegNoRefCoreObjectsWithoutPeers until it gets a
        // parent/association owner. Otherwise its subtree will not be reachable.
        if (pegParent)
        {
            // PegManagedPeerNoRef will peg core object if peer doesn't exist
            pParentOrAssociationOwner->PegManagedPeerNoRef();
        }
    }
}

// Set/Clear expected reference on peer when an object that supports multiple association
// changes association
_Check_return_ HRESULT
CDependencyObject::OnMultipleAssociationChange(
    _In_opt_ CDependencyObject *pAssociationOwner)
{
    // If parent aware, OnParentChange will manage the references on the peer.
    if (DoesAllowMultipleAssociation() && !IsParentAware())
    {
        // Ensure that an ancestor is pegged as GC root. This ensures that a stateful child tree will
        // not be GC'd if the child tree is parented to an orphaned core object parent.
        if (pAssociationOwner != nullptr)
        {
            EnsureGCRootPegOnAncestor(pAssociationOwner);
        }

        // Set expected reference on peer when associated, and clear reference when
        // not associated. If the peer doesn't exist, the ref will be
        // set when OnManagedPeerCreated is called. If the peer optionally participates,
        // the ref will be added if SetParticipatesInManagedTreeDefault is called
        // to make it participate.
        if (HasManagedPeer() && ParticipatesInManagedTree())
        {
            if (IsAssociated())
            {
                // Since SetExpectedReferenceOnPeer sets only one ref, it is safe
                // to call multiple times while this object is associated.
                IFC_RETURN(SetExpectedReferenceOnPeer());
            }
            else
            {
                IFC_RETURN(ClearExpectedReferenceOnPeer());
            }
        }

        // Association owner will protect from GC, so remove peg
        if (IsAssociated())
        {
            UnpegManagedPeerNoRef();
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------------------
//
//  CDependencyObject::SetParticipatesInManagedTreeDefault
//
//  If this object is OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE, and it isn't already
//  participating, start doing so now.  This is called when a previously stateless peer
//  gains state. A stateful peer will be protected from GC, by the core object holding
//  an expected reference on it. On the other hand, a stateless peer can be released
//  and resurrected as needed.
//
//-------------------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::SetParticipatesInManagedTreeDefault()
{
    HRESULT hr = S_OK;
    bool fPegged = false;

    // If this object is either always participating or never participating, then
    // there's nothing we need do.
    if( ParticipatesInManagedTreeInternal() != OPTIONALLY_PARTICIPATES_IN_MANAGED_TREE )
    {
        goto Cleanup;
    }

    // Similarly, if this object has already been set to participate, then we're done.
    if( GetParticipatesInManagedTreeDefault() )
    {
        goto Cleanup;
    }

    if (HasManagedPeer())
    {
        // Up until now, this object may have been resurrectable. If we have a peer, it can
        // be GC'ed at any point until we make the call to SetExpectedReferenceOnPeer.
        // After we set the participates-in-tree flag, the DO will be no longer be considered
        // resurrectable, so our SetExpectedReferenceOnPeer call would fail if a GC happens before
        // the SetExpectedReferenceOnPeer call.
        // To avoid this, we peg the peer now.
        IFC(EnsurePeerAndTryPeg(&fPegged));
    }

    // Set this object to participate.
    m_bitFields.fParticipatesInManagedTreeDefault = TRUE;

    // Forget that we've ever had a managed peer, because up until now resurrection
    // was allowed. From here on out, after we created a managed peer, we no longer
    // allow for resurrection of this object.
    if (!HasManagedPeer())
    {
        SetHasEverHadManagedPeer(false);
    }

    if (HasManagedPeer())
    {
        IFC(SetExpectedReferenceOnPeerAndNotifyParentUpdated());
    }

Cleanup:
    if (fPegged)
    {
        UnpegManagedPeer();
    }

    return hr;
}

// Verify that we're executing on the thread this DO belongs to.
_Check_return_ HRESULT CDependencyObject::CheckThread()
{
    if (GetContext()->GetThreadID() != ::GetCurrentThreadId())
    {
        return RPC_E_WRONG_THREAD;
    }

    return S_OK;
}

HRESULT CDependencyObject::SetExpectedReferenceOnPeerAndNotifyParentUpdated()
{
    auto parentCount = GetParentCount();
    if (parentCount > 0)
    {
        // If a parent exists, protect peer from GC by setting an expected
        // reference on it, and notify peer of parent change. This wasn't done when
        // the peer was created, because this object wasn't participating before or did not have a peer.
        // If there is no parent, this will be done later when a parent is added.

        IFC_RETURN(SetExpectedReferenceOnPeer());

        for (size_t parentIndex = 0; parentIndex < parentCount; parentIndex++)
        {
            CDependencyObject *pParentNoRef = GetParentItem(parentIndex);

            IFC_RETURN(FxCallbacks::FrameworkCallbacks_OnParentUpdated(
                this,
                nullptr, // pOldParent -- Need not track previous parent
                pParentNoRef,
                // If this is a regular DO with a single parent, then we should specify whether
                // the new parent is alive.
                (parentIndex == 0 && !DoesAllowMultipleParents() && pParentNoRef->IsActive()) // isNewParentAlive
                ));
        }
    }
    else if (!IsParentAware() && DoesAllowMultipleAssociation())
    {
        // For non-parent aware DOs that allow multiple association,
        // add expected reference on peer if DO is associated.
        if (IsAssociated())
        {
            IFC_RETURN(SetExpectedReferenceOnPeer());
        }
    }

    return S_OK;
}

// Walks up the parent chain using similar logic to VisualTreeHelper::GetParentStaticPrivate.
CDependencyObject*
CDependencyObject::GetParentForSoundMode()
{
    // Do not return the parent unless this element is in the live tree.
    if (!IsActive())
    {
        return nullptr;
    }

    CDependencyObject* parent = nullptr;

    // Since Hyperlink can take focus, it may be passed in to get the visual parent.
    CHyperlink* hyperLink = do_pointer_cast<CHyperlink>(this);

    if (hyperLink)
    {
        parent = hyperLink->GetContainingFrameworkElement();
    }
    else
    {
        parent = GetParent();
        CUIElement* parentAsUIElement = do_pointer_cast<CUIElement>(parent);

        // Ensure parent is UIElement. Continue up the parent chain while the parent is not a UIElement.
        while (parent && !parentAsUIElement)
        {
            parent = parent->GetParent();
            parentAsUIElement = do_pointer_cast<CUIElement>(parent);
        }
    }

    return parent;
}

// Gets the element sound mode if it has one, else returns ElementSoundMode::Default
DirectUI::ElementSoundMode
CDependencyObject::GetSoundModeIfAvailable()
{
    KnownPropertyIndex propertyIndex;

    if (OfTypeByIndex<KnownTypeIndex::Control>())
    {
        propertyIndex = KnownPropertyIndex::Control_ElementSoundMode;
    }
    else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(this))
    {
        propertyIndex = ifocusable->GetElementSoundModePropertyIndex();
    }
    else if (OfTypeByIndex<KnownTypeIndex::FlyoutBase>())
    {
        propertyIndex = KnownPropertyIndex::FlyoutBase_ElementSoundMode;
    }
    else
    {
        return DirectUI::ElementSoundMode::Default;
    }

    CValue soundModeValue;

    IFCFAILFAST(GetValue(GetPropertyByIndexInline(propertyIndex), &soundModeValue));
    return static_cast<DirectUI::ElementSoundMode>(soundModeValue.AsEnum());
}

// GetEffectiveSoundMode does a walk up the visual tree from this element to see if any of its parents apply more restrictive SoundMode rules than it does.
// If a parent's sound mode is more restrictive than this element's, then the parent's rule applies as the "effective sound mode".
DirectUI::ElementSoundMode CDependencyObject::GetEffectiveSoundMode()
{
    DirectUI::ElementSoundMode effectiveSoundMode = DirectUI::ElementSoundMode::Default;
    CDependencyObject* currentElement = this;

    // Walk up the tree from this element until we find an element with a more restrictive sound mode.
    while (currentElement)
    {
        const DirectUI::ElementSoundMode currentElementSoundMode = currentElement->GetSoundModeIfAvailable();

        if (currentElementSoundMode > effectiveSoundMode)
        {
            effectiveSoundMode = currentElementSoundMode;
        }

        if (effectiveSoundMode == DirectUI::ElementSoundMode::Off)
        {
            break;
        }

        xref_ptr<CDependencyObject> parent;
        CPopup* currentAsPopup = do_pointer_cast<CPopup>(currentElement);

        if (currentAsPopup)
        {
            CFlyoutBase* associatedFlyout = currentAsPopup->GetAssociatedFlyoutNoRef();

            if (associatedFlyout)
            {
                parent = associatedFlyout;
            }
            else
            {
                parent = currentElement->GetParentForSoundMode();
            }
        }
        else
        {
            CFrameworkElement* currentAsFE = do_pointer_cast<CFrameworkElement>(currentElement);
            CHyperlink* currentAsHyperlink = do_pointer_cast<CHyperlink>(currentElement);

            if (currentAsFE || currentAsHyperlink)
            {
                parent = currentElement->GetParentForSoundMode();
                if (do_pointer_cast<CPopupRoot>(parent))
                {
                    if (currentAsFE->IsActive())
                    {
                        parent = currentAsFE->GetLogicalParentNoRef();
                    }
                    else
                    {
                        parent = nullptr;
                    }
                }
            }
            else
            {
                break;
            }
        }

        currentElement = parent;
    }

    return effectiveSoundMode;
}

//------------------------------------------------------------------------
//
// GC's ReferenceTrackerWalk. Walk references accessible from this object.
//
//------------------------------------------------------------------------

void CDependencyObject::ReferenceTrackerWalk(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    if (!IsProcessingReferenceTrackerWalk())
    {
        // Prevent cycles in walk
        SetIsProcessingReferenceTrackerWalk(TRUE);

        ReferenceTrackerWalkCore(
            walkType,
            isRoot,
            shouldWalkPeer);

        SetIsProcessingReferenceTrackerWalk(FALSE);
    }
}

//------------------------------------------------------------------------
//
// GC's ReferenceTrackerWalk. Walk peer and property references to DOs.
//
//------------------------------------------------------------------------

bool CDependencyObject::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool shouldWalkProperties = true;

    // Walk framework peer if it exists, and we are asked to walk it. This allows
    // a framework peer to be created only when it is referenced by an RCW or
    // when it uses a CCW.
    if (shouldWalkPeer && HasManagedPeer())
    {
        bool walkedPeer = false;
        bool isPeerAlive = false;

        IFC(FxCallbacks::FrameworkCallbacks_ReferenceTrackerWalk(
            this,
            walkType,
            isRoot,
            &isPeerAlive,
            &walkedPeer));

        // Walk property values, if peer exists and was walked, or if peer doesn't exist.
        // Don't walk property values if peer exists but was not walked, to optimize the walk.
        shouldWalkProperties = walkedPeer || !isPeerAlive;
    }

    if (shouldWalkProperties)
    {
        const CClassInfo* pClassInfo = GetClassInformation();

        // Walk core property values that are DOs
        const CObjectDependencyProperty* pNullObjectProperty = MetadataAPI::GetNullObjectProperty();
        for (const CObjectDependencyProperty* pObjectProperty = pClassInfo->GetFirstObjectProperty(); pObjectProperty != pNullObjectProperty; pObjectProperty = pObjectProperty->GetNextProperty())
        {
            const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(pObjectProperty->m_nPropertyIndex);
            CDependencyObject *pDONoRef = GetDependencyObjectFromPropertyStorage(pProperty);

            if (pDONoRef)
            {
                pDONoRef->ReferenceTrackerWalk(
                    walkType,
                    false,  //isRoot
                    true);  //shouldWalkPeer
            }
        }

        // Enumerate all the sparse properties with values and reference walk as needed.
        if (m_pValueTable != nullptr)
        {
            for (auto& entry : *m_pValueTable)
            {
                if (entry.second.value.AsObject() != nullptr &&
                    !IsDependencyPropertyBackReference(entry.first))
                {
                    entry.second.value.AsObject()->ReferenceTrackerWalk(
                        walkType,
                        false,  //isRoot
                        true);  //shouldWalkPeer
                }
            }
        }

        // Enumerate the modified value store as well
        ModifiedValuesList* modifiedValues = GetModifiedValuesStorage();

        if (modifiedValues)
        {
            // Enumerate the modified value store as well
            for (auto& modifiedValue : *modifiedValues)
            {
                modifiedValue->ReferenceTrackerWalk(
                    walkType,
                    false,  //isRoot
                    true);  //shouldWalkPeer
            }
        }
    }

Cleanup:
    return shouldWalkProperties;
}

//------------------------------------------------------------------------
//
//  Method: SetParent
//
//  Synopsis:
//     Sets the parent of an element.  If the parent is non-null, the
//     NW render state association between parent and child is also set.
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::SetParent(
    _In_opt_ CDependencyObject *pNewParent,
    bool fPublic /*= true */,
    _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler /*= NULL */)
{
    HRESULT hr = S_OK;

    CDependencyObject *pOldParent = GetParentInternal(false);
    xref::weakref_ptr<CDependencyObject>* pOldMentor = nullptr;
    bool fOldParentIsPublic = m_bitFields.fParentIsPublic;
    bool fOldParentIsInheritanceContextOnly = m_bitFields.fParentIsInheritanceContextOnly;
    bool isSwapChainBackgroundPanel = false;
    RENDERCHANGEDPFN renderChangedHandler = NWGetRenderChangedHandlerInternal();
    auto core = GetContext();

    // Optimize the noop case.
    if (pNewParent == pOldParent)
    {
        // at this point, we did not yet assign pOldMentor, so it won't get released
        goto Cleanup;
    }

    isSwapChainBackgroundPanel = OfTypeByIndex<KnownTypeIndex::SwapChainBackgroundPanel>();
    if(isSwapChainBackgroundPanel)
    {
        IFC(static_cast<CSwapChainBackgroundPanel*>(this)->PreTreeParentUpdated(pNewParent));
    }

    pOldMentor = m_pMentor; // will get released if this call is successful

    if (m_bitFields.fParserOwnsParent)
    {
        IFC(E_UNEXPECTED); // Replace this with a better error code?
    }

    // When the parent is removed, propagate the rendering change to it one last time.
    if (pOldParent
        && !m_bitFields.fParentIsInheritanceContextOnly
        && renderChangedHandler)
    {
        NotifyParentChange(pOldParent, renderChangedHandler);
    }

    m_pParent = pNewParent;
    m_bitFields.fParentIsPublic = fPublic;
    m_bitFields.fParentIsInheritanceContextOnly = FALSE;
    NWSetRenderChangedHandlerInternal(pfnNewParentRenderChangedHandler);

    // Increment global counter on core user for validation of cached inherited property bags.
    // If an element was added/removed from the tree, we simply invalidate all property bags.
    // TODO: MERGE: Why doesn't this happen in CMultiParentShareableDO::AddParent/RemoveParent?
    core->m_cInheritedPropGenerationCounter++;
    core->m_cIsRightToLeftGenerationCounter++;
    if (core->m_cIsRightToLeftGenerationCounter == 0)
    {
        // Since this counter is only 8 bits it can easily overflow, which could cause a no-op when
        // re-evaluating the right-to-left-ness of a UIElement.
        // We don't want to increase the number of bits consumed by UIElement so the compromise solution
        // is to increase the counter to 1 when we detect it's overflowed, which will handle the most common
        // case of a new UIElement entering the tree for the first time.
        core->m_cIsRightToLeftGenerationCounter = 1;
    }

    // Mark the new parent as dirty.
    if (m_pParent &&
        pfnNewParentRenderChangedHandler)
    {
        NotifyParentChange(m_pParent, pfnNewParentRenderChangedHandler);
    }

    // Manage reference to peer and notify peer when parent is changed
    IFC(OnParentChange(pOldParent, m_pParent, m_pParent != nullptr));

    // TODO: MERGE: Why is the code here different from the managed peer code in CMultiParentShareableDO::AddParent/RemoveParent?
    // For some objects, we need to keep the managed peer in sync with this new parent.
    if (HasManagedPeer())
    {
        bool oldParentIsRootVisual = false;
        bool newParentIsRootVisual = false;

        if (m_pParent != NULL)
        {
            IFC(core->IsObjectAnActiveRootVisual(m_pParent, &newParentIsRootVisual));
        }

        // Old code used to use the same IsObjectAnActiveRootVisual to check whether the old
        // parent was an active root, but this doesn't work correctly during the shutdown
        // sequence, where clear the list of active roots (storing the old list in a temp)
        // before cleaning up each root.
        oldParentIsRootVisual = m_bitFields.isParentAnActiveRootVisual;

        // Save time by avoiding the unpeg/re-peg if the old AND new parent are active root
        // visuals.
        if (oldParentIsRootVisual != newParentIsRootVisual)
        {
            if (oldParentIsRootVisual)
            {
                // If the element is begin detached from an active root, it has been pegged
                // by that parent so unpeg it now.
                UnpegManagedPeer();
            }
            else if (newParentIsRootVisual)
            {
                // Or, if we've been added to the internal root visual, then
                // we need to peg it to  keep a strong reference on the managed
                // peer.

                // Note that this is a counted peg, which is safer than PegManagedPeerNoRef
                IGNOREHR(PegManagedPeer());
            }

            // Update the flag that will be used to determine whether we need to unpeg this
            // object when it is detached from this parent.
            m_bitFields.isParentAnActiveRootVisual = newParentIsRootVisual;
        }
    }

#if DBG
    // As the tree gets (re)constructed, we shouldn't see any cases where a parent and child
    // are both live, but are associated with different VisualTrees.
    // The exception is that FlyoutBases returned from GetContextFlyout get parented to multiple TextBoxes (weirdly)
    // when those TextBoxes enter the tree.
    if (m_pParent && m_pParent->IsActive() && this->IsActive() && !this->OfTypeByIndex<KnownTypeIndex::FlyoutBase>())
    {
        VisualTree* parentVisualTree = m_pParent->GetVisualTree();
        VisualTree* thisVisualTree = this->GetVisualTree();
        if (parentVisualTree && thisVisualTree)
        {
            ASSERT(parentVisualTree == thisVisualTree);
        }
    }
#endif

Cleanup:
    if (FAILED(hr))
    {
        // Reset old parent on failure
        if (fOldParentIsInheritanceContextOnly)
        {
            m_pMentor = pOldMentor;

            m_bitFields.fParentIsInheritanceContextOnly = TRUE;
        }
        else
        {
            m_pParent = pOldParent;
        }
        m_bitFields.fParentIsPublic = fOldParentIsPublic;
    }
    else if (fOldParentIsInheritanceContextOnly && pOldMentor)
    {
        // If we were successful, we no longer need a WeakRef to the old mentor
        delete pOldMentor;
    }

    if(isSwapChainBackgroundPanel)
    {
        static_cast<CSwapChainBackgroundPanel*>(this)->PostTreeParentUpdated(pNewParent);
    }

    RRETURN(hr);
}

// Checks if the specified property has ever been set on this object
// NOTE: Does not support attached inherited storage group properties.
bool CDependencyObject::IsPropertyDefaultByIndex(_In_ KnownPropertyIndex nIndex) const
{
    const CDependencyProperty* dp = GetPropertyByIndexInline(nIndex);
    ASSERT(!dp->IsInheritedAttachedPropertyInStorageGroup());
    return IsPropertyDefault(dp);
}

CCoreServices*
CDependencyObject::GetContextInterface() const
{
    return GetContext();
}

//
// Create a peer for this object (if it doesn't already have one), and give the requested peg.
// This is the main ensure-and-peg method that the other methods call.
//

_Check_return_
HRESULT
CDependencyObject::PrivateEnsurePeerAndTryPeg(_In_ bool fPegNoRef, _In_ bool fPegRef, _In_ bool isShutdownException, _Out_opt_ bool* pfPegged)
{
    bool fPegged = false;

    if(!HasManagedPeer())
    {
        // We don't already have a peer.  Create one with appropriate pegging.
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_CreateManagedPeer(this, GetTypeIndex(), fPegNoRef, fPegRef, isShutdownException));
        fPegged = fPegRef;
    }
    else if (fPegRef)
    {
        // We already have the peer, put a peg on it.
        fPegged = TryPegManagedPeer(this, isShutdownException);
    }

    if (pfPegged)
    {
        *pfPegged = fPegged;
    }

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Method: TryEnsureManagedPeer
//
//  Synopsis:
//      Tries to ensure that peer for a core object exists. Cannot create
//  a peer if a peer had previously been created and released and
//  cannot be resurrected, which can occur during DO tear down.
//
//------------------------------------------------------------------------

// Do we need this?  Can't this be accomplished with one of the other ensure/peg methods?

_Check_return_ HRESULT
CDependencyObject::TryEnsureManagedPeer(
    _Out_ bool* pIsPeerAvailable,
    _In_ bool fPegNoRef,
    _In_ bool fPegRef,
    _In_ bool isShutdownException,
    _Out_opt_ bool* pfPegged)
{
    *pIsPeerAvailable = FALSE;

    if (HasManagedPeer() ||                                      // Peer exists. Call CreateManagedPeer to peg if needed.
       !HasEverHadManagedPeer() ||                                 // Peer has never been created
       (HasEverHadManagedPeer() && !ParticipatesInManagedTree()))  // Peer is dead, but can be resurrected
    {
        IFC_RETURN(PrivateEnsurePeerAndTryPeg(fPegNoRef, fPegRef, isShutdownException, pfPegged));
        *pIsPeerAvailable = TRUE;
    }

    return S_OK;
}



// This is called from the framework to indicate that a managed
// peer has been created for this object.  This could happen
// on the initiative of the framework, or as a result of the
// above EnsurePeerAndTryPeg call.
HRESULT CDependencyObject::OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsManagedPeerPeggedNoRef)
{
    CDependencyObject *pParent = nullptr;

    SetHasManagedPeer(TRUE, fIsCustomDOType);

    #if DBG
    // Indicate that if this method was overridden, the override remembered to call base
    m_fCalledOnManagedPeerCreated = true;

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    // If this assert fires, the managed peer of this object
    // was pre-maturely collected.
    if ((ControlsManagedPeerLifetime() || ParticipatesInManagedTree())
            && !runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
    {
        ASSERT(!HasEverHadManagedPeer());
    }

    m_pLastDXAMLPeerNoRef = GetDXamlPeer();
    #endif

    SetHasEverHadManagedPeer(true);

    pParent = GetParentInternal(false);

    // If the peer has state (participates), protect it from GC if the core object is in the tree
    if (ParticipatesInManagedTree())
    {
        IFC_RETURN(SetExpectedReferenceOnPeerAndNotifyParentUpdated());
    }

    // If this DO is in the tree, it need not be pegged anymore because the parent/association owner
    // will protect it. The peg may have been added when the peer was created.
    if ((pParent != nullptr)
        || (!IsParentAware() && DoesAllowMultipleAssociation() && IsAssociated()))
    {
        UnpegManagedPeerNoRef();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetHasManagedPeer
//
//  Synopsis:
//      This sets or clears the m_bitFields.fHasManagedPeer.  This
//      method is thread-safe, so that it can be called from the
//      managed finalizer thread without requiring a lock to access
//      any of the CDependencyObject bit fields.
//
//------------------------------------------------------------------------

void CDependencyObject::SetHasManagedPeer( XUINT32 fHasManagedPeer, XUINT32 fIsCustomType )
{
    DependencyObjectBitFields oldValue, newValue;

    ASSERT( sizeof(DependencyObjectBitFields) == sizeof(XINT32) );

    while(TRUE)
    {
        XINT32 retValue;

        // Get the current value
        oldValue = m_bitFields;

        // Calculate the new value
        newValue = oldValue;
        newValue.fHasManagedPeer = fHasManagedPeer;
        newValue.fIsCustomType |= fIsCustomType;

        // Put the new value into m_bitFields
        retValue = PAL_InterlockedCompareExchange(
                            reinterpret_cast<XINT32*>(&m_bitFields),
                            *reinterpret_cast<XINT32*>(&newValue),
                            *reinterpret_cast<XINT32*>(&oldValue));

        // Break out if successful
        if( retValue == *(XINT32*)&oldValue )
            break;
    }
}

// Set/Clear if a peer has ever existed.
// To save a bit field, this state is stored in LSB of m_pDXAMLPeer.
// because m_hasEverHadManagedPeer & m_pDXAMLPeer are in a union.
void CDependencyObject::SetHasEverHadManagedPeer(
    bool setHasEverHadManagedPeer)
{
    if (setHasEverHadManagedPeer)
    {
        m_hasEverHadManagedPeer |= c_hasEverHadManagedPeer;
    }
    else
    {
        ASSERT(!HasManagedPeer());
        m_hasEverHadManagedPeer &= ~c_hasEverHadManagedPeer;
    }
}

// Set/Clear pointer to peer
// Note that m_hasEverHadManagedPeer & m_pDXAMLPeer are in a union.
void CDependencyObject::SetDXamlPeer(
    _In_opt_ DirectUI::DependencyObject* pDXamlPeer)
{
    bool hasEverHadManagedPeer = HasEverHadManagedPeer();

    m_pDXAMLPeer = pDXamlPeer;

    // Restore HasEverHadManagedPeer because it is in a union with m_pDXAMLPeer
    if (hasEverHadManagedPeer)
    {
        SetHasEverHadManagedPeer(true);
    }
}

//------------------------------------------------------------------------
//
//  Method: Unsafe[Peg/UnPeg]ManagedPeer
//
//  Synopsis:
//      These methods strengthen or weaken the reference to this object's
//      managed peer.  This is necessary when the core needs to hold a
//      strong reference the object, to prevent the peer from being collected.
//      For example, while a timer or storyboard is running, even if there
//      are no references to it, it may not be collected.
//
//------------------------------------------------------------------------

void CDependencyObject::PegManagedPeerNoRef()
{
    if( HasManagedPeer() )
    {
        FxCallbacks::FrameworkCallbacks_PegManagedPeerNoRef(this);
    }
    else
    {
        auto core = GetContext();
        // If there is no peer, mark the core object as a GC root, so
        // it can be reachable during GC
        core->PegNoRefCoreObjectWithoutPeer(this);
    }
}


void CDependencyObject::UnpegManagedPeerNoRef()
{
    auto core = GetContext();

    core->UnpegNoRefCoreObjectWithoutPeer(this);

    if( HasManagedPeer() )
    {
        FxCallbacks::FrameworkCallbacks_UnpegManagedPeerNoRef(this);
    }
}

//------------------------------------------------------------------------
//
//  Method: TryPegPeer
//
//  Synopsis:
//      If peer exists, peg it. Also checks if this object is pending
//  delete because peer has been deleted or is pending delete.
//
//------------------------------------------------------------------------

void
CDependencyObject::TryPegPeer(
    _Out_ bool *pPegged,
    _Out_ bool *pIsPendingDelete)
{
    *pPegged = FALSE;
    *pIsPendingDelete = FALSE;

    if (!HasManagedPeer())
    {
        if (HasEverHadManagedPeer() && ParticipatesInManagedTree())
        {
            // Peer was deleted and cannot be resurrected
            *pIsPendingDelete = TRUE;
            return;
        }
    }
    else
    {
        // Peg if peer exists. Otherwise check if peer is pending delete.
        FxCallbacks::FrameworkCallbacks_TryPegPeer(this, pPegged, pIsPendingDelete);
    }
}

//-----------------------------------------------------------------------------
//
//  PegManagedPeer
//  UnpegManagedPeer
//
//  These methods are used to keep this object's managed peer from being GC-ed.
//  This is equivalent to Peg/UnpegManagedPeerNoRef, except that it's ref-counted and
//  therefore safer (though slower).
//  isShutdownException is false by default.  It should only be set to true in cases
//  where a pegged object would not be considered a leak if we're shutting down due
//  to a legitimate exception.  isShutdownException needs to be the same value for matching
//  Peg and Unpeg calls.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::PegManagedPeer( _In_opt_ bool isShutdownException, _Out_opt_ bool* pfPegged )
{
    if (pfPegged)
    {
        *pfPegged = FALSE;
    }

    if (ParticipatesInManagedTreeInternal() == DOESNT_PARTICIPATE_IN_MANAGED_TREE)
    {
        return S_FALSE;
    }
    
    // It isn't clear why this code to participate was added. Participating doesn't seem necessary,
    // because pegging a peer doesn't make it stateful. Keep this code to reduce risk and try
    // to remove it in the future after re-evaluation.
    IFC_RETURN(SetParticipatesInManagedTreeDefault());

    IFC_RETURN(PrivateEnsurePeerAndTryPeg(
                FALSE, // fPegNoRef
                TRUE,  // fPegRef
                isShutdownException,
                pfPegged ));


    return S_OK;
}


void CDependencyObject::UnpegManagedPeer( _In_opt_ bool isShutdownException )
{
    if( ParticipatesInManagedTreeInternal() != DOESNT_PARTICIPATE_IN_MANAGED_TREE  )
    {
        TryUnpegManagedPeer(this, isShutdownException);
    }
}

// Updates generation counters so that inherited properties know to re-query for the inherited value.
void CDependencyObject::InvalidateInheritedProperty(_In_ const CDependencyProperty* pDP)
{
    auto core = GetContext();

    // TODO: Consider skipping updating the generation counter if we can
    // be absolutely sure this element has no children that could inherit
    // from it.
    core->m_cInheritedPropGenerationCounter++;

    if (pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_FlowDirection
        || pDP->GetIndex() == KnownPropertyIndex::Run_FlowDirection)
    {
        core->m_cIsRightToLeftGenerationCounter++;
        if (core->m_cIsRightToLeftGenerationCounter == 0)
        {
            // Since this counter is only 8 bits it can easily overflow, which could cause a no-op when
            // re-evaluating the right-to-left-ness of a UIElement.
            // We don't want to increase the number of bits consumed by UIElement so the compromise solution
            // is to increase the counter to 1 when we detect it's overflowed, which will handle the most common
            // case of a new UIElement entering the tree for the first time.
            core->m_cIsRightToLeftGenerationCounter = 1;
        }

    }
}

// The inheritance context of this object has changed. This is the time bindings on this object
// should attempt to resolve their sources.
_Check_return_ HRESULT CDependencyObject::NotifyInheritanceContextChanged(_In_ InheritanceContextChangeKind kind)
{
    if (!m_isProcessingInheritanceContextChanged)
    {
        // Protect against cycles.
        auto cleanupGuard = wil::scope_exit([this]{ m_isProcessingInheritanceContextChanged = false; });
        m_isProcessingInheritanceContextChanged = true;

        IFC_RETURN(OnInheritanceContextChanged());

        if (HasManagedPeer())
        {
            IFC_RETURN(FxCallbacks::JoltHelper_RaiseEvent(
                this,
                ManagedEvent::ManagedEventInheritanceContextChanged,
                /* pArgs */ nullptr));
        }

        // Enumerate all the sparse properties with values and notify each about the change of inheritance context.
        if (m_pValueTable != nullptr)
        {
            // To protect against reentrancy, copy into a temporary vector before iterating.
            Jupiter::arena<DefaultSparseArenaSize> localArena;
            auto tempValues = GetSparseValueEntries(localArena);
            for (const auto& entry : tempValues)
            {
                auto object = entry.second.value.AsObject();
                if (object)
                {
                    IFC_RETURN(object->NotifyInheritanceContextChanged());
                }
            }
        }
    }

    return S_OK;
}

// Notifies the managed layer that this dependency property value has
// been changed from the core
//
// Note:
// This is only operational in FEs and above, this is done with the
// assumption that the only non-abstract classes start with FrameworkElement
// DO is also non-abstract but it has no core properties aside from
// attached properties and the only way of changing those is with animations.
_Check_return_ HRESULT CDependencyObject::NotifyPropertyChanged(const PropertyChangedParams& args)
{
    // Here the call to ParserOwnsParent ensure that change notifications are not set while the
    // parser is setting properties to the object, that way the parse time is not affected
    // by the notification mechanism. Once the element enters the tree and the binding/listener is set
    // the notifications will flow.
    //if (!ParserOwnsParent())
    {
        IFC_RETURN(OnPropertyChanged(args));

        if (HasManagedPeer())
        {
            IFC_RETURN(FxCallbacks::DependencyObject_NotifyPropertyChanged(this, args));
        }

        if (EventEnabledPropertyChangedInfo())
        {
            TracePropertyChanged(args.m_pDP, args.m_pNewValue);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetPeerReferenceToProperty
//
//  Synopsis:
//      Adds a reference to the property value's managed peer to keep it alive
//      as part of the tree.
//  Note:
//      This method should mainly be used for shareable properties, for
//      non-shareable properties SetParent is used. There might be special cases where
//      it's used for non-shareable properties (See: CInlineUIContainer::SetValue)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::SetPeerReferenceToProperty(
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& value,
    _In_ bool bPreservePegNoRef,
    _In_opt_ IInspectable* pNewValueOuter,
    _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter)
{
    // If this object isn't yet participating in the managed tree, we need to start
    // doing so now.
    IFC_RETURN(SetParticipatesInManagedTreeDefault());

    // Store the reference from 'this' to the value.
    IFC_RETURN(FxCallbacks::DependencyObject_SetPeerReferenceToProperty(this, pDP, value, bPreservePegNoRef, pNewValueOuter, ppOldValueOuter));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: AddPeerReferenceToItem
//
//  Synopsis:
//      Adds a reference to the child's managed peer to keep it alive
//      as part of the tree.
//------------------------------------------------------------------------


_Check_return_
HRESULT
CDependencyObject::AddPeerReferenceToItem(_In_ CDependencyObject *pChild)
{
    IFCEXPECT_RETURN(pChild);

    // Only add the owner if we have state in our
    // managed peer
    if (pChild->ParticipatesInManagedTree())
    {
        // If this object isn't yet participating in the managed tree, we need to start
        // doing so now.

        IFC_RETURN(SetParticipatesInManagedTreeDefault());

        // Store the reference from 'this' to the child (this is stored in DO.m_pItemReferences).

        IFC_RETURN(FxCallbacks::DependencyObject_AddPeerReferenceToItem(this, pChild));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: RemovePeerReferenceToItem
//
//  Synopsis:
//      Removes a reference to the child's managed peer, no need for this
//      object to keep it alive anymore.
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDependencyObject::RemovePeerReferenceToItem(_In_ CDependencyObject *pChild)
{
    IFCEXPECT_RETURN(pChild);

    // If we don't have a managed peer then we can return early
    // as we won't have the managed reference for sure
    // Note: This might happen during shutdown so we don't want to
    // create managed peers unnecessarily.
    if (!HasManagedPeer() && !pChild->HasManagedPeer())
    {
        return S_OK;
    }

    // Only add the owner if we have state in our
    // managed peer
    if (pChild->ParticipatesInManagedTree())
    {
        IFC_RETURN(FxCallbacks::DependencyObject_RemovePeerReferenceToItem(this, pChild));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Set expected reference on peer, to protect it from GC
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::SetExpectedReferenceOnPeer()
{
    // Cannot add expected reference if peer does not exist. Will be added
    // later by OnManagedPeerCreated.
    if (!HasManagedPeer())
    {
        return S_OK;
    }

     // Set expected reference on peer, if it hasn't been set already
    if (!HasExpectedReferenceOnPeer())
    {
        m_bitFields.hasExpectedReferenceOnPeer = TRUE;
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_SetExpectedReferenceOnPeer(this));
    }

    return S_OK;

}

//------------------------------------------------------------------------
//
//  Clear expected reference on peer, which was set to protect it from GC
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CDependencyObject::ClearExpectedReferenceOnPeer()
{
    // Remove expected reference on peer
    if (HasExpectedReferenceOnPeer())
    {
       m_bitFields.hasExpectedReferenceOnPeer = FALSE;
       IFC_RETURN(FxCallbacks::FrameworkCallbacks_ClearExpectedReferenceOnPeer(this));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
// Peer released the expected reference on itself during shutdown, so clear
// the flag.
// This special case is needed because peer has been disconnected during
// shutdown, so it cannot be called to clear the expected reference.
//
//------------------------------------------------------------------------

void CDependencyObject::OnClearedExpectedReferenceOnPeer()
{
    m_bitFields.hasExpectedReferenceOnPeer = FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This is the generic RENDERCHANGEDPFN for all DependencyObject changes that
//      affect rendering.
//
//------------------------------------------------------------------------
/*static */ void
CDependencyObject::NWSetRenderDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    //ASSERT(!pTarget->OfTypeByIndex<KnownTypeIndex::UIElement>());

    // We support NWSetRenderDirty on UIElement for the case of changes to
    // the Typography.* attached properties. Since these properties are defined
    // on CDependencyObject, they can only choose NWSetRenderDirty.
    // In this case NWPropagateInheritedDirtyFlag will correctly call
    // NWSetContentDirty on all affected UIElements.
    // In other cases, UIElements need to have registered a more specific
    // handler for all changes that propagate to them in order for cached
    // composition and other optimizations to work.

    // The basic changed handler does not set any state (and therefore
    // requires no flags to be cleaned each frame).
    pTarget->NWPropagateDirtyFlag(flags | DirtyFlags::Render);
}

// This method propagates dirty flags to the parent DO via its stored dirty flag function.
void CDependencyObject::NWPropagateDirtyFlag(
    DirtyFlags flags
    )
{
    NW_ASSERT_CAN_PROPAGATE_DIRTY_FLAGS

    RENDERCHANGEDPFN renderChangedHandler = NWGetRenderChangedHandlerInternal();

    // Call the dirty flag function for the parent.
    if (renderChangedHandler)
    {
        CDependencyObject *pParent = GetParentInternal(false);
        if (pParent)
        {
            // Handles sub-property changes for inherited property values.
            // If this DO is the value of an inherited DP, dirtiness needs to be propagated
            // down through its parent UIElement's subgraph.
            if (IsValueOfInheritedProperty() && pParent->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                CUIElement *pUIE = static_cast<CUIElement*>(pParent);
                NWPropagateInheritedDirtyFlag(pUIE, flags);
            }

            renderChangedHandler(pParent, flags);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      This method propagates dirty flags to all the UIElements in pParent's subgraph.
//      It's intended to be used for inherited property changes only.
//
//------------------------------------------------------------------------
void
CDependencyObject::NWPropagateInheritedDirtyFlag(
    _In_ CUIElement *pParent,
    DirtyFlags flags
    )
{
    // TODO: MERGE: This approach is problematic for a number of reasons, but is overall no worse than the old walk.
    // TODO:     1. It over-invalidates FrameworkElements in the subgraph that don't care about this specific inherited property.
    // TODO:     2. It doesn't call the changed handler for non-UIElement DOs, e.g. for TextElements if FontSize
    // TODO:         changes.  This is okay for now since they don't register specific changed handlers, only UIElements
    // TODO:         really need the dirty flag updates, and the change handler will be called on the FE ancestor of the TextElements.
    // TODO:     3. The same handler is called on all UIElement's.  If TextBlock.Foreground and Control.Foreground
    // TODO:         registered different handlers in the future, this code would need to be modified to be aware of
    // TODO:         their relationship as different DPs that share inherited storage space in order to call the corresponding function.
    // TODO:         The same is true if they register a handler for a type more specific than CUIElement.
    // TODO:    4. This code is only called from the base DO and multi-parent DO propagation methods.  If collections, MediaElements
    // TODO:         or any future class that overrides NWPropagateDirtyFlag can be the value of an inherited property, this must
    // TODO:         be called from their overrides as well.

    CUIElementCollection *pChildCollection = static_cast<CUIElementCollection*>(pParent->GetChildren());

    if (pChildCollection)
    {
        for (XUINT32 i = 0; i < pChildCollection->GetCount(); i++)
        {
            CUIElement *pChild = static_cast<CUIElement*>(pChildCollection->GetItemImpl(i));

            if (pChild)
            {
                // TODO: MERGE: It would be better for the initial property change to pass a handler in, but for shareable DOs
                // TODO:    the object can only tell that it is the value of an inherited property, not which parent's property
                // TODO:    is inheritable, so it can't tell which parent and associated handler to propagate through.  If the object
                // TODO:    were set as a non-inherited property of a second parent (with a more specific handler), that
                // TODO:    handler wouldn't be valid to call on all the other UIElements in the subgraph (e.g. a brush set as
                // TODO:    TextBlock.Foreground which is inherited, and also set as Shape.Fill, which is not, shouldn't use the
                // TODO:    Fill handler).
                // The handler only needs to be called on elements that deal with inherited properties,
                // so that other UIElements will not be impacted.

                if (pChild->HasInheritedProperties())
                {
                    CUIElement::NWSetContentDirty(pChild, flags);
                }

                NWPropagateInheritedDirtyFlag(pChild, flags);
            }
        }
    }
}

// Registers the object for animation mapping.
_Check_return_ HRESULT CDependencyObject::SetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
    if (   compositionReason == CompositionRequirement::IndependentAnimation
        || compositionReason == CompositionRequirement::IndependentManipulation
        || compositionReason == CompositionRequirement::IndependentClipManipulation)
    {
        ASSERT(m_bitFields.m_independentTargetCount < ((1 << DependencyObjectBitFields::c_independentCounterBits) - 2));

        m_bitFields.m_independentTargetCount++;

        if (m_bitFields.m_independentTargetCount == 1)
        {
            // Mark this DO as dirty so that it gets rendered again and its new clone can get picked up.
            // CUIElement's override will set the correct element dirty flags.
            // TODO: Delete this. We seem to be hitting a bounds bug
            // see Bug 8145194:CDependencyObject::Set/UnsetRequiresComposition shouldn't dirty the object anymore
            CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        }
    }

    return S_OK;
}

// Unregisters the object for animation mapping.
void CDependencyObject::UnsetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
    if (   compositionReason == CompositionRequirement::IndependentAnimation
        || compositionReason == CompositionRequirement::IndependentManipulation
        || compositionReason == CompositionRequirement::IndependentClipManipulation)
    {
        ASSERT(m_bitFields.m_independentTargetCount > 0);
        m_bitFields.m_independentTargetCount--;

        if (m_bitFields.m_independentTargetCount == 0)
        {
            // Mark this DO as dirty since it no longer has a persistent clone.
            // CUIElement's override will set the correct element dirty flags.
            // TODO: Delete this. We seem to be hitting a bounds bug
            // see Bug 8145194:CDependencyObject::Set/UnsetRequiresComposition shouldn't dirty the object anymore
            CDependencyObject::NWSetRenderDirty(this, DirtyFlags::Render);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   OnCreateAutomationPeerInternal
//
//  Synopsis:  Creates an CAutomationPeer by checking each override
//
//------------------------------------------------------------------------
CAutomationPeer*
CDependencyObject::OnCreateAutomationPeerInternal()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CAutomationPeer* pAP = NULL;
    bool isPeerAvailable = false;

    // Start participating because peer is becoming stateful by
    // keeping a reference to the created automation peer.
    IFC(SetParticipatesInManagedTreeDefault());

    // Ensure that peer exists, for automation peer usage.
    // This can be called during tear-down of a DO, when stateful peer has already
    // been released and cannot be re-created, so use TryEnsureManagedPeer.
    IFC(TryEnsureManagedPeer(
               &isPeerAvailable,
               FALSE /* fPegNoRef */,
               FALSE /* fPegRef */,
               FALSE /* isShutdownException */,
               NULL  /*pfPegged */ ));

    // we need to have HasManagedPeer checking for the UIElements which never
    // be exists on the Managed side. Ex. CPopupRoot
    if (HasManagedPeer())
    {
        IFC(FxCallbacks::UIElement_OnCreateAutomationPeer(this, &pAP));
    }

    AddRefInterface(pAP);

Cleanup:
    return pAP;
}


//------------------------------------------------------------------------
//
//  Called to make sure the text core properties for this element are
//  present and up to date.
//
//  If m_pTextFormatting is null, allocates and initialises one to
//  default values.
//
//  If IsGetValue is TRUE, goes on to make sure that m_pTextFormatting
//  is up to date, by getting any non-local values from the parent. (Makes
//  sure the parent is up to date first.)
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::EnsureTextFormatting(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pProperty,
    _In_     bool forGetValue
)
{
    TextFormatting **ppTextFormatting = pObject->GetTextFormattingMember();
    RRETURN(pObject->UpdateTextFormatting(pProperty, forGetValue, ppTextFormatting));
}


//------------------------------------------------------------------------
//
//  Worker for EnsureTextFormatting that handles non-trivial cases: where
//  the TextFormatting member needs creating, or where its inheritance
//  is out of date.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::UpdateTextFormatting(
    _In_opt_ const CDependencyProperty  *pProperty,
    _In_     bool                  forGetValue,
    _In_     TextFormatting      **ppTextFormatting
)
{
#if DBG
    InheritedProperties::RecordUpdateEnsureTextFormatting();
#endif

    if (pProperty != NULL)
    {
        ASSERT(pProperty->IsInherited());
        ASSERT(pProperty->IsStorageGroup());
    }

    IFCEXPECT_ASSERT_RETURN(ppTextFormatting != NULL);

    if (*ppTextFormatting == NULL)
    {
        // Create a new TextFormatting, initialised to default values
        IFC_RETURN(TextFormatting::Create(GetContext(), ppTextFormatting));
#if DBG
        InheritedProperties::RecordCreationEnsureTextFormatting();
#endif
    }

    if (    forGetValue
        &&  (*ppTextFormatting)->IsOld()
        &&  (    pProperty == NULL
             ||  IsPropertyDefault(pProperty)))
    {
        // Our client wants to read the value of pProperty, pProperty is not
        // set locally in this element, and text formatting inheritance is
        // out of date.
        // TRACE(TraceAlways, L"Pulling inherited properties from parent.");
        IFC_RETURN(PullInheritedTextFormatting());
#if DBG
        InheritedProperties::RecordPulledEnsureTextFormatting();
#endif
    }

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Returns the text core properties applying to this element's parent.
//
//  Note:
//      If the parent element does not yet have a text core properties
//      storage group, it won't create one, instead it finds one by walking
//      up the tree.
//
//      If the parent element does already have a text core properties
//      storage group, it calls EnsureTextFormatting to make sure it is up
//      to date.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::GetParentTextFormatting(
    _Outptr_ TextFormatting **ppTextFormatting
)
{
    CDependencyObject *pInheritanceParent = GetInheritanceParentInternal();

    ReleaseInterface(*ppTextFormatting);

    while (*ppTextFormatting == NULL  &&  pInheritanceParent != NULL)
    {
        TextFormatting **ppParentLocalFormatting = pInheritanceParent->GetTextFormattingMember();
        if (ppParentLocalFormatting != NULL  &&  *ppParentLocalFormatting != NULL)
        {
            // The parent can and does have locally set text formatting
            // properties.
            // Make sure they are all up to date.
            IFC_RETURN(EnsureTextFormatting(pInheritanceParent, NULL, TRUE));
            // Return a reference to our parents formatting properties.
            *ppTextFormatting = *ppParentLocalFormatting;
            AddRefInterface(*ppTextFormatting);
        }
        else
        {
            // Our parent has no locally set text formatting properties.
            // (For example it may be a TextElement or FrameworkElement
            // for which no text formatting properties have yet been
            // set, or it may not be a TextElement or FrameworkElement
            // at all.)
            // Continue looping to get our grandparents text formatting.
            pInheritanceParent = pInheritanceParent->GetInheritanceParentInternal();
        }
    }

    if (*ppTextFormatting == NULL)
    {
        // We were unable to find any parent with locally set formatting,
        // so return default formatting.
        ASSERT(pInheritanceParent == NULL);
        CTextCore *pTextCore = NULL;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        IFC_RETURN(pTextCore->GetDefaultTextFormatting(ppTextFormatting));
    }

    return S_OK;
}


// Determine whether the nearest parent with a set flow direction is
// Right to left.
//
// Note that elements such as FrameworkElement and Run which can have
// locally set FlowDirection values override this implementation.
//
// Here we provide a tree walk implementation used by elements that
// do not have their own local FlowDirection properties.
bool CDependencyObject::IsRightToLeft()
{
    CDependencyObject *pParent = GetInheritanceParentInternal();
    if (pParent != NULL)
    {
        return pParent->IsRightToLeft();
    }
    else
    {
        return false;
    }
}


//------------------------------------------------------------------------
//
//  Implementation of inherited attached property storage group inheritance.
//
//  Returns with a guaranteed non-null m_pInheritedProperties as follows:
//
//    o  It may point to a locally owned m_pInheritedProperties, or a parents
//       m_pInheritedProperties if there are no locally set values.
//    o  The m_pInheritedProperties will be up-to-date.
//
//  Note: Do not use this to prepare for writing a property, call
//  EnsureInheritedProperties(object, NULL, FALSE) instead.
//
//  EnsureInheritedProperties will call this method, then take a copy of the
//  properties if required.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::UpdateInheritedPropertiesForRead()
{
    //InheritedProperties *pInheritedProperties = NULL;
    CDependencyObject   *pParent              = GetInheritanceParentInternal();

    if (m_pInheritedProperties == NULL)
    {
        if (pParent != NULL)
        {
            // Recursively get the parent ready for read, then take a read
            // only reference to its InheritedProperties properties.
            IFC_RETURN(pParent->EnsureInheritedPropertiesForRead());
            m_pInheritedProperties = pParent->m_pInheritedProperties;
            AddRefInterface(m_pInheritedProperties);
        }
        else
        {
            // We've reached the root. Give the root  a copy of the default
            // InheritedProperties properties, make the root its writer and set
            // it up to date.
            IFC_RETURN(InheritedProperties::CreateDefault(GetContext(), &m_pInheritedProperties));
            m_pInheritedProperties->m_pWriter = this;
            m_pInheritedProperties->SetIsUpToDate();
        }
    }
    else if (m_pInheritedProperties->IsOld())
    {
        // Make sure m_pInheritedProperties is up to date
        if (pParent == NULL)
        {
            // Leave is not guaranteed to be called, hence it is possible to get inherited properties cache
            // out of sync: parent is NULL and m_pInheritedProperties->m_pWriter != this.
            // In this particular case perform cleanup on inherited properties and
            // initialize inherited properties to defaults, as in the root element case.
            if (m_pInheritedProperties->m_pWriter != this)
            {
                DisconnectInheritedProperties();
                IFC_RETURN(InheritedProperties::CreateDefault(GetContext(), &m_pInheritedProperties));
                m_pInheritedProperties->m_pWriter = this;
                m_pInheritedProperties->SetIsUpToDate();
            }
            else
            {
                // There's no parent. This means that any non-locally set
                // InheritedProperties properties are defaults and therefore
                // up-to-date, and the locally set properties are also up-to-date
                // implicitly.
                // So all we need t do is to mark this InheritedProperties as
                // up-to-date.
                m_pInheritedProperties->SetIsUpToDate();
            }
        }
        else
        {
            // We have a parent.
            // First make sure our parent is up to date.
            IFC_RETURN(pParent->EnsureInheritedPropertiesForRead());
            ASSERT(pParent->m_pInheritedProperties != NULL);

            if (m_pInheritedProperties->m_pWriter == this)
            {
                // We own our own copy of m_pInheritedProperties.
                // Now copy across any non-local values.
                IFC_RETURN(m_pInheritedProperties->CopyNonLocallySetProperties(
                    pParent->m_pInheritedProperties
                ));
                m_pInheritedProperties->SetIsUpToDate();
            }
            else
            {
                // We don't own our m_pInheritedProperties.
                // Release it to whoever does own it, and replace it with
                // a reference to our parents m_pInheritedProperties.
                DisconnectInheritedProperties();
                m_pInheritedProperties = pParent->m_pInheritedProperties;
                AddRefInterface(m_pInheritedProperties);
            }
        }
    }
    else if (m_pInheritedProperties->m_pWriter != this)
    {
        // We have a pointer to an up to date InheritedProperties owned by one
        // of our parents.
        // Check that no intermediate parent has equally up to date locally set
        // values.

        // Performance note:
        // This code runs frequently: The majority of DOs with
        // m_pInheritedProperties have no locally set inherited property values,
        // and so come through here.
        // The inheritance code could be simplified, and the following loop
        // saved by moving the generation counter from within
        // m_pInheritedProperties to beside it, thus keeping a separate
        // generation counter on each DO.
        //  Thus out of date m_pInheritedProperties pointers would be
        // immediately detected without the need for this parent walk.

        // Find closest parent with local settings

        while (    pParent != NULL
               &&  (    pParent->m_pInheritedProperties == NULL
                    ||  pParent->m_pInheritedProperties->m_pWriter != pParent))
        {
            pParent = pParent->GetInheritanceParentInternal();
        }

        if (    pParent != NULL
            &&  m_pInheritedProperties->m_pWriter != pParent)
        {
            // We've found a closer locally updated parent to refer to.
            //TRACE(TraceAlways, L"Switching m_pInheritedProperties to a closer parent.");
            IFC_RETURN(pParent->EnsureInheritedPropertiesForRead());
            DisconnectInheritedProperties();
            m_pInheritedProperties = pParent->m_pInheritedProperties;
            AddRefInterface(m_pInheritedProperties);
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Called to make sure the InheritedProperties properties for this element are
//  present and up to date.
//
//  Returns with a guaranteed non-null m_pInheritedProperties as follows:
//
//    o  If it's for read access it may point to a locally owned
//       m_pInheritedProperties, or a parents m_pInheritedProperties, or to the
//       default InheritedProperties.
//    o  If it's for write access it will point to a InheritedProperties that
//       belongs specifically to pObject (i.e. the InheritedProperties'
//       m_pWriter field points to pObject).
//    o  If this is for write, we don't bother to bring m_pInheritedProperties
//       up to date.
//    o  If this is for read, pProperty has been passed to identify a specific
//       property, and the specific property has been set locally on pObject,
//       then we don't bother to bring m_pInheritedProperties up to date.
//    o  In all other cases we make sure m_pInheritedProperties is up to date by
//       checking its generation counter and calling
//       PullInheritedProperties if it's old.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CDependencyObject::EnsureInheritedProperties(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pProperty,
    _In_     bool forGetValue
)
{
    if (    forGetValue
        &&  pObject->m_pInheritedProperties != NULL
        &&  pObject->m_pInheritedProperties->m_pWriter == pObject
        && !pObject->m_pInheritedProperties->IsOld())
    {
        return S_OK;
    }

    if (pProperty != NULL)
    {
        ASSERT(pProperty->IsInheritedAttachedPropertyInStorageGroup());

        // Handle fast special case for read - if it's a property that has been
        // set locally on this object we don't need to do any inheritance work.
        if (    pObject->m_pInheritedProperties != NULL
            &&  pObject->m_pInheritedProperties->m_pWriter == pObject
            &&  pObject->m_pInheritedProperties->IsPropertyFlagSet(pProperty, InheritedPropertyFlag::IsSetLocally))
        {
            return S_OK;
        }
    }


    // The fast case didn't work, we need to run inheritance.

    IFC_RETURN(pObject->EnsureInheritedPropertiesForRead());


    // If this is for writing, make sure we own m_pInheritedProperties by copying it if
    // necessary.

    if (    !forGetValue
        &&  !(pObject->m_pInheritedProperties->m_pWriter == pObject))
    {
        // We need to own the m_pInheritedProperties to be allowed to write to it.
        // Take a copy.
        InheritedProperties *pNew = NULL;
        // Create a writeable copy.
        IFC_RETURN(InheritedProperties::CreateCopy(pObject, pObject->m_pInheritedProperties, &pNew));
        // Release the InheritedProperties we previously had r/o access to.
        pObject->DisconnectInheritedProperties();
        pObject->m_pInheritedProperties = pNew;
        pNew = NULL;
    }

    return S_OK;
}

// Required because this is stubbed out in unit tests
Theming::Theme CDependencyObject::GetRequestedThemeForSubTreeFromCore()
{
    return GetContext()->GetRequestedThemeForSubTree();
}

void CDependencyObject::SetRequestedThemeForSubTreeOnCore(Theming::Theme requestedTheme)
{
    GetContext()->SetRequestedThemeForSubTree(requestedTheme);
}

// Notifies us that a DP's value is going to be set from a ThemeResourceExpression in the framework.
// These methods will make sure that when the expression tries to update the effective value, we
// preserve whether the effective value is a modifier value (e.g. animated value), or a base value.
void CDependencyObject::BeginPropertyThemeUpdate(_In_ const CDependencyProperty* pDP)
{
    // Mark if a modified value is being set, so
    // CDependencyObject::SetValue can differentiate between
    // a base value or a modified value being set.
    auto modifiedValue = GetModifiedValue(pDP);
    if (modifiedValue && modifiedValue->HasModifiers())
    {
        modifiedValue->SetModifierValueBeingSet(true);
    }
}

void CDependencyObject::EndPropertyThemeUpdate(_In_ const CDependencyProperty* pDP)
{
    auto modifiedValue = GetModifiedValue(pDP);
    if (modifiedValue && modifiedValue->HasModifiers())
    {
        modifiedValue->SetModifierValueBeingSet(false);
    }
}

//------------------------------------------------------------------------
//
//  Method: GetNamedSelfOrParent
//
//  Synopsis:
//      Starting from the specified node itself, iterate up to its parents
//      until we find a named element parent of the requested type.
//
//------------------------------------------------------------------------

const CDependencyProperty* CDependencyObject::GetPropertyByIndexInline(_In_ KnownPropertyIndex nIndex) const
{
    const CDependencyProperty *pProperty = MetadataAPI::GetDependencyPropertyByIndex(nIndex);

    // Attached inherited storage group properties are nominally on
    // CDependencyObject but are not allocated slots there to avoid
    // the performance impact.
    // However they are recorded in m_pKnownPropertyIndicesMap, so look
    // there.

    if (pProperty)
    {
        // Check that it really is an attached inherited storage group property.
        // then ensure that the property is actually a property on the passed in class.
        if (!pProperty->IsInheritedAttachedPropertyInStorageGroup() &&
            !MetadataAPI::IsAssignableFrom(pProperty->GetTargetType()->m_nIndex, GetClassInformation()->m_nIndex))
        {
            pProperty = nullptr;
        }
    }

    return pProperty;
}

_Check_return_ HRESULT CDependencyObject::EnsureTextFormattingForRead()
{
    TextFormatting **ppTextFormatting = GetTextFormattingMember();

    if (      ppTextFormatting == NULL
        ||   *ppTextFormatting == NULL
        ||  (*ppTextFormatting)->IsOld())
    {
        RRETURN(UpdateTextFormatting(NULL, TRUE, ppTextFormatting));
    }
    else
    {
        RRETURN(S_OK);
    }
}

_Check_return_ HRESULT CDependencyObject::EnsureInheritedPropertiesForRead()
{
    if (    m_pInheritedProperties != NULL
        &&  m_pInheritedProperties->m_pWriter == this
        && !m_pInheritedProperties->IsOld())
    {
        RRETURN(S_OK);
    }
    else
    {
        RRETURN(UpdateInheritedPropertiesForRead());
    }
}

void CDependencyObject::ContextAddRef()
{
// Only AddRef the core if we haven't...
    if (m_bitFields.fNeedToReleaseCore == FALSE)
    {
        m_bitFields.fNeedToReleaseCore = TRUE;
        XcpIncPartTimeStrongPointer(m_pCorePartTimeStrongPointerContext);
        AddRefInterface(GetContext());
    }
}

void CDependencyObject::ContextRelease()
{
    if (m_bitFields.fNeedToReleaseCore)
    {
        m_bitFields.fNeedToReleaseCore = FALSE;
        XcpDecPartTimeStrongPointer(m_pCorePartTimeStrongPointerContext);
        ReleaseInterfaceNoNULL(GetContext());
    }
}

#if DBG

void CDependencyObject::PreOnManagedPeerCreated()
{
    // This flag will get set again in CDependencyObject::OnManagedPeerCreated
    m_fCalledOnManagedPeerCreated = false;
}

void CDependencyObject::PostOnManagedPeerCreated()
{
    // Ensure that CDependencyObject::OnManagedPeerCreated was called, even if overridden.
    ASSERT( m_fCalledOnManagedPeerCreated );
}

#endif

void CDependencyObject::TracePropertyChanged(_In_ const CDependencyProperty *pdp, _In_ const CValue* newValue)
{
    auto pUIElement = do_pointer_cast<CUIElement>(this);
    if (pUIElement)
    {
        auto propertyName = pdp->GetName();
        CUIElement* pParent = GetUIElementParentInternal();

        RENDERCHANGEDPFN pfnRenderChanged = pdp->GetRenderChangedHandler();

        XUINT32 affects = LayoutCausality::None;
        if (pUIElement->GetIsMeasureDirty() || pUIElement->GetIsArrangeDirty())
        {
            affects |= LayoutCausality::ElementAlreadyDirty;
        }

        // Check if the DependencyProperty affects the layout
        if (pdp->AffectsArrange() || pdp->AffectsMeasure())
        {
            affects |= LayoutCausality::Layout;
        }
        // Check if Dependency Property causes a render walk
        if (pfnRenderChanged != nullptr)
        {
            affects |= LayoutCausality::Render; // 0x4 is affects Render
        }

        if ((affects & LayoutCausality::Layout)||(affects & LayoutCausality::Render)) // only care if property affects layout or render
        {
            TracePropertyChangedInfo1(
                reinterpret_cast<UINT64>(this),
                reinterpret_cast<UINT64>(static_cast<CDependencyObject*>(pParent)),
                propertyName.GetBuffer(),
                affects,
                CValueUtil::GetValueOrAddressAsUINT64(*newValue),
                newValue->AsString().GetBuffer(),
                newValue->AsDouble()
               );
        }
    }
}

bool CDependencyObject::TryGetSourceInfoFromPeer(
    _Out_ UINT32* line,
    _Out_ UINT32* column,
    _Out_ xstring_ptr* uri,
    _Out_ xstring_ptr* hash) const
{
    if (!line || !column || !uri || !hash)
    {
        return false;
    }

    *uri = xstring_ptr::NullString();
    *hash = xstring_ptr::NullString();
    *line = 0;
    *column = 0;

    // Source information is only stored on the peer so if there is no peer we should return false
    if (!HasManagedPeer())
    {
        return false;
    }

    // If we have a managed peer we can query interface for a ISourceInfoPrivate. Unfortunately this requires a reinterpret_cast
    // to cast the peer to an IUnknown since because the compiler doesn't think the object can be casted due to the forward declaration
    // of DependencyObject.
    ctl::ComPtr<xaml::ISourceInfoPrivate> spSourceInfo;
    if (FAILED(reinterpret_cast<IUnknown*>(GetDXamlPeer())->QueryInterface<xaml::ISourceInfoPrivate>(spSourceInfo.GetAddressOf())))
    {
        return false;
    }

    int sourceColumn = 0;
    int sourceLine = 0;
    wrl_wrappers::HString sourceUri;
    wrl_wrappers::HString sourceHash;

    if (FAILED(spSourceInfo->get_Column(&sourceColumn)) ||
        FAILED(spSourceInfo->get_Line(&sourceLine)) ||
        FAILED(spSourceInfo->get_ParseUri(sourceUri.GetAddressOf())) ||
        FAILED(spSourceInfo->get_XbfHash(sourceHash.GetAddressOf())))
    {
        return false;
    }

    if (FAILED(xstring_ptr::CloneRuntimeStringHandle(sourceUri.Get(), uri)))
    {
        return false;
    }
    if (FAILED(xstring_ptr::CloneRuntimeStringHandle(sourceHash.Get(), hash)))
    {
        return false;
    }
    *line = static_cast<UINT>(sourceLine);
    *column = static_cast<UINT>(sourceColumn);

    return true;
}

DependencyObjectDCompRegistry* CDependencyObject::GetDCompObjectRegistry() const
{
    auto core = GetContext();

    if (core != nullptr
        && core->m_pNWWindowRenderTarget != nullptr
        && core->m_pNWWindowRenderTarget->GetDCompTreeHost() != nullptr)
    {
        return core->m_pNWWindowRenderTarget->GetDCompTreeHost()->GetDCompObjectRegistry();
    }

    return nullptr;
}

CDeferredStorage& CDependencyObject::EnsureAndGetScopeDeferredStorage()
{
    return CDeferredMapping::EnsureScopeDeferredStorage(this);
}

_Check_return_
HRESULT
CDependencyObject::GetAnnotations(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    xref_ptr<CDependencyObject>& annotations = pObject->EnsureAutomationAnnotationsStorage();

    if (!annotations)
    {
        CREATEPARAMETERS cp(pObject->GetContext());
        CDependencyObject* annotationsTemp = nullptr;
        IFC_RETURN(CAutomationAnnotationCollection::Create(&annotationsTemp, &cp));
        annotations.attach(annotationsTemp);
        annotationsTemp = nullptr;

        IFC_RETURN(static_cast<CAutomationAnnotationCollection*>(annotations.get())->SetOwner(pObject));

        // Enter the new collection just like we do for other implicit collections
        EnterParams params = EnterParams(
            /*isLive*/                pObject->IsActive(),
            /*skipNameRegistration*/  FALSE,
            /*coercedIsEnabled*/      pObject->GetCoercedIsEnabled(),
            /*useLayoutRounding*/     pObject->GetUseLayoutRounding(),
            /*visualTree*/            VisualTree::GetForElementNoRef(pObject, LookupOptions::NoFallback)
            );
        IFC_RETURN(annotations.get()->Enter(pObject->GetStandardNameScopeOwner(), params));
    }

    pResult->SetObjectAddRef(annotations.get());

    return S_OK;
}

CTimeManager* CDependencyObject::GetTimeManager()
{
    return GetContext()->GetTimeManager();
}

_Check_return_ HRESULT
CDependencyObject::ValidateStrictnessOnProperty(_In_ const CPropertyBase* prop)
{
    if (prop->IsNonStrictOnly())
    {
        return NonStrictOnlyApiCheck(prop->GetName().GetBuffer());
    }
    if (prop->IsStrictOnly())
    {
        return StrictOnlyApiCheck(prop->GetName().GetBuffer());
    }

    return DefaultStrictApiCheck();
}

_Check_return_ HRESULT CDependencyObject::DefaultStrictApiCheck()
{
    // The default check applies to all APIs that have no Strictness attribute specified in codegen.
    // The policy is:
    // Strict Types:  Banned
    // Legacy Types:  Allowed
#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (IsStrictType())
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_ACCESSDENIED, ERROR_ACCESSING_NON_STRICT_API_ON_STRICT_TYPE));
    }
#endif
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::StrictOnlyApiCheck(const WCHAR* apiName)
{
    // The StrictOnly check applies to all APIs that have a Strictness attribute explicitly set to StrictOnly
    // The policy is:
    // Strict Types:  Allowed
    // Legacy Types:  Allowed only if object has turned on strict mode
    // This function enforces the policy, and also auto-promotes the object into StrictOnly if necessary.

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (IsStrictType())
    {
        return S_OK;
    }
#endif

    switch(GetObjectStrictness())
    {
    case ObjectStrictness::Agnostic:
        // Promote the object - it is now locked into StrictOnly mode.
        SetObjectStrictness(ObjectStrictness::StrictOnly);
        break;
    case ObjectStrictness::NonStrictOnly:
        {
            // Run through all the non-strict-only properties, if none of them are in use (eg set to non-default, animating, etc)
            // we can auto-promote the object to StrictOnly mode.
            const WCHAR* searchResult = FindFirstNonStrictOnlyPropertyInUse();
            if (searchResult == nullptr)
            {
                SetObjectStrictness(ObjectStrictness::StrictOnly);
            }
            else
            {
                // There is some non-strict-only property in use, fail.
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(
                    E_ACCESSDENIED,
                    ERROR_ACCESS_DENIED_WITH_PROPERTY_IN_USE,
                    xephemeral_string_ptr(apiName, static_cast<XUINT32>(wcslen(apiName))),
                    searchResult));
            }
            break;
        }
    case ObjectStrictness::StrictOnly:
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::NonStrictOnlyApiCheck(const WCHAR* apiName)
{
    // The StrictOnly check applies to all APIs that have a Strictness attribute explicitly set to NonStrictOnly
    // The policy is:
    // Strict Types:  Banned
    // Legacy Types:  Allowed only if object has not turned on strict mode
    // This function enforces the policy, and also auto-promotes the object into NonStrictOnly if necessary.

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (IsStrictType())
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_ACCESSDENIED, ERROR_ACCESSING_NON_STRICT_API_ON_STRICT_TYPE));
    }
#endif

    switch(GetObjectStrictness())
    {
    case ObjectStrictness::Agnostic:
        // Promote the object - it is now locked into NonStrictOnly mode.
        SetObjectStrictness(ObjectStrictness::NonStrictOnly);
        break;
    case ObjectStrictness::NonStrictOnly:
        break;
    case ObjectStrictness::StrictOnly:
        {
            // Run through all the strict-only properties, if none of them are in use (eg set to non-default or animating)
            // we can auto-promote the object to NonStrictOnly mode.
            const WCHAR* searchResult = FindFirstStrictOnlyPropertyInUse();
            if (searchResult == nullptr)
            {
                SetObjectStrictness(ObjectStrictness::NonStrictOnly);
            }
            else
            {
                // There is some strict-only property in use, fail.
                IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingFormattedResourceID(
                    E_ACCESSDENIED,
                    ERROR_ACCESS_DENIED_WITH_PROPERTY_IN_USE,
                    xephemeral_string_ptr(apiName, wcslen(apiName)),
                    searchResult));
            }
            break;
        }
    }

    return S_OK;
}

const WCHAR* CDependencyObject::FindFirstNonStrictOnlyPropertyInUse() const
{
    if (OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Give UIElement a chance to perform UIElement-specific checks which don't rely on the property system.
        auto uielement = static_cast<const CUIElement*>(this);
        auto result = uielement->FindFirstUIElementNonStrictOnlyPropertyInUse();
        if (result != nullptr)
        {
            return result;
        }
    }

    // Run through all the non-strict-only properties and detect if any of these properties are in use.
    const CClassInfo* classInfo = GetClassInformation();
    for (const CPropertyBase* propertyBase = classInfo->GetFirstProperty();
         propertyBase->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty;
         propertyBase = propertyBase->GetNextProperty())
    {
        if (propertyBase->IsNonStrictOnly())
        {
            const CDependencyProperty* dp = propertyBase->AsOrNull<CDependencyProperty>();
            if (dp != nullptr)
            {
                BaseValueSource source = GetBaseValueSource(dp);
                if (source !=  BaseValueSource::BaseValueSourceDefault)
                {
                    // The property was locally set, styled, etc
                    return propertyBase->GetName().GetBuffer();
                }
                std::shared_ptr<CModifiedValue> modifiedValue = GetModifiedValue(dp);
                if (modifiedValue != nullptr && modifiedValue->IsAnimated())
                {
                    // The property is currently being animated
                    return propertyBase->GetName().GetBuffer();
                }
            }
        }
    }

    return nullptr;
}

const WCHAR* CDependencyObject::FindFirstStrictOnlyPropertyInUse() const
{
    if (OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Give UIElement a chance to perform UIElement-specific checks which don't rely on the property system.
        auto uielement = static_cast<const CUIElement*>(this);
        auto result = uielement->FindFirstUIElementStrictOnlyPropertyInUse();
        if (result != nullptr)
        {
            return result;
        }
    }
    // Facades_TODO:  Add specific checks for Brush types

    // Run through all the strict-only properties and detect if any of these properties are in use.
    // Note that the property system doesn't understand facade animations, this was detected above by UIElement
    const CClassInfo* classInfo = GetClassInformation();
    for (const CPropertyBase* propertyBase = classInfo->GetFirstProperty();
         propertyBase->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty;
         propertyBase = propertyBase->GetNextProperty())
    {
        if (propertyBase->IsStrictOnly())
        {
            if (propertyBase->Is<CSimpleProperty>())
            {
                if (!SimpleProperties::IsSimplePropertySetToDefault(propertyBase->GetIndex(), this))
                {
                    return propertyBase->GetName().GetBuffer();
                }
            }
            else
            {
                ASSERT(propertyBase->Is<CDependencyProperty>());
                if (!IsPropertyDefault(MetadataAPI::GetPropertyByIndex(propertyBase->GetIndex())))
                {
                    return propertyBase->GetName().GetBuffer();
                }
            }
        }
   }

    return nullptr;
}

VisualTree* CDependencyObject::GetVisualTree()
{
    auto visualTreeWeak = m_sharedState->Value().GetVisualTree();
    if (VisualTree* visualTree = visualTreeWeak.lock_noref())
    {
        return visualTree;
    }

    return nullptr;
}

void CDependencyObject::SetVisualTree(_In_ VisualTree* visualTree)
{
    if (DoesAllowMultipleAssociation())
    {
        // This kind of element can be associated with multiple trees, so we leave the VisualTree pointer
        // null.  We do support sharing sharable DOs between XamlIslands, because we want elements from
        // different trees to both reference the Application-wide ResourceDictionary.
        ASSERT(GetVisualTree() == nullptr);
    }
    else if (OfTypeByIndex<KnownTypeIndex::XamlIslandRootCollection>())
    {
        // CXamlIslandRootCollection does not have a VisualTree pointer, because it's never really in a tree.
        // CRootVisuals and CXamlIslandRoots each define their own VisualTree, and a CXamlIslandRootCollection
        // is really just a holder of CXamlIslandRoots.  When we delete CXamlIslandRootCollection this logic
        // gets simpler.
        ASSERT(GetVisualTree() == nullptr);
    }
    else
    {        
        if (IsActive())
        {
            // We don't expect an element to change trees while active. The exception is flyouts, on which Enter
            // is called early in a way that can cause them to be associated with the first XamlRoot's VisualTree.
            ASSERT(GetVisualTree() == nullptr ||
                visualTree == GetVisualTree() ||
                OfTypeByIndex<KnownTypeIndex::FlyoutBase>());
        }

        if (visualTree != GetVisualTree())
        {
            const auto& current = m_sharedState->Value();
            auto core = GetContext();
            m_sharedState = Flyweight::Create<CDOSharedState>(
                core,
                core,
                current.GetRenderChangedHandler(),
                current.GetBaseUri(),
                xref::get_weakref(visualTree));
        }
    }
}

CDependencyObject* CDependencyObject::GetPublicRootVisual()
{
    if (VisualTree* visualTree = VisualTree::GetForElementNoRef(this))
    {
        return visualTree->GetPublicRootVisual();
    }
    return GetContext()->getVisualRoot();
}

// Stores a WarningContext with the provided type and info. This DependencyObject's instance pointer and type index are added to the string array
// included in the memory dump, should it be created.
bool CDependencyObject::StoreWarningContext(WarningContextLog::WarningContextType type, _In_ std::vector<std::wstring>& warningInfo, size_t framesToSkip)
{
    wchar_t instance[32];
    std::swprintf(instance, std::size(instance), L"Instance: 0x%p", this);
    warningInfo.push_back(instance);

    std::wstring typeIndex(L"TypeIndex: ");
    typeIndex.append(std::to_wstring(static_cast<uint16_t>(GetTypeIndex())));
    warningInfo.push_back(std::move(typeIndex));

    return OnWarningEncountered(type, &warningInfo, framesToSkip);
}
