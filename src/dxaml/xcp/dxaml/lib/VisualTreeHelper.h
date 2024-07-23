// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{

    class VisualTreeHelper :
        public xaml_media::IVisualTreeHelper,
        public xaml_media::IVisualTreeHelperStatics,
        public ctl::AbstractActivationFactory
    {
    public:

        // IVisualTreeHelperStatics implementation
        IFACEMETHODIMP FindElementsInHostCoordinatesPoint(
            _In_ wf::Point intersectingPoint,
            _In_opt_ xaml::IUIElement* pSubTree,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements) override;

        IFACEMETHODIMP FindElementsInHostCoordinatesRect(
            _In_ wf::Rect intersectingRect,
            _In_opt_ xaml::IUIElement* pSubTree,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements) override;

        IFACEMETHODIMP FindAllElementsInHostCoordinatesPoint(
            _In_ wf::Point intersectingPoint,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN includeAllElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements) override;

        IFACEMETHODIMP FindAllElementsInHostCoordinatesRect(
            _In_ wf::Rect intersectingRect,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN includeAllElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements) override;

        IFACEMETHODIMP GetChild(
            _In_ xaml::IDependencyObject* pReference,
            _In_ INT nChildIndex,
            _Out_ xaml::IDependencyObject** ppDO) override;

        IFACEMETHODIMP GetChildrenCount(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ INT* pnCount) override;

        IFACEMETHODIMP GetParent(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ xaml::IDependencyObject** ppDO) override;

        IFACEMETHODIMP DisconnectChildrenRecursive(
            _In_ xaml::IUIElement* pElement) override;

        IFACEMETHODIMP GetOpenPopups(
            _In_ xaml::IWindow* pWindow,
            _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups) override;

        _Check_return_ IFACEMETHODIMP GetOpenPopupsForXamlRoot(
            _In_ xaml::IXamlRoot* xamlRoot,
            _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups) override;

    BEGIN_INTERFACE_MAP(VisualTreeHelper, ctl::AbstractActivationFactory)
        INTERFACE_ENTRY(VisualTreeHelper, xaml_media::IVisualTreeHelper)
        INTERFACE_ENTRY(VisualTreeHelper, xaml_media::IVisualTreeHelperStatics)
    END_INTERFACE_MAP(VisualTreeHelper,ctl::AbstractActivationFactory)

    protected:

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        VisualTreeHelper();


    public:

        // Internal static methods usable from within the framework.
        // NOTE: XamlDiagHitTestMode is meant for use with XamlDiagnostics ONLY and should be FALSE in all other cases. It will cause all
        // elements to be returned even if IsHitTestVisible is FALSE or the element is disabled. That is why it off by default.
        static const BOOLEAN c_canHitDisabledElementsDefault = FALSE;
        static const BOOLEAN c_canHitInvisibleElementsDefault = FALSE;

        static _Check_return_ HRESULT FindElementsInHostCoordinatesPointStatic(
            _In_ wf::Point intersectingPoint,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN canHitDisabledElements,
            _In_ BOOLEAN canHitInvisibleElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements);

        static _Check_return_ HRESULT FindElementsInHostCoordinatesRectStatic(
            _In_ wf::Rect intersectingRect,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN canHitDisabledElements,
            _In_ BOOLEAN canHitInvisibleElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements);

        static _Check_return_ HRESULT FindAllElementsInHostCoordinatesPointStatic(
            _In_ wf::Point intersectingPoint,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN includeAllElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements);

        static _Check_return_ HRESULT FindAllElementsInHostCoordinatesRectStatic(
            _In_ wf::Rect intersectingRect,
            _In_opt_ xaml::IUIElement* pSubTree,
            _In_ BOOLEAN includeAllElements,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements);

        static _Check_return_ HRESULT GetChildStatic(
            _In_ xaml::IDependencyObject* pReference,
            _In_ INT nChildIndex,
            _Out_ xaml::IDependencyObject** ppDO);

        static _Check_return_ HRESULT GetChildrenCountStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ INT* pnCount);

        static _Check_return_ HRESULT HasParentStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ BOOLEAN* pHasParent,
            _Out_opt_ BOOLEAN* pIsParentAccessible = NULL);

        static _Check_return_ HRESULT GetParentStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ xaml::IDependencyObject** ppDO);

        // ignores visual tree presence check and tries to create the peer.
        static _Check_return_ HRESULT TryGetParentStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ xaml::IDependencyObject** ppDO);

        static _Check_return_ HRESULT GetRootStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ xaml::IDependencyObject** ppDO);

        static _Check_return_ HRESULT DisconnectChildrenRecursiveStatic(
            _In_ xaml::IUIElement* pElement );

        static _Check_return_ HRESULT GetOpenPopupsStatic(
            _In_opt_ VisualTree* visualTree,
            _Outptr_ wfc::IVectorView<xaml_primitives::Popup*>** ppPopups);

        static _Check_return_ HRESULT GetFullWindowMediaRootStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Outptr_ xaml_controls::IPanel** ppFullWindowMediaRoot);

        static _Check_return_ HRESULT GetChildrenStatic(
            _In_ xaml::IDependencyObject* pReference,
            _Out_ xaml::IDependencyObject** ppDO);

    protected:

        enum RelativeKind
        {
            RelativeKindChild = 0,
            RelativeKindParent = 1,
            RelativeKindRoot = 2,
        };

        static _Check_return_ HRESULT GetRelative(
            _In_ xaml::IDependencyObject* pReference,
            _In_ RelativeKind relativeKind,
            _In_ BOOLEAN createPeer,
            _Out_ xaml::IDependencyObject** ppDO,
            _Out_opt_ BOOLEAN* pHasNativeRelative = NULL);

        static _Check_return_ HRESULT GetVisualRelative(
            _In_ xaml::IDependencyObject* pReference,
            _In_ RelativeKind relativeKind,
            _In_ BOOLEAN createPeer,
            _Out_ xaml::IDependencyObject** ppDO,
            _Out_opt_ BOOLEAN* pHasNativeRelative = NULL);

    private:
        static _Check_return_ HRESULT GetParentStaticPrivate(
            _In_ xaml::IDependencyObject* pReference,
            _In_ BOOLEAN isForAccessibleParentOnly,
            _Out_ xaml::IDependencyObject** ppDO,
            _Out_opt_ BOOLEAN* pHasNativeParent = NULL,
            _Out_opt_ BOOLEAN* pIsParentAccessible = NULL);

        static _Check_return_ HRESULT MakeUIElementList(
            _In_reads_(nCount) CUIElement** pElements,
            _In_ XUINT32 nCount,
            _Out_ wfc::IIterable<xaml::UIElement*>** ppElements);

    };
}
