// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AppBarSeparator.g.h"
#include "CommandBar.g.h"

using namespace DirectUI;
using namespace ::Windows::Internal;

_Check_return_
HRESULT AppBarSeparator::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(AppBarSeparatorGenerated::OnPropertyChanged2(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::AppBarSeparator_IsCompact ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::AppBarSeparator_UseOverflowStyle)
    {
        IFC(UpdateVisualState());
    }

Cleanup:
    RRETURN(hr);
}

// After template is applied, set the initial view state
// (FullSize or Compact) based on the value of our
// IsCompact property
IFACEMETHODIMP AppBarSeparator::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    IFC(AppBarSeparatorGenerated::OnApplyTemplate());

    // Set the initial view state
    IFC(UpdateVisualState());

Cleanup:
    RRETURN(hr);
}

// Sets the visual state to "Compact" or "FullSize" based on the value
// of our IsCompact property.
_Check_return_ HRESULT
AppBarSeparator::ChangeVisualState(_In_ bool useTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN ignored = FALSE;
    BOOLEAN isCompact = FALSE;
    BOOLEAN useOverflowStyle = FALSE;

    IFC(AppBarSeparatorGenerated::ChangeVisualState(useTransitions));

    IFC(get_UseOverflowStyle(&useOverflowStyle));
    IFC(get_IsCompact(&isCompact));
    if (useOverflowStyle)
    {
        IFC(GoToState(useTransitions, L"Overflow", &ignored));
    }
    else if (isCompact)
    {
        IFC(GoToState(useTransitions, L"Compact", &ignored));
    }
    else
    {
        IFC(GoToState(useTransitions, L"FullSize", &ignored));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
AppBarSeparator::OnVisibilityChanged()
{
    return CommandBar::OnCommandBarElementVisibilityChanged(this);
}

_Check_return_ HRESULT
AppBarSeparator::get_IsInOverflowImpl(_Out_ BOOLEAN* pValue)
{
    return CommandBar::IsCommandBarElementInOverflow(this, pValue);
}