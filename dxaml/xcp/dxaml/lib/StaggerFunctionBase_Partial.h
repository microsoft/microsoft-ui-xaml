// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StaggerFunctionBase.g.h"

namespace DirectUI
{
    // Provides /data for the SelectionChanged event that occurs when a user
    // changes selected item(s) on a Selector based control
    PARTIAL_CLASS(StaggerFunctionBase)
    {
    protected:
        virtual _Check_return_ HRESULT GetTransitionDelays(_In_ wfc::IVector<xaml::DependencyObject*>* staggerItems) { RRETURN(E_NOTIMPL); }


    // callbacks
    public:
        static _Check_return_ HRESULT GetTransitionDelayValues(
            _In_ CDependencyObject* pStaggerFunction,
            _In_ XINT32 cElements,
            _In_reads_(cElements) CUIElement** ppElements,
            _In_reads_(cElements) XRECTF *pBounds,
            _Out_writes_(cElements) XFLOAT* pDelays);
    };

}
