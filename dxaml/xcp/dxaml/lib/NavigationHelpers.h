// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class NavigationHelpers
    {
    public:
        static _Check_return_ HRESULT
        CreateINavigationEventArgs(
            _In_ IInspectable *pContentIInspectable,
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode,
            _Outptr_ xaml::Navigation::INavigationEventArgs **ppINavigationEventArgs);

        static _Check_return_ HRESULT
        CreateINavigatingCancelEventArgs(
            _In_opt_ IInspectable *pParameterIInspectable,
            _In_opt_ xaml_animation::INavigationTransitionInfo *pTransitionInfo,
            _In_ HSTRING descriptor,
            _In_ xaml::Navigation::NavigationMode navigationMode,
            _Outptr_ xaml::Navigation::INavigatingCancelEventArgs **ppINavigatingCancelEventArgs);

        static _Check_return_ HRESULT
        WriteUINT32ToString(
            _In_ UINT32 value,
            _Inout_ string &buffer);

        static _Check_return_ HRESULT
        ReadUINT32FromString(
            _In_ string &buffer,
            size_t currentPosition,
            _Out_ UINT32* pValue,
            _Out_ size_t* pNextPosition);

        static _Check_return_ HRESULT
        WriteHSTRINGToString(
            _In_opt_ HSTRING hstr,
            _Inout_ string &buffer);

        static _Check_return_ HRESULT
        ReadHSTRINGFromString(
            _In_ string &buffer,
            size_t currentPosition,
            _Out_ HSTRING* phstr,
            _Out_ size_t* pNextPosition);

        static _Check_return_ HRESULT
        WriteNavigationParameterToString(
            _In_opt_ IInspectable* pPropertyValue,
            _Inout_ string &buffer,
            _Out_ BOOLEAN *pIsParameterTypeSupported);

        static _Check_return_ HRESULT
        ReadNavigationParameterFromString(
            _In_ string &buffer,
            size_t currentPosition,
            _Out_ IInspectable** ppPropertyValue,
            _Out_ size_t* pNextPosition);

    private:
        static _Check_return_ HRESULT
        ReadNextSubString(
            _In_ string &buffer,
            size_t currentPosition,
            _Inout_ string &subString,
            _Out_ size_t* pNextPosition);

        static _Check_return_ HRESULT
        ReadNextSubString(
            _In_ string &buffer,
            size_t currentPosition,
            size_t subStringLength,
            _Inout_ string &subString,
            _Out_ size_t* pNextPosition);

        static _Check_return_ HRESULT
        ConvertNavigationParameterToHSTRING(
            _In_ IInspectable *pNavigatonParameter,
            _Out_ HSTRING *phstr,
            _Out_ wf::PropertyType *pParameterType,
            _Out_ BOOLEAN *pIsConversionSupported);

        static _Check_return_ HRESULT
        ConvertHSTRINGToNavigationParameter(
            _In_ HSTRING hstr,
            _In_ wf::PropertyType propertyType,
            _Out_ IInspectable** ppNavigationParameter,
            _Out_ BOOLEAN *pIsConversionSupported);
    };
}
