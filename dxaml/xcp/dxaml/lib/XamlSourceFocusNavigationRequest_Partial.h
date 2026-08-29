// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlSourceFocusNavigationRequest.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(XamlSourceFocusNavigationRequest)
    {
    public:
        XamlSourceFocusNavigationRequest();

        _Check_return_ HRESULT Initialize(
            xaml_hosting::XamlSourceFocusNavigationReason m_reason,
            wf::Rect m_origin,
            GUID m_correlationId);

        _Check_return_ HRESULT Initialize(
            xaml_hosting::XamlSourceFocusNavigationReason m_reason,
            wf::Rect m_origin);

        _Check_return_ HRESULT Initialize(
            xaml_hosting::XamlSourceFocusNavigationReason m_reason);

        ~XamlSourceFocusNavigationRequest() override;

        // Properties.
        _Check_return_ HRESULT get_CorrelationIdImpl(_Out_ GUID* pValue);
        _Check_return_ HRESULT get_HintRectImpl(_Out_ wf::Rect* pValue);
        _Check_return_ HRESULT get_ReasonImpl(_Out_ xaml_hosting::XamlSourceFocusNavigationReason* pValue);

    private:
        GUID m_correlationId = {};
        wf::Rect m_origin = {};
        xaml_hosting::XamlSourceFocusNavigationReason m_reason = {};
    };
}

