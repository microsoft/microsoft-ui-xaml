// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Private
{
    class ValueConversionHelpers
    {
    public:
        static
        BOOLEAN CanConvertValueToString(
            _In_ wf::PropertyType propertyType);

        static
        _Check_return_ HRESULT
        ConvertValueToString(
            _In_ wf::IPropertyValue* pPropertyValue,
            _In_ wf::PropertyType propertyType,
            _Out_ HSTRING *pResult);

    private:
        static
        _Check_return_ HRESULT
        ConvertGuidToString(
            _In_ GUID guid,
            _Out_ HSTRING *pResult);
    };

    class ValueComparisonHelpers
    {
    public:
        static _Check_return_ HRESULT AreEqual(
            _In_ IInspectable* pValue1,
            _In_ IInspectable* pValue2,
            _Out_ BOOLEAN* pAreEqual);
    };
}
