// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlRoot.g.h"
#include "microsoft.ui.xaml.xamlroot.h"
#include <VisualTree.h>

namespace DirectUI
{
    class ContentDialogMetadata;
    class ApplicationBarServiceGenerated;
    class LayoutBoundsChangedHelper;
    interface IApplicationBarService;

    PARTIAL_CLASS(XamlRoot),
        public IXamlRootNative
    {
        BEGIN_INTERFACE_MAP(XamlRoot, XamlRootGenerated)
            INTERFACE_ENTRY(XamlRoot, IXamlRootNative)
        END_INTERFACE_MAP(XamlRoot, XamlRootGenerated)
    public:

        static IInspectable* Create(_In_ VisualTree* visualTree);
        ~XamlRoot() override;

        _Check_return_ HRESULT STDMETHODCALLTYPE get_ContentImpl(_Outptr_ xaml::IUIElement** content);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_SizeImpl(_Out_ wf::Size* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_RasterizationScaleImpl(_Out_ double* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsHostVisibleImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_CompositorImpl(_Outptr_ WUComp::ICompositor** compositor);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_IsInputActiveImpl(_Out_ BOOLEAN* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE get_ContentIslandEnvironmentImpl(_Outptr_ ixp::IContentIslandEnvironment** contentIslandEnvironment);

        _Check_return_ HRESULT GetChangedEventSourceNoRef(_Outptr_ ChangedEventSourceType** ppEventSource) override;
        _Check_return_ HRESULT GetInputActivationChangedEventSourceNoRef(_Outptr_ InputActivationChangedEventSourceType** ppEventSource) override;

        _Check_return_ HRESULT RaiseChangedEvent();
        _Check_return_ HRESULT RaiseInputActivationChangedEvent();

        VisualTree* GetVisualTreeNoRef() { return m_visualTree.get(); }

        static ctl::ComPtr<xaml::IXamlRoot> GetForElementStatic(
            _In_ DirectUI::DependencyObject* element,
            bool createIfNotExist = true);

        static ctl::ComPtr<XamlRoot> GetImplementationForElementStatic(
            _In_ DirectUI::DependencyObject* element,
            bool createIfNotExist = true);

        static _Check_return_ HRESULT SetForElementStatic(
            _In_ DirectUI::DependencyObject* pElement,
            _In_ xaml::IXamlRoot* pXamlRoot);

        IFACEMETHOD(get_HostWindow)(_Out_ HWND* pValue) override;

        _Check_return_ HRESULT GetContentDialogMetadata(_Outptr_ ContentDialogMetadata** ppContentDialogMetadata);

        _Check_return_ HRESULT GetApplicationBarService(_Out_ ctl::ComPtr<IApplicationBarService>& service);
        _Check_return_ HRESULT TryGetApplicationBarService(_Out_ ctl::ComPtr<IApplicationBarService>& service);
        LayoutBoundsChangedHelper* GetLayoutBoundsHelperNoRef();

        wf::Rect GetSimulatedInputPaneOccludedRect() const;
        void SetSimulatedInputPaneOccludedRect(const wf::Rect& rect);

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

        xref_ptr<VisualTree> m_visualTree;

        ctl::ComPtr<ChangedEventSourceType> m_changedEventSource;
        ctl::ComPtr<InputActivationChangedEventSourceType> m_inputActivationChangedEventSource;
        ctl::ComPtr<ContentDialogMetadata> m_contentDialogMetadata;

        ctl::ComPtr<ApplicationBarServiceGenerated> m_applicationBarService;
        ctl::ComPtr<LayoutBoundsChangedHelper> m_layoutBoundsChangedHelper;

        wf::Rect m_simulatedInputPaneOccludedRect = {};
    };
}
