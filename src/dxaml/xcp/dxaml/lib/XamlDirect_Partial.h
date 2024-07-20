// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlDirect.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(XamlDirect)
    {
        friend class XamlDirectFactory;

    public:
        XamlDirect();
        ~XamlDirect() override;

        _Check_return_ HRESULT AddEventHandlerImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXaml, _In_ xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler);
        _Check_return_ HRESULT AddEventHandler_HandledEventsTooImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo);
        _Check_return_ HRESULT AddToCollectionImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::IXamlDirectObject* pValue);
        _Check_return_ HRESULT ClearCollectionImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject);
        _Check_return_ HRESULT ClearPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex);
        _Check_return_ HRESULT CreateInstanceImpl(_In_ xaml::Core::Direct::XamlTypeIndex typeIndex, _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT GetBooleanPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ BOOLEAN* pResult);
        _Check_return_ HRESULT GetCollectionCountImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Out_ UINT* pResult);
        _Check_return_ HRESULT GetColorPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wu::Color* pResult);
        _Check_return_ HRESULT GetCornerRadiusPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml::CornerRadius* pResult);
        _Check_return_ HRESULT GetDateTimePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wf::DateTime* pResult);
        _Check_return_ HRESULT GetObjectImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Outptr_ IInspectable** ppResult);
        _Check_return_ HRESULT GetDoublePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ DOUBLE* pResult);
        _Check_return_ HRESULT GetDurationPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml::Duration* pResult);
        _Check_return_ HRESULT GetEnumPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ UINT* pResult);
        _Check_return_ HRESULT GetGridLengthPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml::GridLength* pResult);
        _Check_return_ HRESULT GetInt32PropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ INT* pResult);
        _Check_return_ HRESULT GetMatrix3DPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml_media::Media3D::Matrix3D* pResult);
        _Check_return_ HRESULT GetMatrixPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml_media::Matrix* pResult);
        _Check_return_ HRESULT GetObjectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ IInspectable** ppResult);
        _Check_return_ HRESULT GetPointPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wf::Point* pResult);
        _Check_return_ HRESULT GetRectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wf::Rect* pResult);
        _Check_return_ HRESULT GetSizePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wf::Size* pResult);
        _Check_return_ HRESULT GetStringPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ HSTRING* pResult);
        _Check_return_ HRESULT GetThicknessPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ xaml::Thickness* pResult);
        _Check_return_ HRESULT GetTimeSpanPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ wf::TimeSpan* pResult);
        _Check_return_ HRESULT GetXamlDirectObjectImpl(_In_ IInspectable* pObject, _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT GetXamlDirectObjectFromCollectionAtImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT GetXamlDirectObjectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT InsertIntoCollectionAtImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _In_ xaml::Core::Direct::IXamlDirectObject* pValue);
        _Check_return_ HRESULT RemoveEventHandlerImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler);
        _Check_return_ HRESULT RemoveFromCollectionImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::IXamlDirectObject* pValue, _Out_ BOOLEAN* pResult);
        _Check_return_ HRESULT RemoveFromCollectionAtImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index);
        _Check_return_ HRESULT SetBooleanPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ BOOLEAN value);
        _Check_return_ HRESULT SetColorPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wu::Color value);
        _Check_return_ HRESULT SetCornerRadiusPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml::CornerRadius value);
        _Check_return_ HRESULT SetDateTimePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wf::DateTime value);
        _Check_return_ HRESULT SetDoublePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ DOUBLE value);
        _Check_return_ HRESULT SetDurationPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml::Duration value);
        _Check_return_ HRESULT SetEnumPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ UINT value);
        _Check_return_ HRESULT SetGridLengthPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml::GridLength value);
        _Check_return_ HRESULT SetInt32PropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ INT value);
        _Check_return_ HRESULT SetMatrix3DPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml_media::Media3D::Matrix3D value);
        _Check_return_ HRESULT SetMatrixPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml_media::Matrix value);
        _Check_return_ HRESULT SetObjectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ IInspectable* pValue);
        _Check_return_ HRESULT SetPointPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wf::Point value);
        _Check_return_ HRESULT SetRectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wf::Rect value);
        _Check_return_ HRESULT SetSizePropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wf::Size value);
        _Check_return_ HRESULT SetStringPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ HSTRING value);
        _Check_return_ HRESULT SetThicknessPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ xaml::Thickness value);
        _Check_return_ HRESULT SetTimeSpanPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ wf::TimeSpan value);
        _Check_return_ HRESULT SetXamlDirectObjectPropertyImpl(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ xaml::Core::Direct::IXamlDirectObject* pValue);

    private:
        template<typename T>
        _Check_return_ HRESULT SetValueByIndex(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ T inputValue);

        template<typename T>
        _Check_return_ HRESULT GetValueByIndex(_In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ T* pResult);

        void CheckPropertyIndex(_In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex);
        _Check_return_ HRESULT CheckNeedsPeer(_In_ CDependencyObject *pCDO, _In_ const CDependencyProperty *pDP);
    };
}
