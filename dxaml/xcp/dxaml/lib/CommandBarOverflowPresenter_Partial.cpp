// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CommandBarOverflowPresenter.g.h"

using namespace DirectUI;
using namespace std::placeholders;

CommandBarOverflowPresenter::CommandBarOverflowPresenter()
    : m_useFullWidth(false)
    , m_shouldOpenUp(false)
{
}

IFACEMETHODIMP CommandBarOverflowPresenter::OnApplyTemplate()
{
    IFC_RETURN(__super::OnApplyTemplate());

    IFC_RETURN(UpdateVisualState(false));

    return S_OK;
}

_Check_return_ HRESULT CommandBarOverflowPresenter::SetDisplayModeState(bool isFullWidth, bool isOpenUp)
{
    m_useFullWidth = isFullWidth;
    m_shouldOpenUp = isOpenUp;

    IFC_RETURN(UpdateVisualState(false));

    return S_OK;
}

_Check_return_ HRESULT CommandBarOverflowPresenter::ChangeVisualState(bool useTransitions)
{
    BOOLEAN ignored = FALSE;
    IFC_RETURN(__super::ChangeVisualState(useTransitions));

    if (m_useFullWidth && m_shouldOpenUp)
    {
        IFC_RETURN(GoToState(useTransitions, L"FullWidthOpenUp", &ignored));
    }
    else if (m_useFullWidth && !m_shouldOpenUp)
    {
        IFC_RETURN(GoToState(useTransitions, L"FullWidthOpenDown", &ignored));
    }
    else
    {
        IFC_RETURN(GoToState(useTransitions, L"DisplayModeDefault", &ignored));
    }

    return S_OK;
}