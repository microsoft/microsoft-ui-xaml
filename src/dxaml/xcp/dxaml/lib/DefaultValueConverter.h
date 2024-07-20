// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    MIDL_INTERFACE("86ab9557-337a-47d7-96b6-178a8d9d99f9")
    IValueConverterInternal: IUnknown
    {
        virtual _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue) = 0;
        virtual _Check_return_ HRESULT ConvertBack(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue) = 0;
    };

    //
    //  DefaultValueConverter class
    //
    //  This is a value converter that figures out how to convert between types
    //
    class DefaultValueConverter
    {
        friend class DynamicValueConverter;

    private:
        static _Check_return_ HRESULT CreateConverter(_In_ const CClassInfo *pSourceType,
                                                     _In_ const CClassInfo *pTargetType,
                                                     _Outptr_ IValueConverterInternal **ppConverter);

    protected:
        //
        //  DefaultValueConverter (constructor)
        //
        DefaultValueConverter(_In_ const CClassInfo *pSourceType,
                              _In_ const CClassInfo *pTargetType)
        {
            m_pSourceType = pSourceType;
            m_pTargetType = pTargetType;
        }

        // DefaultValueConverter base abstract class for all type converters.
        // We don't want to able to instantiate it.
        virtual ~DefaultValueConverter() = 0;

        //
        //  ConvertFrom
        //
        //  Protected implementation for subclasses
        //
        _Check_return_ HRESULT ConvertFrom(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType, /*_In_ CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue)

        {
            RRETURN(ConvertHelper(pSource, pTargetType, /*culture,*/ /*fForward*/FALSE, ppTargetValue));
        }

        //
        //  ConvertTo
        //
        //  Protected implementation for subclasses
        //
        _Check_return_ HRESULT ConvertTo(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue)
        {
            RRETURN(ConvertHelper(pTarget, pSourceType, /*culture,*/ /*fForward*/TRUE, ppSourceValue));
        }

    private:
        //
        //  ConvertHelper
        //
        //  Worker for ConvertTo and ConvertFrom.
        //
        _Check_return_ HRESULT ConvertHelper(
            _In_ IInspectable *pValue,
            _In_ const CClassInfo *pDestinationType, /*CultureInfo culture,*/
            _In_ bool fForward,
            _Outptr_ IInspectable **ppDestinationValue);

    private:
        //
        //  State
        //
        const CClassInfo *m_pSourceType;
        const CClassInfo *m_pTargetType;
    };

    class BoolToVisibilityValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:
        BoolToVisibilityValueConverter() = default;

    public:
        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);

        //  Convert from source to target
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue) override;

        //  Convert from target to source
        _Check_return_ HRESULT ConvertBack(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue) override;

    };

    class NumberStringValueConverter :
        public DefaultValueConverter,
        public ctl::implements<IValueConverterInternal>
    {
        friend class DefaultValueConverter;

    private:
        static _Check_return_ HRESULT CreateConverter(_In_ const CClassInfo *pNumberType,
                                                     _In_ bool fStringToNumber,
                                                     _Outptr_ IValueConverterInternal **ppConverter);

        NumberStringValueConverter(_In_ const CClassInfo *pSourceType,
                                   _In_ const CClassInfo *pTargetType):
            DefaultValueConverter(pSourceType, pTargetType)
        {
        }

        ~NumberStringValueConverter() override {}

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue) override
        {
            RRETURN(ConvertFrom(pSource, pTargetType, /*culture,*/ ppTargetValue));
        }

        //
        //  ConvertBack
        //
        //  Convert from target to source
        //
        _Check_return_ HRESULT ConvertBack(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(ConvertTo(pTarget, pSourceType, /*culture,*/ ppSourceValue));
        }
    };

    class NumberTypeValueConverter :
        public DefaultValueConverter,
        public ctl::implements<IValueConverterInternal>
    {
        friend class DefaultValueConverter;

    private:
        static _Check_return_ HRESULT CreateConverter(_In_ const CClassInfo *pSourceType,
                                                     _In_ const CClassInfo *pTargetType,
                                                     _Outptr_ IValueConverterInternal **ppConverter);

        NumberTypeValueConverter(_In_ const CClassInfo *pSourceType,
                                 _In_ const CClassInfo *pTargetType):
            DefaultValueConverter(pSourceType, pTargetType)
        {
        }

        ~NumberTypeValueConverter() override {}

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppTargetValue) override
        {
            RRETURN(ConvertFrom(pSource, pTargetType, /*culture,*/ ppTargetValue));
        }

        //
        //  ConvertBack
        //
        //  Convert from target to source
        //
        _Check_return_ HRESULT ConvertBack(
            _In_ IInspectable *pTarget,
            _In_ const CClassInfo *pSourceType,
            _In_opt_ IUnknown * pConverterParameter, /*CultureInfo culture,*/
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(ConvertTo(pTarget, pSourceType, /*culture,*/ ppSourceValue));
        }
    };

    class StringToUriValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:

        StringToUriValueConverter()
        { }

    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(E_FAIL);
        }

    public:

        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };

    class UriToStringValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:

        UriToStringValueConverter()
        { }

    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(E_FAIL);
        }

    public:

        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };

    class StringToGuidValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:

        StringToGuidValueConverter()
        { }

    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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

    public:

        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };

    class GuidToStringValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:

        GuidToStringValueConverter()
        { }

    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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

    public:

        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };



    // This class converts from a uri to an object using
    // the Core type converters
    class CoreUriToValueConverter : public ctl::implements<IValueConverterInternal>
    {
    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(E_FAIL);
        }

    public:
        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };

    // This class converts from a string to an object using
    // the Core type converters
    class CoreValueConverter : public ctl::implements<IValueConverterInternal>
    {
    protected:
        ~CoreValueConverter() override
        { }

    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(E_FAIL);
        }

    public:
        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
        static _Check_return_ HRESULT ConvertStringToCoreValue(_In_ const CClassInfo *pTargetType, _In_ HSTRING hValue, _Outptr_ IInspectable **ppTargetValue);
    };

    // This class converts always to a string
    class StringValueConverter : public ctl::implements<IValueConverterInternal>
    {
    public:

        //
        //  Convert
        //
        //  Convert from source to target
        //
        _Check_return_ HRESULT Convert(
            _In_ IInspectable *pSource,
            _In_ const CClassInfo *pTargetType,
            _In_opt_ IUnknown *pConverterParameter, /*CultureInfo culture,*/
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
            _Outptr_ IInspectable **ppSourceValue) override
        {
            RRETURN(E_FAIL);
        }

    public:

        static _Check_return_ HRESULT CreateConverter(_Outptr_ IValueConverterInternal **ppNewConverter);
    };

    class ValueConversionHelpers
    {
    public:
        static bool
        CanConvertValueToString(
            wf::PropertyType propertyType);

        static _Check_return_ HRESULT
        ConvertValueToString(
            _In_ wf::IPropertyValue* pPropertyValue,
            _In_ wf::PropertyType propertyType,
            _Out_ HSTRING *phstr);

        static _Check_return_ HRESULT
        ConvertStringToValue(
            _In_ HSTRING phstr,
            _In_ wf::PropertyType propertyType,
            _Out_ IInspectable** ppPropertyValue);

    private:
        static _Check_return_ HRESULT
        ConvertGuidToString(
            _In_ GUID guid,
            _Out_ HSTRING *phstr);

        static _Check_return_ HRESULT
       ConvertStringToGuid(
            _In_ HSTRING phstr,
            _Out_ GUID *pguid);
    };
}

