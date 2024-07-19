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

#define __PickerFlyoutThemeTransition_GUID "b282fe7e-2f31-4fa3-b8f1-1797a0eb3ad1"

namespace DirectUI
{
    class PickerFlyoutThemeTransition;

    class __declspec(novtable) PickerFlyoutThemeTransitionGenerated:
        public DirectUI::Transition
    {
        friend class DirectUI::PickerFlyoutThemeTransition;



    public:
        PickerFlyoutThemeTransitionGenerated();
        ~PickerFlyoutThemeTransitionGenerated() override;

        // Event source typedefs.

        KnownTypeIndex GetTypeIndex() const override
        {
            return KnownTypeIndex::PickerFlyoutThemeTransition;
        }

        static XCP_FORCEINLINE KnownTypeIndex GetTypeIndexStatic()
        {
            return KnownTypeIndex::PickerFlyoutThemeTransition;
        }

        // Properties.
        _Check_return_ HRESULT get_OffsetFromCenter(_Out_ DOUBLE* pValue);
        _Check_return_ HRESULT put_OffsetFromCenter(_In_ DOUBLE value);
        _Check_return_ HRESULT get_OpenedLength(_Out_ DOUBLE* pValue);
        _Check_return_ HRESULT put_OpenedLength(_In_ DOUBLE value);

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "PickerFlyoutThemeTransition_Partial.h"

