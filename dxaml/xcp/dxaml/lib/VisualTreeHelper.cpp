// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualTreeHelper.h"
#include "Window_Partial.h"
#include "TextCommon.h"
#include "XamlRoot_Partial.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

VisualTreeHelper::VisualTreeHelper()
{
}

HRESULT VisualTreeHelper::QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml_media::IVisualTreeHelperStatics)))
    {
        *ppObject = static_cast<xaml_media::IVisualTreeHelperStatics *>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml_media::IVisualTreeHelper)))
    {
        *ppObject = static_cast<xaml_media::IVisualTreeHelper *>(this);
    }
    else
    {
        return ctl::AbstractActivationFactory::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP VisualTreeHelper::FindElementsInHostCoordinatesPoint(
    _In_ wf::Point intersectingPoint,
    _In_opt_ xaml::IUIElement* pSubTree,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppElements);
    IFC(CheckActivationAllowed());

    IFC(FindElementsInHostCoordinatesPointStatic(intersectingPoint, pSubTree, c_canHitDisabledElementsDefault, c_canHitInvisibleElementsDefault, ppElements));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::FindElementsInHostCoordinatesRect(
    _In_ wf::Rect intersectingRect,
    _In_opt_ xaml::IUIElement* pSubTree,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppElements);
    IFC(CheckActivationAllowed());

    IFC(FindElementsInHostCoordinatesRectStatic(intersectingRect, pSubTree, c_canHitDisabledElementsDefault, c_canHitInvisibleElementsDefault, ppElements));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::FindAllElementsInHostCoordinatesPoint(
    _In_ wf::Point intersectingPoint,
    _In_opt_ xaml::IUIElement* pSubTree,
    _In_ BOOLEAN includeAllElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppElements);
    IFC(CheckActivationAllowed());

    IFC(FindAllElementsInHostCoordinatesPointStatic(intersectingPoint, pSubTree, includeAllElements, ppElements));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::FindAllElementsInHostCoordinatesRect(
    _In_ wf::Rect intersectingRect,
    _In_opt_ xaml::IUIElement* pSubTree,
    _In_ BOOLEAN includeAllElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppElements);
    IFC(CheckActivationAllowed());

    IFC(FindAllElementsInHostCoordinatesRectStatic(intersectingRect, pSubTree, includeAllElements, ppElements));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::GetChild(
    _In_ xaml::IDependencyObject* reference,
    _In_ INT nChildIndex,
    _Out_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppDO);
    ARG_NOTNULL(reference, "reference");
    IFC(CheckActivationAllowed());
    IFC(DefaultStrictApiCheck(static_cast<DependencyObject*>(reference)));

    IFC(GetChildStatic(reference, nChildIndex, ppDO));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::GetChildrenCount(
    _In_ xaml::IDependencyObject* reference,
    _Out_ INT* pnCount)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(pnCount);
    ARG_NOTNULL(reference, "reference");
    IFC(CheckActivationAllowed());
    IFC(DefaultStrictApiCheck(static_cast<DependencyObject*>(reference)));

    IFC(GetChildrenCountStatic(reference, pnCount));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::GetParent(
    _In_ xaml::IDependencyObject* reference,
    _Out_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ppDO);
    ARG_NOTNULL(reference, "reference");
    IFC(CheckActivationAllowed());

    IFC(GetParentStatic(reference, ppDO));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::DisconnectChildrenRecursive(
    _In_ xaml::IUIElement* element)
{
    HRESULT hr = S_OK;

    ARG_NOTNULL(element, "element");
    IFC(CheckActivationAllowed());

    IFC(DisconnectChildrenRecursiveStatic(element));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP VisualTreeHelper::GetOpenPopups(
    _In_ xaml::IWindow* window,
    _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups)
{
    HRESULT hr = S_OK;
    DXamlCore* dxamlCore = nullptr;
    ctl::ComPtr<Window> spWindow;

    IFC(ctl::do_query_interface(spWindow, window));
    dxamlCore = spWindow->GetDXamlCore();
    IFCPTR(dxamlCore);

    ARG_VALIDRETURNPOINTER(ppPopups);
    ARG_NOTNULL(window, "window");
    IFC(CheckActivationAllowed());

    IFC(GetOpenPopupsStatic(dxamlCore->GetHandle()->GetMainVisualTree(), ppPopups));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetOpenPopupsForXamlRoot(
    _In_ xaml::IXamlRoot* pXamlRoot,
    _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::XamlRoot> xamlRoot;

    ARG_VALIDRETURNPOINTER(ppPopups);
    ARG_NOTNULL(pXamlRoot, "xamlRoot");
    IFC(CheckActivationAllowed());

    IFC(pXamlRoot->QueryInterface(IID_PPV_ARGS(&xamlRoot)));
    IFC(GetOpenPopupsStatic(xamlRoot->GetVisualTreeNoRef(), ppPopups));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::FindElementsInHostCoordinatesPointStatic(
    _In_ wf::Point intersectingPoint,
    _In_opt_ IUIElement* pSubTree,
    _In_ BOOLEAN canHitDisabledElements,
    _In_ BOOLEAN canHitInvisibleElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spSubTreeAsDO;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CDependencyObject* pCoreDONoRef = NULL;
    XPOINTF point = {0};
    XINT32 nCount = 0;
    CUIElement** pElements = NULL;

    point.x = intersectingPoint.X;
    point.y = intersectingPoint.Y;

    if (pSubTree != NULL)
    {
        IFC(ctl::do_query_interface(spSubTreeAsDO, pSubTree));
        pCoreDONoRef = spSubTreeAsDO.Cast<DependencyObject>()->GetHandle();
    }

    IFC(CoreImports::UIElement_HitTestPoint(
        static_cast<CUIElement*>(pCoreDONoRef),
        static_cast<CCoreServices*>(pCore->GetHandle()),
        point,
        !!canHitDisabledElements,
        !!canHitInvisibleElements,
        &nCount,
        &pElements));

    IFC(MakeUIElementList(pElements, nCount, ppElements));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::FindElementsInHostCoordinatesRectStatic(
    _In_ wf::Rect intersectingRect,
    _In_opt_ IUIElement* pSubTree,
    _In_ BOOLEAN canHitDisabledElements,
    _In_ BOOLEAN canHitInvisibleElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spSubTreeAsDO;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    CDependencyObject* pCoreDONoRef = NULL;
    XRECTF rect = {0};
    XINT32 nCount = 0;
    CUIElement** pElements = NULL;
    bool isRootNull_dontCare;

    rect.X = intersectingRect.X;
    rect.Y = intersectingRect.Y;
    rect.Width = intersectingRect.Width;
    rect.Height = intersectingRect.Height;

    if (pSubTree != NULL)
    {
        IFC(ctl::do_query_interface(spSubTreeAsDO, pSubTree));
        pCoreDONoRef = spSubTreeAsDO.Cast<DependencyObject>()->GetHandle();
    }

    IFC(CoreImports::UIElement_HitTestRect(
        static_cast<CUIElement*>(pCoreDONoRef),
        static_cast<CCoreServices*>(pCore->GetHandle()),
        rect,
        !!canHitDisabledElements,
        !!canHitInvisibleElements,
        &nCount,
        &pElements,
        &isRootNull_dontCare));

    IFC(MakeUIElementList(pElements, nCount, ppElements));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::FindAllElementsInHostCoordinatesPointStatic(
    _In_ wf::Point intersectingPoint,
    _In_opt_ IUIElement* pSubTree,
    _In_ BOOLEAN includeAllElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    BOOLEAN bHistTestModeSet = FALSE;

    // enable invisible hit testing
    if (includeAllElements)
    {
        CoreImports::HostProperties_SetInvisibleHitTestMode(pCore->GetHandle(), true);
        bHistTestModeSet = TRUE;
    }

    // now call the regular hit test function.
    IFC(VisualTreeHelper::FindElementsInHostCoordinatesPointStatic(intersectingPoint, pSubTree, c_canHitDisabledElementsDefault, c_canHitInvisibleElementsDefault, ppElements));

Cleanup:
    if (bHistTestModeSet)
    {
        CoreImports::HostProperties_SetInvisibleHitTestMode(pCore->GetHandle(), false);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::FindAllElementsInHostCoordinatesRectStatic(
    _In_ wf::Rect intersectingRect,
    _In_opt_ IUIElement* pSubTree,
    _In_ BOOLEAN includeAllElements,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    BOOLEAN bHistTestModeSet = FALSE;

    // enable invisible hit testing
    if (includeAllElements)
    {
        CoreImports::HostProperties_SetInvisibleHitTestMode(pCore->GetHandle(), true);
        bHistTestModeSet = TRUE;
    }

    // now call the regular hit test function.
    IFC(VisualTreeHelper::FindElementsInHostCoordinatesRectStatic(intersectingRect, pSubTree, c_canHitDisabledElementsDefault, c_canHitInvisibleElementsDefault, ppElements));

Cleanup:
    if (bHistTestModeSet)
    {
        CoreImports::HostProperties_SetInvisibleHitTestMode(pCore->GetHandle(), false);
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetChildStatic(
    _In_ xaml::IDependencyObject* pReference,
    _In_ INT nChildIndex,
    _Out_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spResultAsDO;

    IFC(GetRelative(pReference, RelativeKindChild, TRUE /*createPeer*/, &spResultAsDO));
    if (spResultAsDO)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spResultAsPFC;
        ctl::ComPtr<xaml::IUIElement> spChild;

        IFC(spResultAsDO.As(&spResultAsPFC));

        IFC(spResultAsPFC->GetAt(nChildIndex, &spChild));

        IFCCATASTROPHIC(spChild);

        IFC(spChild.CopyTo(ppDO));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetChildrenCountStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ INT* pnCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spResultAsDO;
    UINT nCount = 0;

    IFC(GetRelative(pReference, RelativeKindChild, TRUE /*createPeer*/, &spResultAsDO));
    if (spResultAsDO)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spResultAsPFC;

        IFC(spResultAsDO.As(&spResultAsPFC));

        IFC(spResultAsPFC->get_Size(&nCount));
        *pnCount = nCount;
    }
    else
    {
        *pnCount = 0;
    }

Cleanup:
    RRETURN(hr);
}

// pHasParent is set to True when pReference has a parent, whether it is live in the tree or not.
// pIsParentAccessible is only set to True when pHasParent is also set to True and pReference is live.
// VisualTreeHelper::GetParentStatic returns a parent when and only when pIsParentAccessible is True.
_Check_return_ HRESULT
VisualTreeHelper::HasParentStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ BOOLEAN* pHasParent,
    _Out_opt_ BOOLEAN* pIsParentAccessible)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spDO;

    *pHasParent = FALSE;

    IFC(GetParentStaticPrivate(pReference, FALSE /*isForAccessibleParentOnly*/, &spDO, pHasParent, pIsParentAccessible));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetParentStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ xaml::IDependencyObject** ppDO)
{
    RRETURN(GetParentStaticPrivate(pReference, TRUE /*isForAccessibleParentOnly*/, ppDO));
}

_Check_return_ HRESULT
VisualTreeHelper::TryGetParentStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ xaml::IDependencyObject** ppDO)
{
    RRETURN(GetParentStaticPrivate(pReference, FALSE /*isForAccessibleParentOnly*/, ppDO));
}

_Check_return_ HRESULT
VisualTreeHelper::GetParentStaticPrivate(
    _In_ xaml::IDependencyObject* pReference,
    _In_ BOOLEAN isForAccessibleParentOnly,
    _Out_ xaml::IDependencyObject** ppDO,
    _Out_opt_ BOOLEAN* pHasNativeParent,
    _Out_opt_ BOOLEAN* pIsParentAccessible)
{
    CValue result;
    CDependencyObject* pDO = nullptr;
    ctl::ComPtr<DependencyObject> spLinkHost;

    ASSERT(pReference);
    ASSERT(ppDO);

    if (pIsParentAccessible)
    {
        *pIsParentAccessible = FALSE;
    }

    const bool bLive = static_cast<DependencyObject*>(pReference)->IsInLiveTree();

    // Don't return the parent unless we're in the live tree.
    if (isForAccessibleParentOnly && !bLive)
    {
        *ppDO = nullptr;
        return S_OK;
    }

    // Since a link can take focus, it may be passed in to get the visual parent.
    // Handle this case.
    if (Link::IsLink(static_cast<DependencyObject*>(pReference)->GetHandle()))
    {
        IFC_RETURN(CoreImports::GetLinkHostFrameworkElement(
            static_cast<CInline*>(static_cast<DependencyObject*>(pReference)->GetHandle()),
            &result));
        pDO = result.AsObject();
        if (pDO != nullptr)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pDO, &spLinkHost));
            IFC_RETURN(spLinkHost.CopyTo(ppDO));
        }
    }
    else
    {
        IFC_RETURN(GetRelative(pReference, RelativeKindParent, bLive, ppDO, pHasNativeParent));
        // XamlIsland is private type, hence conceal it by returning nullptr as a parent.
        if (*ppDO && ctl::is<xaml_hosting::IXamlIsland>(*ppDO))
        {
            ReleaseInterface(*ppDO);
            return S_OK;
        }
    }

    if (pIsParentAccessible && bLive && *ppDO)
    {
        *pIsParentAccessible = TRUE;
    }

    return S_OK;
}

_Check_return_
HRESULT
VisualTreeHelper::GetRootStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;

    IFC(GetVisualRelative(pReference, RelativeKindRoot, TRUE /*createPeer*/, ppDO));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetRelative(
    _In_ xaml::IDependencyObject* pReference,
    _In_ RelativeKind relativeKind,
    _In_ BOOLEAN createPeer,
    _Out_ xaml::IDependencyObject** ppDO,
    _Out_opt_ BOOLEAN* pHasNativeRelative)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IDependencyObject> spRelative;

    IFCEXPECT(ctl::is<xaml::IUIElement>(pReference));

    // Get the parent or child.
    IFC(GetVisualRelative(pReference, relativeKind, createPeer, &spRelative, pHasNativeRelative));

    // For RelativeKind.Child, only include UIElements in the child collection.
    // Currently the Core returns a UIElementCollection where elements are
    // strongly typed as UIElements so no additional checks are required.

    // Ensure parent is UIElement
    if (relativeKind == RelativeKindParent)
    {
        ctl::ComPtr<xaml::IDependencyObject> spPreviousRelative;

        // Continue up the parent chain while the parent is not a UIElement.
        while (spRelative && !ctl::is<xaml::IUIElement>(spRelative.Get()))
        {
            // Get the parent or child.
            spPreviousRelative = spRelative;
            spRelative = nullptr;

            IFC(GetVisualRelative(spPreviousRelative.Get(), relativeKind, createPeer, &spRelative, pHasNativeRelative));
        }
    }

    IFC(spRelative.CopyTo(ppDO));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::GetVisualRelative(
    _In_ xaml::IDependencyObject* pReference,
    _In_ RelativeKind relativeKind,
    _In_ BOOLEAN createPeer,
    _Out_ xaml::IDependencyObject** ppDO,
    _Out_opt_ BOOLEAN* pHasNativeRelative)
{
    HRESULT hr = S_OK;
    CValue result;
    CDependencyObject* pDO = NULL;
    ctl::ComPtr<DependencyObject> spRelative;

    if (pHasNativeRelative)
    {
        *pHasNativeRelative = FALSE;
    }

    // Get the parent or child.
    IFC(CoreImports::DependencyObject_GetVisualRelative(
        static_cast<CUIElement*>(static_cast<DependencyObject*>(pReference)->GetHandle()),
        (XINT32)relativeKind,
        &result));

    pDO = result.AsObject();
    if (pDO != NULL)
    {
        if (pHasNativeRelative)
        {
            *pHasNativeRelative = TRUE;
        }

        KnownTypeIndex hClass = pDO->GetTypeIndex();
        if (createPeer)
        {
            IFC(DXamlCore::GetCurrent()->GetPeer(pDO, hClass, &spRelative));
        }
        else
        {
            IFC(DXamlCore::GetCurrent()->TryGetPeer(pDO, hClass, &spRelative));
        }

        IFC(spRelative.CopyTo(ppDO));
    }
    else
    {
        *ppDO = NULL;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::MakeUIElementList(
    _In_reads_(nCount) CUIElement** pElements,
    _In_ XUINT32 nCount,
    _Out_ wfc::IIterable<xaml::UIElement*>** ppElements)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    ctl::ComPtr<TrackerCollection<xaml::UIElement*>> spUIEC;

    IFC(ctl::make(&spUIEC));
    for (XUINT32 i = 0; i < nCount; i++)
    {
        ctl::ComPtr<DependencyObject> spDO;
        if (SUCCEEDED(pCore->GetPeer(pElements[i], &spDO)))
        {
            ctl::ComPtr<IUIElement> spDOAsUIE;
            IFC(spDO.As(&spDOAsUIE));
            IFC(spUIEC->Append(spDOAsUIE.Get()));
        }
    }

    IFC(spUIEC.CopyTo(ppElements));

Cleanup:
    VERIFYRETURNHR(CoreImports::UIElement_DeleteList(pElements, nCount));
    RRETURN(hr);
}

_Check_return_ HRESULT
VisualTreeHelper::DisconnectChildrenRecursiveStatic(
    _In_ xaml::IUIElement* pElement )
{
    RRETURN(static_cast<UIElement*>(pElement)->DisconnectChildrenRecursive());
}

_Check_return_ HRESULT VisualTreeHelper::GetChildrenStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Out_ xaml::IDependencyObject** ppDO)
{
    return GetRelative(pReference, RelativeKindChild, TRUE /*createPeer*/, ppDO);
}

_Check_return_ HRESULT
VisualTreeHelper::GetOpenPopupsStatic(
    _In_opt_ VisualTree* visualTree,
    _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups)
{
    HRESULT hr = S_OK;

    DXamlCore* pCore = nullptr;
    CPopupRoot* pPopupRoot = nullptr;
    CPopup** ppOpenedPopups = nullptr;
    XINT32 nCountOpenedPopups = 0;
    ctl::ComPtr<TrackerCollection<xaml_primitives::Popup*>> spPopupCollection;

    *ppPopups = nullptr;

    // It's okay to return an empty collection of popups if there is no
    // element. The API shouldn't fail, just return count of 0
    IFC(ctl::make(&spPopupCollection));

    if (visualTree)
    {
        pPopupRoot = visualTree->GetPopupRoot();
        if (pPopupRoot)
        {
            IFC(pPopupRoot->GetOpenPopups(&nCountOpenedPopups, &ppOpenedPopups));

            for (XINT32 i = 0; i < nCountOpenedPopups; i++)
            {
                ctl::ComPtr<DependencyObject> spDO;

                IFC(pCore->TryGetPeer(ppOpenedPopups[i], &spDO));
                if (spDO != nullptr)
                {
                    ctl::ComPtr<xaml_primitives::IPopup> spDOAsPopup;
                    IFC(spDO.As(&spDOAsPopup));
                    IFC(spPopupCollection->Append(spDOAsPopup.Get()));
                }
            }
        }
    }

    IFC(spPopupCollection->GetView(ppPopups));

Cleanup:
    VERIFYRETURNHR(CoreImports::UIElement_DeleteList(ppOpenedPopups, nCountOpenedPopups));
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to get FullWindowMediaRoot
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
VisualTreeHelper::GetFullWindowMediaRootStatic(
    _In_ xaml::IDependencyObject* pReference,
    _Outptr_ xaml_controls::IPanel** ppFullWindowMediaRoot)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = NULL;
    CCoreServices* pCoreServices = NULL;
    CFullWindowMediaRoot* pFullWindowMediaRootNoRef = NULL;
    ctl::ComPtr<xaml::IDependencyObject> spReferenceDO;
    ctl::ComPtr<DependencyObject> spFullWindowMediaRootAsDO;

    *ppFullWindowMediaRoot = NULL;

    IFC(ctl::do_query_interface(spReferenceDO, pReference));

    pCore = DXamlCore::GetFromDependencyObject(spReferenceDO.Cast<DependencyObject>());
    IFCPTR(pCore);

    pCoreServices = pCore->GetHandle();
    IFCPTR(pCoreServices);

    pFullWindowMediaRootNoRef = pCoreServices->GetMainFullWindowMediaRoot();

    if (pFullWindowMediaRootNoRef)
    {
        // Get FullWindowMediaRoot framework peer, creating if necessary
        IFC(pCore->GetPeer(pFullWindowMediaRootNoRef, &spFullWindowMediaRootAsDO));
        IFC(spFullWindowMediaRootAsDO.CopyTo(ppFullWindowMediaRoot));
    }

Cleanup:
    RRETURN(hr);
}

namespace DirectUI
{
    _Check_return_ IActivationFactory* CreateActivationFactory_VisualTreeHelper()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::VisualTreeHelper>::CreateActivationFactory());
    }
}
