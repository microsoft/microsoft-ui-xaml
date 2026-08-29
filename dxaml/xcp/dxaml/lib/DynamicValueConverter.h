// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DefaultValueConverter.h"

namespace DirectUI
{
    //
    //  DynamicValueConverter
    //
    //  Dynamically pick and switch a default value converter to convert between source and target type
    //
    class DynamicValueConverter
        : public ctl::implements<IValueConverterInternal>
    {
    public:
        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppConverter);

    private:
        //
        //  DynamicValueConverter (constructor)
        //
        //  targetToSourceNeeded indicates if the binding is two-way.
        //
        DynamicValueConverter():
            m_pConverter(NULL),
            m_pSourceType(NULL),
            m_pTargetType(NULL)
        {
        }

        ~DynamicValueConverter() override
        {
            ReleaseInterface(m_pConverter);
        }

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue) override;

        //
        //  ConvertBack
        //
        //  Convert from target to source
        //
        _Check_return_ HRESULT ConvertBack(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue) override;

    private:
        //
        //  EnsureConverter
        //
        //  Create the appropriate converter for the source & target types
        //
        _Check_return_ HRESULT EnsureConverter(
            _In_ const CClassInfo *pSourceType,
            _In_ const CClassInfo *pTargetType);

    private:
        //
        //  State
        //
        const CClassInfo *m_pSourceType;
        const CClassInfo *m_pTargetType;
        IValueConverterInternal *m_pConverter;
    };
}

