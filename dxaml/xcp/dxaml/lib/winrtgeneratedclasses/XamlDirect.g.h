// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "CornerRadius.g.h"
#include "Duration.g.h"
#include "GridLength.g.h"
#include "Matrix.g.h"
#include "Matrix3D.g.h"
#include "Thickness.g.h"
#include <FeatureFlags.h>
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect) 
#define FEATURE_XAMLDIRECT_OVERRIDE override
#else
#define FEATURE_XAMLDIRECT_OVERRIDE
#endif
#define __XamlDirect_GUID "6a38cf51-578f-4551-b5ef-6b3cf208b35f"

#pragma region forwarders
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect)
namespace ctl
{
    template<typename impl_type>
    class interface_forwarder< ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect, impl_type> final
        : public ctl::iinspectable_forwarder_base< ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect, impl_type>
    {
        impl_type* This() { return this->This_helper<impl_type>(); }
        IFACEMETHOD(AddEventHandler)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler) override { return This()->AddEventHandler(pXamlDirectObject, eventIndex, pHandler); }
        IFACEMETHOD(AddEventHandler_HandledEventsToo)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo) override { return This()->AddEventHandler_HandledEventsToo(pXamlDirectObject, eventIndex, pHandler, handledEventsToo); }
        IFACEMETHOD(AddToCollection)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue) override { return This()->AddToCollection(pXamlDirectObject, pValue); }
        IFACEMETHOD(ClearCollection)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject) override { return This()->ClearCollection(pXamlDirectObject); }
        IFACEMETHOD(ClearProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex) override { return This()->ClearProperty(pXamlDirectObject, propertyIndex); }
        IFACEMETHOD(CreateInstance)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlTypeIndex typeIndex, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult) override { return This()->CreateInstance(typeIndex, ppResult); }
        IFACEMETHOD(GetBooleanProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ BOOLEAN* pResult) override { return This()->GetBooleanProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetCollectionCount)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Out_ UINT* pResult) override { return This()->GetCollectionCount(pXamlDirectObject, pResult); }
        IFACEMETHOD(GetColorProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::UI::Color* pResult) override { return This()->GetColorProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetCornerRadiusProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::CornerRadius* pResult) override { return This()->GetCornerRadiusProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetDateTimeProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::DateTime* pResult) override { return This()->GetDateTimeProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetDoubleProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ DOUBLE* pResult) override { return This()->GetDoubleProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetDurationProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Duration* pResult) override { return This()->GetDurationProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetEnumProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ UINT* pResult) override { return This()->GetEnumProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetGridLengthProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::GridLength* pResult) override { return This()->GetGridLengthProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetInt32Property)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ INT* pResult) override { return This()->GetInt32Property(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetMatrix3DProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Media::Media3D::Matrix3D* pResult) override { return This()->GetMatrix3DProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetMatrixProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Media::Matrix* pResult) override { return This()->GetMatrixProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetObject)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Outptr_ IInspectable** ppResult) override { return This()->GetObject(pXamlDirectObject, ppResult); }
        IFACEMETHOD(GetObjectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ IInspectable** ppResult) override { return This()->GetObjectProperty(pXamlDirectObject, propertyIndex, ppResult); }
        IFACEMETHOD(GetPointProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Point* pResult) override { return This()->GetPointProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetRectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Rect* pResult) override { return This()->GetRectProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetSizeProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Size* pResult) override { return This()->GetSizeProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetStringProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ HSTRING* pResult) override { return This()->GetStringProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetThicknessProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Thickness* pResult) override { return This()->GetThicknessProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetTimeSpanProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::TimeSpan* pResult) override { return This()->GetTimeSpanProperty(pXamlDirectObject, propertyIndex, pResult); }
        IFACEMETHOD(GetXamlDirectObject)(_In_ IInspectable* pObject, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult) override { return This()->GetXamlDirectObject(pObject, ppResult); }
        IFACEMETHOD(GetXamlDirectObjectFromCollectionAt)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult) override { return This()->GetXamlDirectObjectFromCollectionAt(pXamlDirectObject, index, ppResult); }
        IFACEMETHOD(GetXamlDirectObjectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult) override { return This()->GetXamlDirectObjectProperty(pXamlDirectObject, propertyIndex, ppResult); }
        IFACEMETHOD(InsertIntoCollectionAt)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue) override { return This()->InsertIntoCollectionAt(pXamlDirectObject, index, pValue); }
        IFACEMETHOD(RemoveEventHandler)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler) override { return This()->RemoveEventHandler(pXamlDirectObject, eventIndex, pHandler); }
        IFACEMETHOD(RemoveFromCollection)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue, _Out_ BOOLEAN* pResult) override { return This()->RemoveFromCollection(pXamlDirectObject, pValue, pResult); }
        IFACEMETHOD(RemoveFromCollectionAt)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index) override { return This()->RemoveFromCollectionAt(pXamlDirectObject, index); }
        IFACEMETHOD(SetBooleanProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ BOOLEAN value) override { return This()->SetBooleanProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetColorProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::UI::Color value) override { return This()->SetColorProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetCornerRadiusProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::CornerRadius value) override { return This()->SetCornerRadiusProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetDateTimeProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::DateTime value) override { return This()->SetDateTimeProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetDoubleProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ DOUBLE value) override { return This()->SetDoubleProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetDurationProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Duration value) override { return This()->SetDurationProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetEnumProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ UINT value) override { return This()->SetEnumProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetGridLengthProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::GridLength value) override { return This()->SetGridLengthProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetInt32Property)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ INT value) override { return This()->SetInt32Property(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetMatrix3DProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Media::Media3D::Matrix3D value) override { return This()->SetMatrix3DProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetMatrixProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Media::Matrix value) override { return This()->SetMatrixProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetObjectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ IInspectable* pValue) override { return This()->SetObjectProperty(pXamlDirectObject, propertyIndex, pValue); }
        IFACEMETHOD(SetPointProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Point value) override { return This()->SetPointProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetRectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Rect value) override { return This()->SetRectProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetSizeProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Size value) override { return This()->SetSizeProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetStringProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ HSTRING value) override { return This()->SetStringProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetThicknessProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Thickness value) override { return This()->SetThicknessProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetTimeSpanProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::TimeSpan value) override { return This()->SetTimeSpanProperty(pXamlDirectObject, propertyIndex, value); }
        IFACEMETHOD(SetXamlDirectObjectProperty)(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue) override { return This()->SetXamlDirectObjectProperty(pXamlDirectObject, propertyIndex, pValue); }
    };
}
#endif
#pragma endregion

namespace DirectUI
{
    class XamlDirect;

    class __declspec(novtable) XamlDirectGenerated:
        public ctl::WeakReferenceSource
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect)
        , public ctl::forwarder_holder< ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect, XamlDirectGenerated >
#endif
    {
        friend class DirectUI::XamlDirect;

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Core.Direct.XamlDirect");

        BEGIN_INTERFACE_MAP(XamlDirectGenerated, ctl::WeakReferenceSource)
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect)
            INTERFACE_ENTRY(XamlDirectGenerated, ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect)
#else
            INTERFACE_ENTRY(DUMMYINTERFACE, IUnknown)
#endif
        END_INTERFACE_MAP(XamlDirectGenerated, ctl::WeakReferenceSource)

    public:
        XamlDirectGenerated();
        ~XamlDirectGenerated() override;

        // Event source typedefs.


        // Properties.

        // Events.

        // Methods.
        _Check_return_ HRESULT STDMETHODCALLTYPE AddEventHandler(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler);
        _Check_return_ HRESULT STDMETHODCALLTYPE AddEventHandler_HandledEventsToo(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler, _In_ BOOLEAN handledEventsToo);
        _Check_return_ HRESULT STDMETHODCALLTYPE AddToCollection(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE ClearCollection(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject);
        _Check_return_ HRESULT STDMETHODCALLTYPE ClearProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex);
        _Check_return_ HRESULT STDMETHODCALLTYPE CreateInstance(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlTypeIndex typeIndex, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetBooleanProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ BOOLEAN* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetCollectionCount(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Out_ UINT* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetColorProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::UI::Color* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetCornerRadiusProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::CornerRadius* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetDateTimeProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::DateTime* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetDoubleProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ DOUBLE* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetDurationProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Duration* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetEnumProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ UINT* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetGridLengthProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::GridLength* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetInt32Property(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ INT* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetMatrix3DProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Media::Media3D::Matrix3D* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetMatrixProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Media::Matrix* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetObject(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _Outptr_ IInspectable** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetObjectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ IInspectable** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetPointProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Point* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetRectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Rect* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetSizeProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::Size* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetStringProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ HSTRING* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetThicknessProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Microsoft::UI::Xaml::Thickness* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetTimeSpanProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Out_ ABI::Windows::Foundation::TimeSpan* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetXamlDirectObject(_In_ IInspectable* pObject, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetXamlDirectObjectFromCollectionAt(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE GetXamlDirectObjectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject** ppResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE InsertIntoCollectionAt(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE RemoveEventHandler(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlEventIndex eventIndex, _In_ IInspectable* pHandler);
        _Check_return_ HRESULT STDMETHODCALLTYPE RemoveFromCollection(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue, _Out_ BOOLEAN* pResult);
        _Check_return_ HRESULT STDMETHODCALLTYPE RemoveFromCollectionAt(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ UINT index);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetBooleanProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ BOOLEAN value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetColorProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::UI::Color value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetCornerRadiusProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::CornerRadius value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetDateTimeProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::DateTime value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetDoubleProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ DOUBLE value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetDurationProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Duration value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetEnumProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ UINT value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetGridLengthProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::GridLength value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetInt32Property(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ INT value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetMatrix3DProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Media::Media3D::Matrix3D value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetMatrixProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Media::Matrix value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetObjectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ IInspectable* pValue);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetPointProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Point value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetRectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Rect value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetSizeProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::Size value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetStringProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ HSTRING value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetThicknessProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Microsoft::UI::Xaml::Thickness value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetTimeSpanProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_ ABI::Windows::Foundation::TimeSpan value);
        _Check_return_ HRESULT STDMETHODCALLTYPE SetXamlDirectObjectProperty(_In_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject, _In_ ABI::Microsoft::UI::Xaml::Core::Direct::XamlPropertyIndex propertyIndex, _In_opt_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectObject* pValue);


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:

        // Fields.
    };
}

#include "XamlDirect_Partial.h"

namespace DirectUI
{
    // Note that the ordering of the base types here is important - the base factory comes first, followed by all the
    // interfaces specific to this type.  By doing this, we allow every Factory's CreateInstance method to be more
    // COMDAT-folding-friendly.  Because this ensures that the first vfptr contains GetTypeIndex, it means that all
    // CreateInstance functions with the same base factory generate the same assembly instructions and thus will
    // fold together.  This is significant for binary size in Microsoft.UI.Xaml.dll so change this only with great
    // care.
    class __declspec(novtable) XamlDirectFactory:
       public ctl::AbstractActivationFactory
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect)
        , public ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectStatics
#endif
    {
        BEGIN_INTERFACE_MAP(XamlDirectFactory, ctl::AbstractActivationFactory)
#if WI_IS_FEATURE_PRESENT(Feature_XamlDirect)
            INTERFACE_ENTRY(XamlDirectFactory, ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirectStatics)
#else
            INTERFACE_ENTRY(DUMMYINTERFACE, IUnknown)
#endif
        END_INTERFACE_MAP(XamlDirectFactory, ctl::AbstractActivationFactory)

    public:
        // Factory methods.

        // Static properties.

        // Dependency properties.

        // Attached properties.

        // Static methods.
        IFACEMETHOD(GetDefault)(_Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect** ppResult) FEATURE_XAMLDIRECT_OVERRIDE;

        // Static events.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;



    private:

        // Customized static properties.

        // Customized static  methods.
         _Check_return_ HRESULT GetDefaultImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Core::Direct::IXamlDirect** ppResult); 
    };
}