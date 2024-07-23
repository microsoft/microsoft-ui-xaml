// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "microsoft.ui.input.h"

namespace DirectUI
{
    class XamlIslandRoot;

    class PointerPointTransform :
        public mui::IPointerPointTransform,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(PointerPointTransform, ctl::ComBase)
            INTERFACE_ENTRY(PointerPointTransform, mui::IPointerPointTransform)
        END_INTERFACE_MAP(PointerPointTransform, ctl::ComBase)

    public:
        PointerPointTransform();
        ~PointerPointTransform() override;

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        // IPointerPointTransform Methods
        IFACEMETHOD(get_Inverse)(
            _Outptr_ mui::IPointerPointTransform **ppValue) override;
        IFACEMETHOD(TryTransform)(
            _In_ wf::Point inPoint,
            _Out_ wf::Point * outPoint,
            _Out_ BOOLEAN * returnValue) override;
        IFACEMETHOD(TryTransformBounds)(
            _In_ wf::Rect inRect,
            _Out_ wf::Rect * outRect,
            _Out_ BOOLEAN * returnValue) override;

        _Check_return_ HRESULT SetTransform(
            _In_ xaml_media::IGeneralTransform* pTransform,
            _In_opt_ wf::Point *pWindowTranslation,
            _In_ bool isInverse);

        _Check_return_ static HRESULT CreatePointerPointTransform(
            _In_opt_ xaml::IUIElement *pRelativeTo,
            _Outptr_ mui::IPointerPointTransform **ppPointerPointTransform);

        _Check_return_ static HRESULT CreatePointerPointTransform(
            _In_opt_ xaml::IUIElement* pRelativeTo,
            _In_opt_ wf::Point* pWindowTranslation,
            _Outptr_ mui::IPointerPointTransform** ppPointerPointTransform);

    private :
        // It's not necessary for this to be a TrackerPtr; it's only used to interact with core input.
        xaml_media::IGeneralTransform *m_pTransform;

        // Optional translation from Pointer's target window to Jupiter window, used only for windowed popups
        wf::Point m_windowTranslation;

        // Is this an inverse transform?
        bool m_isInverse;
    };
}
