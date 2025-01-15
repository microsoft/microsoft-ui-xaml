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

#include "Transition.g.h"

#define __ReorderThemeTransition_GUID "e23a139c-a080-4357-aaad-224526007562"

namespace DirectUI
{
    class ReorderThemeTransition;

    class __declspec(novtable) ReorderThemeTransitionGenerated:
        public DirectUI::Transition
        , public ABI::Microsoft::UI::Xaml::Media::Animation::IReorderThemeTransition
    {
        friend class DirectUI::ReorderThemeTransition;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Media.Animation.ReorderThemeTransition");

        BEGIN_INTERFACE_MAP(ReorderThemeTransitionGenerated, DirectUI::Transition)
            INTERFACE_ENTRY(ReorderThemeTransitionGenerated, ABI::Microsoft::UI::Xaml::Media::Animation::IReorderThemeTransition)
        END_INTERFACE_MAP(ReorderThemeTransitionGenerated, DirectUI::Transition)

    public:
        ReorderThemeTransitionGenerated();
        ~ReorderThemeTransitionGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::ReorderThemeTransition;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::ReorderThemeTransition;
        }

        // Properties.

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "ReorderThemeTransition_Partial.h"
