// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualTree.h"
#include <corep.h>
#include "Theme.h"
#include "RootVisual.h"
#include "RefreshRateInfo.h"
#include "RefreshAlignedClock.h"

CDeferredElement* CCoreServices::GetDeferredElementIfExists(
    _In_ const xstring_ptr_view& strName,
    _In_ const CDependencyObject *pNamescopeOwner,
    _In_ Jupiter::NameScoping::NameScopeType nameScopeType)
{
    return nullptr;
}

void CCoreServices::SetRequestedThemeForSubTree(_In_ Theming::Theme requestedTheme)
{
}
void CCoreServices::IncrementPendingImplicitShowHideCount(void)
{
}

void CCoreServices::DecrementPendingImplicitShowHideCount(void)
{
}

void CCoreServices::IncrementActiveFacadeAnimationCount(void)
{
}

void CCoreServices::DecrementActiveBrushTransitionCount(void)
{
}

void CCoreServices::IncrementActiveBrushTransitionCount(void)
{
}

void CCoreServices::DecrementActiveFacadeAnimationCount(void)
{
}

void CCoreServices::DecrementPendingAnimatedFacadePropertyChangeCount(void)
{
}

CResourceDictionary* CCoreServices::GetApplicationResourceDictionary()
{
    return nullptr;
}

_Check_return_ bool
CCoreServices::HasRegisteredNames(_In_ const CDependencyObject *) const
{
    return false;
}

_Ret_maybenull_ CWindowRenderTarget* CCoreServices::NWGetWindowRenderTarget()
{
    return nullptr;
}

CResourceDictionary* CCoreServices::GetThemeResources(void)
{
    return nullptr;
}

_Check_return_ HRESULT
CCoreServices::CheckUri(_In_ const xstring_ptr&, _In_ unsigned int)
{
    ASSERT(false);
    return E_NOTIMPL;
}

_Check_return_ HRESULT
CCoreServices::UnsecureDownloadFromSite(_In_ const xstring_ptr&, _In_opt_ struct IPALUri *, _In_ struct IPALDownloadResponseCallback *, _In_ unsigned int, _Outptr_opt_ struct IPALAbortableOperation * *, _In_opt_ struct IPALUri *)
{
    ASSERT(false);
    return E_NOTIMPL;
}

_Check_return_ HRESULT
CCoreServices::ExecuteOnUIThread(_In_ struct IPALExecuteOnUIThread *, const ReentrancyBehavior)
{
    ASSERT(false);
    return E_NOTIMPL;
}

_Check_return_ HRESULT CCoreServices::SetLayoutCleanSignaledStatus(_In_ bool)
{
    ASSERT(false);
    return E_NOTIMPL;
}

_Check_return_ HRESULT
CCoreServices::SetImageDecodingIdleEventSignaledStatus(_In_ bool)
{
    ASSERT(false);
    return E_NOTIMPL;
}

_Check_return_ HRESULT
CCoreServices::UnregisterRedirectionElement(_In_ CUIElement *pRedirectionElement)
{
    return S_OK;
}

HINSTANCE
CCoreServices::GetInstanceHandle()
{
    ASSERT(false);
    return nullptr;
}

DCompTreeHost* CCoreServices::GetDCompTreeHost()
{
    return nullptr;
}

_Check_return_ CPopupRoot* VisualTree::GetPopupRoot()
{
    ASSERT(false);
    return nullptr;
}

CRootVisual* VisualTree::GetRootForElement(_In_ const CDependencyObject *pObject)
{
    return (CRootVisual*)pObject;
}

_Check_return_ HRESULT VisualTree::GetPopupRootForElementNoRef(_In_ CDependencyObject *pObject, _Outptr_result_maybenull_ CPopupRoot **ppPopupRoot)
{
    ASSERT(false);
    return E_NOTIMPL;
}

VisualTree* VisualTree::GetForElementNoRef(_In_opt_ CDependencyObject*, LookupOptions)
{
    return nullptr;
}

CPopupRoot* CRootVisual::GetAssociatedPopupRootNoRef() const
{
    return nullptr;
}

_Check_return_
CDependencyObject* CCoreServices::getVisualRoot(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_
CDependencyObject* CCoreServices::getRootScrollViewer(void)
{
    ASSERT(FALSE);
    return NULL;
}

_Check_return_ HRESULT
CCoreServices::UpdateImplicitStylesOnRoots(_In_opt_ CStyle *, _In_opt_ CStyle *, bool)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

void CCoreServices::AddValueWithExpectedReference(_In_ CModifiedValue *)
{
}

void CCoreServices::RemoveValueWithExpectedReference(_In_ CModifiedValue *)
{
}

CDependencyObject* CCoreServices::GetRootForElement(_In_ CDependencyObject* dependencyObject)
{
    return nullptr;
}

void CCoreServices::SetTransparentBackground(bool isTransparent)
{
}

_Ret_maybenull_ CD3D11Device* CCoreServices::GetGraphicsDevice()
{
    return nullptr;
}

ABI::Windows::Foundation::Size CCoreServices::GetContentRootMaxSize()
{
    return ABI::Windows::Foundation::Size{ 0, 0 };
}

void CCoreServices::RequestReplayPreviousPointerUpdate()
{
}

void CCoreServices::HandleDeviceLost(_Inout_ HRESULT*)
{
}

Diagnostics::ResourceLookupLogger* CCoreServices::GetResourceLookupLogger()
{
    return nullptr;
}

/* static */ std::shared_ptr<ActivationFactoryCache> ActivationFactoryCache::GetActivationFactoryCache()
{
    return nullptr;
}

ixp::ICompositionEasingFunctionStatics* ActivationFactoryCache::GetCompositionEasingFunctionStatics()
{
    return nullptr;
}

void RefreshRateInfo::SetRefreshIntervalInMilliseconds(float refreshIntervalInMilliseconds)
{
}

XDOUBLE RefreshAlignedClock::GetLastTickTimeInSeconds() const
{
    return 0;
}
