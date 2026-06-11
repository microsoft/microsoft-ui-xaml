// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace DirectUI;

_Check_return_ HRESULT CRangeBase::GetDoubleValueHelper(
    KnownPropertyIndex valueIndex,
    _Out_ double& result)
{
    CValue value;
    IFC_RETURN(GetValue(MetadataAPI::GetDependencyPropertyByIndex(valueIndex), &value));
    IFC_RETURN(value.Get<valueDouble>(result));
    return S_OK;
}

_Check_return_ HRESULT CRangeBase::SetDoubleValueHelper(
    _In_opt_ const SetValueParams* args,
    KnownPropertyIndex valueIndex,
    double value)
{
    CValue coercedValue;

    coercedValue.Set<valueDouble>(value);

    if (args)
    {
        ASSERT(args->m_pDP->GetIndex() == valueIndex);

        IFC_RETURN(CControl::SetValue(
            SetValueParams(
                *args,
                coercedValue)));
    }
    else
    {
        const CDependencyProperty* property = MetadataAPI::GetDependencyPropertyByIndex(valueIndex);

        IFC_RETURN(CControl::SetValue(
            SetValueParams(
                property,
                coercedValue,
                GetBaseValueSource(property))));
    }

    return S_OK;
}

_Check_return_ HRESULT CRangeBase::CoerceAndSetValue(
    _In_opt_ const SetValueParams* args,
    double min,
    double max)
{
    IFC_RETURN(SetDoubleValueHelper(
        args,
        KnownPropertyIndex::RangeBase_Value,
        std::max(
            std::min(m_uncoercedValue, max),
            min)));

    return S_OK;
}

_Check_return_ HRESULT CRangeBase::SetMaximum(
    _In_opt_ const SetValueParams* args,
    double max)
{
    IFC_RETURN(SetDoubleValueHelper(
        args,
        KnownPropertyIndex::RangeBase_Maximum,
        max));

    return S_OK;
}

_Check_return_ HRESULT CRangeBase::SetValue(_In_ const SetValueParams& args)
{
    bool wasHandled = false;

    if (args.m_pDP)
    {
        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::RangeBase_Value:
                {
                    m_uncoercedValue = args.m_value.As<valueDouble>();

                    double min = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Minimum, min));

                    double max = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Maximum, max));

                    IFC_RETURN(CoerceAndSetValue(&args, min, max));

                    wasHandled = true;
                }
                break;

            case KnownPropertyIndex::RangeBase_Minimum:
                {
                    double newMin = args.m_value.As<valueDouble>();
                    double oldMin = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Minimum, oldMin));
                    double max = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Maximum, max));

                    if (newMin <= oldMin)
                    {
                        // expanding range (decreasing minimum)

                        // set minimum
                        IFC_RETURN(CControl::SetValue(args));

                        // update value
                        IFC_RETURN(CoerceAndSetValue(nullptr, newMin, max));
                    }
                    else
                    {
                        // contracting range (increasing minimum)

                        if (newMin > max)
                        {
                            // coerce and update maximum
                            max = newMin;
                            IFC_RETURN(SetMaximum(nullptr, max));
                        }

                        // set value
                        IFC_RETURN(CoerceAndSetValue(nullptr, newMin, max));

                        // set minimum
                        IFC_RETURN(CControl::SetValue(args));
                    }

                    wasHandled = true;
                }
                break;

            case KnownPropertyIndex::RangeBase_Maximum:
                {
                    double newMax = args.m_value.As<valueDouble>();
                    double oldMax = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Maximum, oldMax));
                    double min = 0.0;
                    IFC_RETURN(GetDoubleValueHelper(KnownPropertyIndex::RangeBase_Minimum, min));

                    if (newMax >= oldMax)
                    {
                        // expanding range (increasing maximum)

                        // set maximum
                        IFC_RETURN(CControl::SetValue(args));

                        // set value
                        IFC_RETURN(CoerceAndSetValue(nullptr, min, newMax));
                    }
                    else
                    {
                        // contracting range (decreasing maximum)

                        // keep maximum higher than minimum
                        newMax = std::max(newMax, min);

                        // coerce and set value
                        IFC_RETURN(CoerceAndSetValue(nullptr, min, newMax));

                        // set maximum
                        IFC_RETURN(SetMaximum(&args, newMax));
                    }

                    wasHandled = true;
                }
                break;
        }
    }

    if (!wasHandled)
    {
        IFC_RETURN(CControl::SetValue(args));
    }

    return S_OK;
}