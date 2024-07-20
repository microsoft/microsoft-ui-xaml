// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DesktopUtility.h"
#include "ApplicationBarService.g.h"
#include "XamlRoot.g.h"
#include "XamlRootChangedEventArgs.g.h"
#include "Window_partial.h"
#include "WindowChrome_Partial.h"
#include "ContentDialogMetadata.h"
#include "RootScrollViewer.g.h"

using namespace DirectUI;

/*static*/ IInspectable* DirectUI::XamlRoot::Create(_In_ VisualTree* visualTree)
{
    ctl::ComPtr<XamlRoot> newXamlRoot;
    IFCFAILFAST(ctl::make(&newXamlRoot));
    newXamlRoot->m_visualTree = visualTree;

    IInspectable* iinsp = nullptr;
    IFCFAILFAST(ctl::do_query_interface(iinsp, newXamlRoot.Get()));
    return iinsp;
}

XamlRoot::~XamlRoot()
{
    if (m_contentDialogMetadata)
    {
        m_contentDialogMetadata->UpdatePeg(false);
        m_contentDialogMetadata.Reset();
    }

    if (m_applicationBarService)
    {
        ctl::ComPtr<IApplicationBarService> spApplicationBarService;
        VERIFYHR(ctl::do_query_interface(spApplicationBarService, m_applicationBarService.Get()));
        VERIFYHR(spApplicationBarService->ClearCaches());

        m_applicationBarService->UpdatePeg(false);
        m_applicationBarService.Reset();
    }

    if (m_layoutBoundsChangedHelper)
    {
        m_layoutBoundsChangedHelper->DisconnectWindowEventHandlers();
        m_layoutBoundsChangedHelper->UpdatePeg(false);
        m_layoutBoundsChangedHelper.Reset();
    }
}

HRESULT XamlRoot::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IXamlRootNative)))
    {
        *ppObject = static_cast<IXamlRootNative*>(this);
    }
    else
    {
        return DirectUI::XamlRootGenerated::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_ContentImpl(_Outptr_ xaml::IUIElement** content)
{
    *content = nullptr;

    if (CUIElement* publicRootVisual = m_visualTree->GetPublicRootVisual())
    {
        ctl::ComPtr<DependencyObject> peer;
        ctl::ComPtr<xaml::IUIElement> peerAsElement;

        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(publicRootVisual, &peer));
        auto windowChrome = peer.AsOrNull<WindowChrome>();
        // if window chrome the is top most element on a visual tree, return its content as
        // the real root element of the visual tree
        // TODO: Task 38535248
        if (windowChrome)
        {
            IFC_RETURN(windowChrome->get_Content(&peerAsElement));
        }
        else
        {
            IFC_RETURN(ctl::do_query_interface(peerAsElement, peer.Get()));
        }

        *content = peerAsElement.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_SizeImpl(_Out_ wf::Size* pValue)
{
    *pValue = m_visualTree->GetSize();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_RasterizationScaleImpl(_Out_ double* pValue)
{
    *pValue = m_visualTree->GetRasterizationScale();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_IsHostVisibleImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_visualTree->GetIsVisible();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_IsInputActiveImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = m_visualTree->GetContentRootNoRef()->GetIsInputActive();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_ContentIslandEnvironmentImpl(
    _Outptr_ ixp::IContentIslandEnvironment** value)
{
    *value = nullptr;
    if (CContentRoot* contentRoot = m_visualTree->GetContentRootNoRef())
    {
        *value = contentRoot->GetContentIslandEnvironment().Detach();
    }
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::GetChangedEventSourceNoRef(_Outptr_ ChangedEventSourceType** ppEventSource)
{
    if (!m_changedEventSource)
    {
        IFC_RETURN(ctl::make(&m_changedEventSource));
        m_changedEventSource->Initialize(KnownEventIndex::XamlRoot_Changed, this, false /*bUseEventManager*/);
    }
    *ppEventSource = m_changedEventSource.Get();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::GetInputActivationChangedEventSourceNoRef(_Outptr_ InputActivationChangedEventSourceType** ppEventSource)
{
    if (!m_inputActivationChangedEventSource)
    {
        IFC_RETURN(ctl::make(&m_inputActivationChangedEventSource));
        m_inputActivationChangedEventSource->Initialize(KnownEventIndex::XamlRoot_InputActivationChanged, this, true /*bUseEventManager*/);
    }
    *ppEventSource = m_inputActivationChangedEventSource.Get();
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::RaiseChangedEvent()
{
    if (m_changedEventSource)
    {
        ctl::ComPtr<XamlRoot> spThis(this);
        ctl::ComPtr<xaml::IXamlRoot> spThat;
        IFC_RETURN(spThis.As(&spThat));

        ctl::ComPtr<XamlRootChangedEventArgs> args;
        IFC_RETURN(ctl::make(&args));

        IFC_RETURN(m_changedEventSource->Raise(spThat.Get(), args.Get()));
    }
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::RaiseInputActivationChangedEvent()
{
    if (m_inputActivationChangedEventSource)
    {
        ctl::ComPtr<XamlRoot> spThis(this);
        ctl::ComPtr<xaml::IXamlRoot> spThat;
        IFC_RETURN(spThis.As(&spThat));

        ctl::ComPtr<XamlRootChangedEventArgs> args;
        IFC_RETURN(ctl::make(&args));

        IFC_RETURN(m_inputActivationChangedEventSource->Raise(spThat.Get(), args.Get()));
    }
    return S_OK;
}

ctl::ComPtr<xaml::IXamlRoot> XamlRoot::GetForElementStatic(_In_ DirectUI::DependencyObject* element, bool createIfNotExist)
{
    ctl::ComPtr<xaml::IXamlRoot> result;

    IInspectable* xamlRootInsp = nullptr;
    VisualTree* visualTree = VisualTree::GetForElementNoRef(element->GetHandle());
    if (visualTree)
    {
        xamlRootInsp = createIfNotExist ? visualTree->GetOrCreateXamlRootNoRef() : visualTree->GetXamlRootNoRef();
    }

    if (xamlRootInsp)
    {
        IFCFAILFAST(xamlRootInsp->QueryInterface(IID_PPV_ARGS(&result)));
    }
    return result;
}

ctl::ComPtr<XamlRoot> XamlRoot::GetImplementationForElementStatic(_In_ DirectUI::DependencyObject* element, bool createIfNotExist)
{
    ctl::ComPtr<XamlRoot> result;
    ctl::ComPtr<xaml::IXamlRoot> ixamlRoot = GetForElementStatic(element, createIfNotExist);
    IFCFAILFAST(ixamlRoot.As(&result));
    return result;
}

_Check_return_ HRESULT XamlRoot::SetForElementStatic(
    _In_ DirectUI::DependencyObject* pElement,
    _In_ xaml::IXamlRoot* pXamlRoot)
{
    ctl::ComPtr<XamlRoot> xamlRoot;
    IFCFAILFAST(pXamlRoot->QueryInterface(IID_PPV_ARGS(&xamlRoot)));
    IFC_RETURN(xamlRoot->m_visualTree->AttachElement(pElement->GetHandle()));
    return S_OK;
}

_Check_return_ HRESULT XamlRoot::get_HostWindow(_Out_ HWND* pValue)
{
    *pValue = nullptr;

    // Always return nullptr on Onecore-based systems
    if (DesktopUtility::IsOnDesktop())
    {
        auto cr = m_visualTree->GetContentRootNoRef();
        HWND hwnd = cr->GetHostingHWND();

        if (!hwnd)
        {
            // The caller is hosted in a XamlIsland. Ideally, we would return null here, as
            // we can't guarantee that the XamlIsland is hosted in an HWND bridge and can't access that
            // HWND if it is. Any calls to method should instead include an equivalent call to the
            // ContentIsland if we return null.

            // Bug https://task.ms/48685229: For now, there are still calls to this method that cannot be
            // easily replaced with a ContentIsland API, specifically in WebView2.cpp. This means that we
            // have to return a hosting HWND even in the XamlIsland scenario. The InputSite gives us a
            // way to retrieve this HWND as long as InputSite velocity key 45720437 is disabled. Once
            // this key is enabled for this repo, this will no longer be the case, as the InputSite will
            // return an HWND that works for input but not for hosting or positioning. At this point,
            // we will return null and callers will need to make an equivalent call to the ContentIsland.
            wrl::ComPtr<ixp::IIslandInputSitePartner> inputSite = 
            GetVisualTreeNoRef()->GetContentRootNoRef()->GetXamlIslandRootNoRef()->GetIslandInputSite();

            if (inputSite)
            {
                hwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(inputSite.Get());
            }
        }

        *pValue = hwnd;
    }

    return S_OK;
}

_Check_return_ HRESULT XamlRoot::GetContentDialogMetadata(
    _Outptr_ ContentDialogMetadata** ppContentDialogMetadata)
{
    if (!m_contentDialogMetadata)
    {
        IFC_RETURN(ctl::make(&m_contentDialogMetadata));
        m_contentDialogMetadata->UpdatePeg(true);
    }

    IFC_RETURN(m_contentDialogMetadata.CopyTo(ppContentDialogMetadata));

    return S_OK;
}

_Check_return_ HRESULT XamlRoot::GetApplicationBarService(_Out_ ctl::ComPtr<IApplicationBarService>& service)
{
    if (!m_applicationBarService)
    {
        IFC_RETURN(ctl::ComObject<ApplicationBarService>::CreateInstance(m_applicationBarService.ReleaseAndGetAddressOf()));
        m_applicationBarService->UpdatePeg(true);

        IFC_RETURN(ctl::do_query_interface(service, m_applicationBarService.Get()));
        service->SetXamlRoot(this);
    }
    else
    {
        IFC_RETURN(ctl::do_query_interface(service, m_applicationBarService.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlRoot::TryGetApplicationBarService(_Out_ ctl::ComPtr<IApplicationBarService>& service)
{
    return m_applicationBarService.CopyTo(service.ReleaseAndGetAddressOf());
}

LayoutBoundsChangedHelper* XamlRoot::GetLayoutBoundsHelperNoRef()
{
    if (!m_layoutBoundsChangedHelper)
    {
        IFCFAILFAST(ctl::make(&m_layoutBoundsChangedHelper));

        m_layoutBoundsChangedHelper->UpdatePeg(true);
        m_layoutBoundsChangedHelper->InitializeSizeChangedHandlers(static_cast<xaml::IXamlRoot*>(this));
    }

    return m_layoutBoundsChangedHelper.Get();
}

wf::Rect XamlRoot::GetSimulatedInputPaneOccludedRect() const
{
    return m_simulatedInputPaneOccludedRect;
}

void XamlRoot::SetSimulatedInputPaneOccludedRect(const wf::Rect& rect)
{
    m_simulatedInputPaneOccludedRect = rect;

    // notify the root scroll viewer of the change
    if (m_simulatedInputPaneOccludedRect.Width != 0 && m_simulatedInputPaneOccludedRect.Height != 0)
    {
        DXamlCore::GetCurrent()->ClientToScreen(&m_simulatedInputPaneOccludedRect);
    }

    CScrollContentControl *scrollContentControl = m_visualTree->GetRootScrollViewer();
    if (!scrollContentControl)
    {
        // nothing further to do
        return;
    }

    ctl::ComPtr<DirectUI::DependencyObject> rootScrollViewerAsDO;
    ctl::ComPtr<xaml_controls::IScrollViewer> rootScrollViewer;
    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(scrollContentControl, &rootScrollViewerAsDO));
    IFCFAILFAST(rootScrollViewerAsDO.As(&rootScrollViewer));

    if (rootScrollViewer)
    {
        IFCFAILFAST(rootScrollViewer.Cast<RootScrollViewer>()->NotifyInputPaneStateChange(
            m_simulatedInputPaneOccludedRect.Height > 0 ? InputPaneState::InputPaneShowing : InputPaneState::InputPaneHidden,
            {m_simulatedInputPaneOccludedRect.X, m_simulatedInputPaneOccludedRect.Y, m_simulatedInputPaneOccludedRect.Width, m_simulatedInputPaneOccludedRect.Height}
        ));
    }
}