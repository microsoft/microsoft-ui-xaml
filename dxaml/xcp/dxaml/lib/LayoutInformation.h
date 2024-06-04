// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class LayoutInformation: 
        public xaml_primitives::ILayoutInformation,
        public xaml_primitives::ILayoutInformationStatics,
        public ctl::AbstractActivationFactory
    {

    BEGIN_INTERFACE_MAP(LayoutInformation, ctl::AbstractActivationFactory)
        INTERFACE_ENTRY(LayoutInformation,  xaml_primitives::ILayoutInformation)
        INTERFACE_ENTRY(LayoutInformation,  xaml_primitives::ILayoutInformationStatics)
    END_INTERFACE_MAP(LayoutInformation, ctl::AbstractActivationFactory)

    public:

        IFACEMETHOD(GetLayoutExceptionElement)(_In_ IInspectable* pDispatcher, _Outptr_ xaml::IUIElement** ppElement) override;
        IFACEMETHOD(GetLayoutSlot)(_In_ xaml::IFrameworkElement* pElement, _Out_ wf::Rect* pSlot) override;
        IFACEMETHOD(GetAvailableSize)(_In_ xaml::IUIElement* element, _Out_ wf::Size* availableSize) override;

    protected:
        
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
        LayoutInformation();

    private:

        _Check_return_ HRESULT GetRawLayoutData(
            _In_ xaml::IFrameworkElement* pElement,
            _Out_ XUINT32& cFloats,
            _Out_writes_(cFloats) XFLOAT*& pFloats);
    };
}
