// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      An entry corresponding to each page in the Navigation stack.

#pragma once

#include "PageStackEntry.g.h"

namespace DirectUI
{
    // Entry in NavigationHistory
    PARTIAL_CLASS(PageStackEntry)
    {
        
    private:
        // Descriptor -- this is the type of the page that corresponds to this entry
        wrl_wrappers::HString m_descriptor;

        // Frame which owns Navigation History and its PageStackEntries
        ctl::WeakRefPtr m_wrFrame;

        _Check_return_ HRESULT
        GetFrame(
            _Outptr_ xaml_controls::IFrame** ppIFrame);            

    public:
    
        static _Check_return_ HRESULT
        Create(
            _In_ xaml_controls::IFrame *pIFrame,
            _In_ HSTRING descriptor,
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo* pTransitionInfo, 
            _Outptr_ PageStackEntry **ppPageStackEntry);

        _Check_return_ HRESULT
        PrepareContent(
            _In_ IInspectable *pContentIInspectable);

        _Check_return_ HRESULT
        GetDescriptor(
            _Out_ HSTRING *pDescriptor);

        _Check_return_ HRESULT
        SetDescriptor(
            _In_ HSTRING descriptor);

        _Check_return_ HRESULT
        SetFrame(
            _In_ xaml_controls::IFrame* pIFrame);

        _Check_return_ HRESULT
        CanBeAddedToFrame(
            _In_ xaml_controls::IFrame* pIFrame, 
            _Out_ BOOLEAN* pCanAdd);
    };
}
