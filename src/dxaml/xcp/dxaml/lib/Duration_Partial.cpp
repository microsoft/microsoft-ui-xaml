// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Duration.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares two Durations for equality.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::Equals(
    _In_ xaml::Duration target,
    _In_ xaml::Duration value,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == xaml::DurationType_TimeSpan)
    {
        if (value.Type == xaml::DurationType_TimeSpan)
        {
            *pReturnValue = (target.TimeSpan.Duration == value.TimeSpan.Duration);
        }
        else
        {
            *pReturnValue = FALSE;
        }
    }
    else
    {
        *pReturnValue = (target.Type == value.Type);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a Duration from the specified time span.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::FromTimeSpan(
    _In_ wf::TimeSpan timeSpan,
    _Out_ xaml::Duration* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (timeSpan.Duration < 0)
    {
        RRETURN(E_INVALIDARG);
    }

    pReturnValue->TimeSpan = timeSpan;
    pReturnValue->Type = xaml::DurationType_TimeSpan;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Indicates whether this Duration is a TimeSpan value.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::GetHasTimeSpan(
    _In_ xaml::Duration target,
    _Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = (target.Type == xaml::DurationType_TimeSpan);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the specified Duration to this instance.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::Add(
    _In_ xaml::Duration target,
    _In_ xaml::Duration duration,
    _Out_ xaml::Duration* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_TimeSpan)
    {
        pReturnValue->TimeSpan.Duration = (target.TimeSpan.Duration + duration.TimeSpan.Duration);
        pReturnValue->Type = xaml::DurationType_TimeSpan;
    }
    else if (target.Type != xaml::DurationType_Automatic && duration.Type != xaml::DurationType_Automatic)
    {
        // Neither durations are Automatic, so one is Forever while the other
        // is Forever or a TimeSpan.  Either way the sum is Forever.
        IFC(DurationHelper::GetForever(pReturnValue));
    }
    else
    {
        // Automatic + anything is Automatic.
        IFC(DurationHelper::GetAutomatic(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper method for the Automatic property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
DurationHelper::GetAutomatic(_Out_opt_ xaml::Duration *pValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(pValue);
    pValue->Type = xaml::DurationType_Automatic;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Static helper method for the Forever property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
DurationHelper::GetForever(_Out_opt_ xaml::Duration *pValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(pValue);
    pValue->Type = xaml::DurationType_Forever;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Indicates whether one Duration is greater than another.
//
//  Returns:
//      True if both durations have values and the value of this duration
//      is greater than the value of the passed in duration; otherwise false.
//      Forever is considered greater than all finite values and any
//      comparison with Automatic returns false.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
DurationHelper::GreaterThan(
    _In_ xaml::Duration target,
    _In_ xaml::Duration duration,
    _Out_ bool* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_TimeSpan)
    {
        *pReturnValue = (target.TimeSpan.Duration > duration.TimeSpan.Duration);
    }
    else if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_Forever)
    {
        // TimeSpan > Forever is false;
        *pReturnValue = FALSE;
    }
    else if (target.Type == xaml::DurationType_Forever && duration.Type == xaml::DurationType_TimeSpan)
    {

        // Forever > TimeSpan is true;
        *pReturnValue = TRUE;
    }
    else
    {
        // Cases covered:
        // Either t1 or t2 are Automatic, 
        // or t1 and t2 are both Forever 
        *pReturnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Indicates whether one Duration is less than another.
//
//  Returns:
//      True if both durations have values and the value of this duration
//      is less than the value of the passed in duration; otherwise false.
//      Forever is considered greater than all finite values and any
//      comparison with Automatic returns false.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
DurationHelper::LessThan(
    _In_ xaml::Duration target,
    _In_ xaml::Duration duration,
    _Out_ bool* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_TimeSpan)
    {
        *pReturnValue = (target.TimeSpan.Duration < duration.TimeSpan.Duration);
    }
    else if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_Forever)
    {
        // TimeSpan < Forever is true;
        *pReturnValue = TRUE;
    }
    else if (target.Type == xaml::DurationType_Forever && duration.Type == xaml::DurationType_TimeSpan)
    {

        // Forever < TimeSpan is false;
        *pReturnValue = FALSE;
    }
    else
    {
        // Cases covered:
        // Either t1 or t2 are Automatic, 
        // or t1 and t2 are both Forever 
        *pReturnValue = FALSE;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Subtracts the specified Duration from this instance.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::Subtract(
    _In_ xaml::Duration target,
    _In_ xaml::Duration duration,
    _Out_ xaml::Duration* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (target.Type == xaml::DurationType_TimeSpan && duration.Type == xaml::DurationType_TimeSpan)
    {
        pReturnValue->TimeSpan.Duration = (target.TimeSpan.Duration - duration.TimeSpan.Duration);
        pReturnValue->Type = xaml::DurationType_TimeSpan;
    }
    else if (target.Type == xaml::DurationType_Forever && duration.Type == xaml::DurationType_TimeSpan)
    {
        // The only way for the result to be Forever is
        // if t1 is Forever and t2 is a TimeSpan.
        IFC(DurationHelper::GetForever(pReturnValue));
    }
    else
    {
        // This covers the following conditions:
        // Forever - Forever
        // TimeSpan - Forever
        // TimeSpan - Automatic
        // Forever - Automatic
        // Automatic - Automatic
        // Automatic - Forever
        // Automatic - TimeSpan
        IFC(DurationHelper::GetAutomatic(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a Duration that represents an Automatic value.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::get_Automatic(
    _Out_ xaml::Duration *pValue)
{
    RRETURN(DurationHelper::GetAutomatic(pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a Duration that represents a Forever value.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::get_Forever(
    _Out_ xaml::Duration *pValue)
{
    RRETURN(DurationHelper::GetForever(pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares one Duration value to another.
//
//  Returns:
//      A negative value, zero or a positive value, respectively, if t1 is
//      less than, equal or greater than t2.
//
//      Duration.Automatic is a special case and has the following return values:
//
//       -1 if t1 is Automatic and t2 is not Automatic
//        0 if t1 and t2 are Automatic
//        1 if t1 is not Automatic and t2 is Automatic
//
//------------------------------------------------------------------------

IFACEMETHODIMP
DurationFactory::Compare(
    _In_ xaml::Duration t1,
    _In_ xaml::Duration t2,
    _Out_ INT* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    if (t1.Type == xaml::DurationType_Automatic)
    {
        if (t2.Type == xaml::DurationType_Automatic)
        {
            *pReturnValue = 0;
        }
        else
        {
            *pReturnValue = -1;
        }
    }
    else if (t2.Type == xaml::DurationType_Automatic)
    {
        *pReturnValue = 1;
    }
    else
    {
        bool comparison = false;
        IFC(DurationHelper::LessThan(t1, t2, &comparison));
        if (comparison)
        {
            *pReturnValue = -1;
        }
        else
        {
            IFC(DurationHelper::GreaterThan(t1, t2, &comparison));
            if (comparison)
            {
                *pReturnValue = 1;
            }
            else
            {
                *pReturnValue = 0;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

