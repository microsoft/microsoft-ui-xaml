// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "framework.h"
#include "focusmgr.h"

#include "FxCallbacks.h"

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_SetExpectedReferenceOnPeer(_In_ CDependencyObject*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_ClearExpectedReferenceOnPeer(_In_ CDependencyObject*)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_CheckPeerType(_In_ CDependencyObject*, _In_ const xstring_ptr&, _In_ XINT32)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::ExternalObjectReference_GetTarget(_In_ CDependencyObject*, _Outptr_opt_result_maybenull_ IInspectable**)
{
    return E_NOTIMPL;
}

bool FxCallbacks::UIElement_ShouldPlayImplicitShowHideAnimation(_In_ CUIElement* nativeTarget)
{
    return false;
}

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(_Out_ bool* pIsAnimationEnabled)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_LoadThemeResources()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::XamlCompositionBrushBase_OnConnected(_In_ CDependencyObject* object)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::XamlCompositionBrushBase_OnDisconnected(_In_ CDependencyObject* object)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::FrameworkCallbacks_GetCustomTypeFullName(
        _In_ CDependencyObject* ,
        _Out_ xstring_ptr* )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::DependencyObject_OnCollectionChanged(_In_ CDependencyObject*, _In_ XUINT32, _In_ XUINT32)
{
    return S_OK;
}

/* static */ _Check_return_ HRESULT FxCallbacks::DependencyObject_RefreshExpressionsOnThemeChange(_In_ CDependencyObject* nativeDO, _In_ Theming::Theme theme, _In_ bool forceRefresh)
{
    return S_OK;
}

_Check_return_ HRESULT FxCallbacks::DxamlCore_OnCompositionContentStateChangedForUWP()
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT FxCallbacks::XamlCompositionBrushBase_OnElementConnected(_In_ CDependencyObject* object, _In_ CDependencyObject* connectedElement)
{
    return E_NOTIMPL;
}

bool FxCallbacks::XamlCompositionBrushBase_HasPrivateOverrides(_In_ CDependencyObject* object)
{
    return false;
}
