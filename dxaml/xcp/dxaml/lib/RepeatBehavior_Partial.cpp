// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RepeatBehavior.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares two RepeatBehaviors for equality.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::Equals(
    _In_ xaml_animation::RepeatBehavior target,
    _In_ xaml_animation::RepeatBehavior value,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == value.Type)
    {
        switch (target.Type)
        {
            case xaml_animation::RepeatBehaviorType_Count:
                *pReturnValue = (target.Count == value.Count);
                break;

            case xaml_animation::RepeatBehaviorType_Duration:
                *pReturnValue = (target.Duration.Duration == value.Duration.Duration);
                break;

            case xaml_animation::RepeatBehaviorType_Forever:
                *pReturnValue = TRUE;
                break;

            default:
                *pReturnValue = FALSE;
                break;
        }
    }
    else
    {
        *pReturnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a RepeatBehavior from the specified count.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::FromCount(
    _In_ DOUBLE count,
    _Out_ xaml_animation::RepeatBehavior* pReturnValue)
{
    HRESULT hr = S_OK;
    wf::TimeSpan duration = {0};

    IFCPTR(pReturnValue);

    if (DoubleUtil::IsInfinity(count) || DoubleUtil::IsNaN(count) || count < 0.0)
    {

        RRETURN(E_INVALIDARG);
    }

    pReturnValue->Count = count;
    pReturnValue->Duration = duration;
    pReturnValue->Type = xaml_animation::RepeatBehaviorType_Count;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a RepeatBehavior from the specified duration.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::FromDuration(
    _In_ wf::TimeSpan duration,
    _Out_ xaml_animation::RepeatBehavior* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (duration.Duration < 0)
    {

        RRETURN(E_INVALIDARG);
    }

    pReturnValue->Count = 0.0;
    pReturnValue->Duration = duration;
    pReturnValue->Type = xaml_animation::RepeatBehaviorType_Duration;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Indicates whether this RepeatBehavior represents a count.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::GetHasCount(
    _In_ xaml_animation::RepeatBehavior target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.Type == xaml_animation::RepeatBehaviorType_Count);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Indicates whether this RepeatBehavior represents a duration.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::GetHasDuration(
    _In_ xaml_animation::RepeatBehavior target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.Type == xaml_animation::RepeatBehaviorType_Duration);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a RepeatBehavior that represents a Forever value.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
RepeatBehaviorFactory::get_Forever(
    _Out_opt_ xaml_animation::RepeatBehavior *pValue)
{
    RRETURN(RepeatBehaviorHelper::GetForever(pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper method for the Forever property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
RepeatBehaviorHelper::GetForever(
    _Out_opt_ xaml_animation::RepeatBehavior *pValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(pValue);
    pValue->Count = 0.0;
    pValue->Duration.Duration = 0;
    pValue->Type = xaml_animation::RepeatBehaviorType_Forever;

Cleanup:
    RRETURN(hr);
}

