// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once


#define __DragUIOverride_GUID "a1fcdb3c-2a9e-488a-a3e6-3ac7db0e1cfa"

namespace DirectUI
{
    class DragUIOverride;
    class BitmapImage;

    class __declspec(novtable) DragUIOverrideGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::IDragUIOverride
    {
        friend class DirectUI::DragUIOverride;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.DragUIOverride");

        BEGIN_INTERFACE_MAP(DragUIOverrideGenerated, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(DragUIOverrideGenerated, ABI::Microsoft::UI::Xaml::IDragUIOverride)
        END_INTERFACE_MAP(DragUIOverrideGenerated, ctl::WeakReferenceSource)

    public:
        DragUIOverrideGenerated();
        ~DragUIOverrideGenerated() override;

        // Event source typedefs.


        // Properties.
        IFACEMETHOD(get_Caption)(_Out_ HSTRING* pValue) override;
        IFACEMETHOD(put_Caption)(_In_opt_ HSTRING value) override;
        IFACEMETHOD(get_IsCaptionVisible)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsCaptionVisible)(_In_ BOOLEAN value) override;
        IFACEMETHOD(get_IsContentVisible)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsContentVisible)(_In_ BOOLEAN value) override;
        IFACEMETHOD(get_IsGlyphVisible)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_IsGlyphVisible)(_In_ BOOLEAN value) override;

        // Events.

        // Methods.
        IFACEMETHOD(Clear)() override;
        IFACEMETHOD(SetContentFromBitmapImage)(_In_ ABI::Microsoft::UI::Xaml::Media::Imaging::IBitmapImage* pBitmapImage) override;
        IFACEMETHOD(SetContentFromBitmapImageWithAnchorPoint)(_In_ ABI::Microsoft::UI::Xaml::Media::Imaging::IBitmapImage* pBitmapImage, _In_ ABI::Windows::Foundation::Point anchorPoint) override;
        IFACEMETHOD(SetContentFromSoftwareBitmap)(_In_ ABI::Windows::Graphics::Imaging::ISoftwareBitmap* pSoftwareBitmap) override;
        IFACEMETHOD(SetContentFromSoftwareBitmapWithAnchorPoint)(_In_ ABI::Windows::Graphics::Imaging::ISoftwareBitmap* pSoftwareBitmap, _In_ ABI::Windows::Foundation::Point anchorPoint) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "DragUIOverride_Partial.h"
