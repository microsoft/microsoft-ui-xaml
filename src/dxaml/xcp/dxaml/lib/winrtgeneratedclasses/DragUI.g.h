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


#define __DragUI_GUID "f994a62f-b072-4e48-a337-ea367bc1a208"

namespace DirectUI
{
    class DragUI;
    class BitmapImage;

    class __declspec(novtable) DragUIGenerated:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::IDragUI
    {
        friend class DirectUI::DragUI;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.DragUI");

        BEGIN_INTERFACE_MAP(DragUIGenerated, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(DragUIGenerated, ABI::Microsoft::UI::Xaml::IDragUI)
        END_INTERFACE_MAP(DragUIGenerated, ctl::WeakReferenceSource)

    public:
        DragUIGenerated();
        ~DragUIGenerated() override;

        // Event source typedefs.


        // Properties.

        // Events.

        // Methods.
        IFACEMETHOD(SetContentFromBitmapImage)(_In_ ABI::Microsoft::UI::Xaml::Media::Imaging::IBitmapImage* pBitmapImage) override;
        IFACEMETHOD(SetContentFromBitmapImageWithAnchorPoint)(_In_ ABI::Microsoft::UI::Xaml::Media::Imaging::IBitmapImage* pBitmapImage, ABI::Windows::Foundation::Point anchorPoint) override;
        IFACEMETHOD(SetContentFromDataPackage)() override;
        IFACEMETHOD(SetContentFromSoftwareBitmap)(_In_ ABI::Windows::Graphics::Imaging::ISoftwareBitmap* pSoftwareBitmap) override;
        IFACEMETHOD(SetContentFromSoftwareBitmapWithAnchorPoint)(_In_ ABI::Windows::Graphics::Imaging::ISoftwareBitmap* pSoftwareBitmap, ABI::Windows::Foundation::Point anchorPoint) override;


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "DragUI_Partial.h"

