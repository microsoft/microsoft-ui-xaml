// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "xstring_ptr.h"
#include "ThemeResource.h"
#include "theming\inc\Theme.h"
#include "VisualTree.h"
#include "DependencyObject.h"
// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif
#include "CDependencyObject.h"
#include "CDOAssociativeImpl.h"


namespace DirectUI
{
    DOUBLE GetModernDoubleValue(int key)
    {
        ASSERT(false);
        return 0;
    }

    CDependencyObject* DependencyObject::GetHandle() const
    {
        return nullptr;
    }
}

UINT GetDefaultSelectionHighlightColor()
{
    ASSERT(false);
    return 0;
}

CDependencyObject::~CDependencyObject()
{
}

// Not thread safe.
void CDependencyObject::AddRefImpl(UINT32 cRef)
{
}

void CDependencyObject::ReleaseImpl(UINT32 cRef)
{
    if (cRef == 0)
    {
        ResetReferencesFromChildren();
        delete this;
    }
}

void CDependencyObject::ContextRelease()
{
}

bool CDependencyObject::IsSettingValueFromManaged(_In_ CDependencyObject* obj) const
{
    return false;
}

Theming::Theme CDependencyObject::GetRequestedThemeForSubTreeFromCore()
{
    return Theming::Theme::None;
}

void CDependencyObject::SetRequestedThemeForSubTreeOnCore(Theming::Theme)
{
}

void CDependencyObject::ClearThemeResource(_In_ const CDependencyProperty *pdp, _Out_opt_ xref_ptr<CThemeResource>* themeResource)
{
}

CDependencyObject* CDependencyObject::GetStandardNameScopeOwnerInternal(_In_ CDependencyObject* pFirstOwner)
{
    return nullptr;
}

const CDependencyProperty* CDependencyObject::GetPropertyByIndexInline(_In_ KnownPropertyIndex nIndex) const
{
    return nullptr;
}

_Check_return_ HRESULT CDependencyObject::GetDefaultInheritedPropertyValue(_In_ const  CDependencyProperty* dp, _Out_ CValue* value)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyObject::SetName(_In_ const xstring_ptr& strName)
{
    return E_NOTIMPL;
}

CThemeResource* CDependencyObject::GetThemeResourceNoRef(_In_ KnownPropertyIndex ePropertyIndex)
{
    return nullptr;
}

_Check_return_ HRESULT CDependencyObject::OnManagedPeerCreated(XUINT32 fIsCustomDOType, XUINT32 fIsManagedPeerPeggedNoRef)
{
    return E_NOTIMPL;
}

void CDependencyObject::DisconnectManagedPeer()
{
}

_Check_return_ HRESULT CDependencyObject::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyObject::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue
    )
{
    return E_NOTIMPL;
}

_Check_return_ xstring_ptr __thiscall CDependencyObject::GetClassName() const
{
    return xstring_ptr();
}

CCoreServices* CDependencyObject::GetContextInterface() const
{
    return nullptr;
}

_Check_return_ HRESULT CDependencyObject::NotifyInheritanceContextChanged(_In_ InheritanceContextChangeKind kind)
{
    return S_OK;
}

bool CDependencyObject::ShouldReleaseCoreObjectWhenTrackingPeerReference() const
{
    // Until we properly emulate lifetime in unit tests, we cannot safely release
    // core object references that we store in the property system.
    return false;
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
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::AddParent(
    _In_ CDependencyObject *pNewParent,
    bool fPublic,
    _In_opt_ RENDERCHANGEDPFN pfnParentRenderChangedHandler
    )
{
    m_pParent = pNewParent;
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::RemoveParent(_In_ CDependencyObject *pDO)
{
    m_pParent = nullptr;
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::SetParent(
    _In_opt_ CDependencyObject *pNewParent,
    bool fPublic /*= true */,
    _In_opt_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler /*= NULL */
    )
{
    return S_OK;
}

bool CDependencyObject::ParticipatesInManagedTree()
{
    return false;
}

XUINT32 CDependencyObject::ParticipatesInManagedTreeInternal()
{
    return DOESNT_PARTICIPATE_IN_MANAGED_TREE;
}

bool CDependencyObject::IsRightToLeft()
{
    return false;
}

bool CDependencyObject::GetUseLayoutRounding() const
{
    return false;
}

bool CDependencyObject::IsFiringEvents()
{
    return false;
}

_Check_return_ HRESULT CDependencyObject::SetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
    return E_NOTIMPL;
}

void CDependencyObject::UnsetRequiresComposition(
    CompositionRequirement compositionReason,
    IndependentAnimationType detailedAnimationReason
    ) noexcept
{
}

void CDependencyObject::NWPropagateDirtyFlag(DirtyFlags flags)
{
}

_Check_return_ HRESULT CDependencyObject::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::NotifyPropertyChanged(const PropertyChangedParams& args)
{
    return OnPropertyChanged(args);
}

_Check_return_ HRESULT CDependencyObject::RegisterName( _In_ CDependencyObject *pNamescopeOwner, XUINT32 bTemplateNamescope)
{
    return E_NOTIMPL;
}

void CDependencyObject::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
}

_Check_return_ HRESULT CDependencyObject::Enter(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params)
{
    return EnterImpl(namescopeOwner, params);
}

_Check_return_ HRESULT CDependencyObject::EnterImpl(_In_ CDependencyObject* namescopeOwner, _In_ EnterParams params)
{
    return EnterSparseProperties(namescopeOwner, params);
}

_Check_return_ HRESULT CDependencyObject::Leave(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params)
{
    return LeaveImpl(namescopeOwner, params);
}

_Check_return_ HRESULT CDependencyObject::LeaveImpl(_In_ CDependencyObject* namescopeOwner, _In_ LeaveParams params)
{
    return LeaveSparseProperties(namescopeOwner, params);
}

_Check_return_ HRESULT CDependencyObject::InvokeImpl(_In_ const CDependencyProperty* pDP, _In_opt_ CDependencyObject* namescopeOwner)
{
    return S_OK;
}

void CDependencyObject::NotifyParentChange(
    _In_ CDependencyObject *pNewParent,
    _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
    )
{
}

CDependencyObject* CDependencyObject::GetStandardNameScopeParent()
{
    return nullptr;
}

IPALUri* CDependencyObject::GetBaseUri()
{
    return nullptr;
}

_Check_return_ HRESULT CDependencyObject::ClonePropertySetField(_In_ const CDependencyObject *pdoOriginal)
{
    return E_NOTIMPL;
}

bool CDependencyObject::IsCustomType() const
{
    return true;
}

void CDependencyObject::NWSetRenderDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
}

void CDependencyObject::ReferenceTrackerWalk(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
}

bool CDependencyObject::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    return true;
}

_Check_return_ HRESULT
CDependencyObject::OnMultipleAssociationChange(_In_opt_ CDependencyObject *pAssociationOwner)
{
   return E_NOTIMPL;
}

#if DBG

void CDependencyObject::PreOnManagedPeerCreated()
{
}

void CDependencyObject::PostOnManagedPeerCreated()
{
}

#endif

void CDependencyObject::InvalidateInheritedProperty(_In_ const CDependencyProperty* pDP)
{
}

_Check_return_ HRESULT CDependencyObject::EnsureInheritedProperties(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pProperty,
    _In_     bool forGetValue
    )
{
    return E_NOTIMPL;
}

DependencyObjectDCompRegistry* CDependencyObject::GetDCompObjectRegistry() const
{
    return nullptr;
}

void __thiscall CDependencyObject::UnpegManagedPeer(bool) { }

_Check_return_ HRESULT __thiscall CDependencyObject::PegManagedPeer(bool, _Out_opt_ bool*) { return S_OK; }

_Check_return_ HRESULT CDependencyObject::SetAndOriginateError(
    _In_ HRESULT hrToOriginate,
    _In_::ErrorType eType,
    _In_ XUINT32 iErrorCode,
    _In_ XUINT32 cParams,
    _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams)
{
    return E_NOTIMPL;
}

CTimeManager* CDependencyObject::GetTimeManager()
{
    return nullptr;
}

void CDependencyObject::SetThemeResource(_In_ const CDependencyProperty *pdp, _In_ CThemeResource* pThemeResource)
{}

void CDependencyObject::UnpegManagedPeerNoRef()
{
    ASSERT(FALSE);
}

_Check_return_
HRESULT
CDependencyObject::AddPeerReferenceToItem(_In_ CDependencyObject *)
{
    return E_NOTIMPL;
}

_Check_return_
HRESULT
CDependencyObject::RemovePeerReferenceToItem(_In_ CDependencyObject *)
{
    return E_NOTIMPL;
}

void CDependencyObject::PegManagedPeerNoRef()
{
    ASSERT(false);
}

void CDependencyObject::TryPegPeer(
    _Out_ bool *pPegged,
    _Out_ bool *pIsPendingDelete)
{
    *pPegged = FALSE;
    *pIsPendingDelete = FALSE;
}

_Check_return_ HRESULT CDependencyObject::CheckThread()
{
    return S_OK;
}

_Check_return_ HRESULT
CDependencyObject::ValidateStrictnessOnProperty(_In_ const CPropertyBase* prop)
{
    return S_OK;
}

bool CDependencyObject::StoreWarningContext(WarningContextLog::WarningContextType type, _In_ std::vector<std::wstring>& warningInfo, size_t framesToSkip)
{
    return false;
}
