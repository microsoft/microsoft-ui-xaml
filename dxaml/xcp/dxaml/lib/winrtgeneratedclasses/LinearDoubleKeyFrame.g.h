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

#include "DoubleKeyFrame.g.h"

#define __LinearDoubleKeyFrame_GUID "8344399d-36a4-4cee-834e-11f99e20d760"

namespace DirectUI
{
    class LinearDoubleKeyFrame;

    class __declspec(novtable) __declspec(uuid(__LinearDoubleKeyFrame_GUID)) LinearDoubleKeyFrame:
        public DirectUI::DoubleKeyFrame
        , public ABI::Microsoft::UI::Xaml::Media::Animation::ILinearDoubleKeyFrame
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.LinearDoubleKeyFrame");

        BEGIN_INTERFACE_MAP(LinearDoubleKeyFrame, DirectUI::DoubleKeyFrame)
            INTERFACE_ENTRY(LinearDoubleKeyFrame, ABI::Microsoft::UI::Xaml::Media::Animation::ILinearDoubleKeyFrame)
        END_INTERFACE_MAP(LinearDoubleKeyFrame, DirectUI::DoubleKeyFrame)

    public:
        LinearDoubleKeyFrame();
        ~LinearDoubleKeyFrame() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::LinearDoubleKeyFrame;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::LinearDoubleKeyFrame;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}

