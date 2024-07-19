// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlDirect.g.h"
#include "XbfMetadataApi.h"
#include "XamlDirect_Partial.h"
#include <MetadataAPI.h>
#include <ValueType.h>
#include "UIElement.g.h"

using namespace DirectUI;

XamlDirect::XamlDirect()
{
}

XamlDirect::~XamlDirect()
{
}

_Check_return_ HRESULT XamlDirectFactory::GetDefaultImpl(
    _Outptr_ xaml::Core::Direct::IXamlDirect** ppResult)
{
    *ppResult = nullptr;
    XamlDirect* pDefaultInstance = DXamlCore::GetCurrent()->GetXamlDirectDefaultInstanceNoRef();
    IFC_RETURN(ctl::do_query_interface(*ppResult, pDefaultInstance));

    return S_OK;
}

#pragma region CreateInstance

_Check_return_ HRESULT XamlDirect::CreateInstanceImpl(
    _In_ xaml::Core::Direct::XamlTypeIndex typeIndex,
    _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult)
{
    *ppResult = nullptr;
    auto localIndex = static_cast<std::underlying_type<Parser::StableXbfTypeIndex>::type>(typeIndex);
    if (localIndex <= 0 || localIndex >= Parser::StableXbfTypeCount)
    {
        IFCFAILFAST(E_INVALIDARG);
    }

    const CClassInfo* ppType = MetadataAPI::GetClassInfoByIndex(GetKnownTypeIndex(static_cast<Parser::StableXbfTypeIndex>(typeIndex)));

    CDependencyObject* pCDO = nullptr;
    IFC_RETURN(ActivationAPI::ActivateCoreInstance(ppType, &pCDO));

    *ppResult = ctl::interface_cast<xaml::Core::Direct::IXamlDirectObject>(pCDO);

    return S_OK;
}

#pragma endregion

#pragma region GetObject

_Check_return_ HRESULT XamlDirect::GetObjectImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _Outptr_ IInspectable** ppResult)
{
    *ppResult = nullptr;
    HRESULT hr = S_OK;
    DependencyObject* pPeer = nullptr;

    IFC(DXamlCore::GetCurrent()->GetPeer(ctl::impl_cast<CDependencyObject>(pXamlDirectObject), &pPeer));
    IFC(ctl::do_query_interface(*ppResult, pPeer));

Cleanup:
    ctl::release_interface(pPeer);

    return hr;
}

#pragma endregion

#pragma region GetXamlDirectObject

_Check_return_ HRESULT XamlDirect::GetXamlDirectObjectImpl(
    _In_ IInspectable* pObject,
    _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult)
{
    *ppResult = nullptr;
    DependencyObject* pDO = ctl::query_interface_cast<DependencyObject>(pObject).Get();

    CDependencyObject* pCDO = pDO->GetHandleAddRef();

    *ppResult = ctl::interface_cast<xaml::Core::Direct::IXamlDirectObject>(pCDO);

    return S_OK;
}

#pragma endregion

#pragma region SetValue

template<typename T>
_Check_return_ HRESULT XamlDirect::SetValueByIndex(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ T inputValue)
{
    CheckPropertyIndex(propertyIndex);

    CValue value;
    BoxerBuffer buffer;
    IFC_RETURN(CValueBoxer::BoxValue(&value, inputValue, &buffer));

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);
    IFC_RETURN(pCDO->SetValueByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)), value));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::SetObjectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_opt_ IInspectable* pValue)
{
    CheckPropertyIndex(propertyIndex);

    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)));

    CValue value;
    BoxerBuffer buffer;
    DependencyObject* pMOR;
    IFC_RETURN(CValueBoxer::BoxObjectValue(&value, pDP->GetPropertyType(), pValue, &buffer, &pMOR));

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);
    IFC_RETURN(CheckNeedsPeer(pCDO, pDP));
    IFC_RETURN(pCDO->SetValue(pDP, value));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::SetXamlDirectObjectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_opt_ xaml::Core::Direct::IXamlDirectObject* pValue)
{
    CheckPropertyIndex(propertyIndex);

    CValue value;
    if (pValue != nullptr)
    {
        value.SetObjectAddRef(ctl::impl_cast<CDependencyObject>(pValue));
    }
    else
    {
        value.SetNull();
    }

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);
    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)));
    IFC_RETURN(CheckNeedsPeer(pCDO, pDP));
    IFC_RETURN(pCDO->SetValue(pDP, value));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::SetBooleanPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ BOOLEAN boolValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, boolValue);
}

_Check_return_ HRESULT XamlDirect::SetDoublePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ DOUBLE doubleValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, doubleValue);
}

_Check_return_ HRESULT XamlDirect::SetInt32PropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ INT intValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, intValue);
}

_Check_return_ HRESULT XamlDirect::SetStringPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_opt_ HSTRING stringValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, stringValue);
}

_Check_return_ HRESULT XamlDirect::SetDateTimePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wf::DateTime dateTimeValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, dateTimeValue);
}

_Check_return_ HRESULT XamlDirect::SetPointPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wf::Point pointValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, pointValue);
}

_Check_return_ HRESULT XamlDirect::SetRectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wf::Rect rectValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, rectValue);
}

_Check_return_ HRESULT XamlDirect::SetSizePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wf::Size sizeValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, sizeValue);
}

_Check_return_ HRESULT XamlDirect::SetTimeSpanPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wf::TimeSpan timeSpanValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, timeSpanValue);
}

_Check_return_ HRESULT XamlDirect::SetColorPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ wu::Color colorValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, colorValue);
}

_Check_return_ HRESULT XamlDirect::SetCornerRadiusPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml::CornerRadius cornerRadiusValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, cornerRadiusValue);
}

_Check_return_ HRESULT XamlDirect::SetDurationPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml::Duration durationValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, durationValue);
}

_Check_return_ HRESULT XamlDirect::SetGridLengthPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml::GridLength gridLengthValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, gridLengthValue);
}

_Check_return_ HRESULT XamlDirect::SetThicknessPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml::Thickness thicknessValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, thicknessValue);
}

_Check_return_ HRESULT XamlDirect::SetMatrixPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml_media::Matrix matrixValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, matrixValue);
}

_Check_return_ HRESULT XamlDirect::SetMatrix3DPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ xaml_media::Media3D::Matrix3D matrix3dValue)
{
    return SetValueByIndex(pXamlDirectObject, propertyIndex, matrix3dValue);
}

_Check_return_ HRESULT XamlDirect::SetEnumPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _In_ UINT uintValue)
{
    CheckPropertyIndex(propertyIndex);

    CValue value;
    IFC_RETURN(CValueBoxer::BoxEnumValue(&value, uintValue));

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);
    IFC_RETURN(pCDO->SetValueByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)), value));

    return S_OK;
}

#pragma endregion

#pragma region GetValue

template<typename T>
_Check_return_ HRESULT XamlDirect::GetValueByIndex(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ T* pResult)
{
    CheckPropertyIndex(propertyIndex);

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CValue value;
    IFC_RETURN(pCDO->GetValueByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)), &value));
    IFC_RETURN(CValueBoxer::UnboxValue(&value, pResult));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::GetObjectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ IInspectable** pResult)
{
    *pResult = nullptr;
    CheckPropertyIndex(propertyIndex);

    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)));
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CValue value;
    IFC_RETURN(CheckNeedsPeer(pCDO, pDP));
    IFC_RETURN(pCDO->GetValue(pDP, &value));
    IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, pDP->GetPropertyType(), pResult));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::GetXamlDirectObjectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml::Core::Direct::IXamlDirectObject** pResult)
{
    *pResult = nullptr;
    CheckPropertyIndex(propertyIndex);

    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)));
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CValue value;
    IFC_RETURN(pCDO->GetValue(pDP, &value));

    if (!value.IsNull())
    {
        CDependencyObject* pValueCDO = value.AsObject();
        pValueCDO->AddRef();
        *pResult = ctl::interface_cast<xaml::Core::Direct::IXamlDirectObject>(pValueCDO);
    }

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::GetBooleanPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ BOOLEAN* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetDoublePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ DOUBLE* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetInt32PropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ INT* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetStringPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ HSTRING* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetDateTimePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wf::DateTime* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetPointPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wf::Point* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetRectPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wf::Rect* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetSizePropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wf::Size* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetTimeSpanPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wf::TimeSpan* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetColorPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ wu::Color* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetCornerRadiusPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml::CornerRadius* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetDurationPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml::Duration* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetGridLengthPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml::GridLength* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetThicknessPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml::Thickness* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetMatrixPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml_media::Matrix* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetMatrix3DPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ xaml_media::Media3D::Matrix3D* pResult)
{
    return GetValueByIndex(pXamlDirectObject, propertyIndex, pResult);
}

_Check_return_ HRESULT XamlDirect::GetEnumPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex,
    _Out_ UINT* pResult)
{
    CheckPropertyIndex(propertyIndex);

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CValue value;
    IFC_RETURN(pCDO->GetValueByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)), &value));
    IFC_RETURN(CValueBoxer::UnboxEnumValue(&value, NULL, pResult));

    return S_OK;
}

#pragma endregion

#pragma region ClearValue

_Check_return_ HRESULT XamlDirect::ClearPropertyImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex)
{
    CheckPropertyIndex(propertyIndex);

    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);
    const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(GetKnownPropertyIndex(static_cast<Parser::StableXbfPropertyIndex>(propertyIndex)));
    IFC_RETURN(CheckNeedsPeer(pCDO, pDP));
    IFC_RETURN(pCDO->ClearValue(pDP));

    return S_OK;
}

#pragma endregion

#pragma region Collections

_Check_return_ HRESULT XamlDirect::GetCollectionCountImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _Out_ UINT* pResult)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    *pResult = static_cast<UINT>(pCollection->GetCount());

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::GetXamlDirectObjectFromCollectionAtImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ UINT index,
    _Outptr_ xaml::Core::Direct::IXamlDirectObject** ppResult)
{
    *ppResult = nullptr;
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    CDependencyObject* pValueCDO = static_cast<CDependencyObject*>(pCollection->GetItemWithAddRef(index));
    *ppResult = ctl::interface_cast<xaml::Core::Direct::IXamlDirectObject>(pValueCDO);

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::AddToCollectionImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::IXamlDirectObject* pValue)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    CValue value;
    value.Wrap<valueObject>(ctl::impl_cast<CDependencyObject>(pValue));

    IFC_RETURN(pCollection->Append(value));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::InsertIntoCollectionAtImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ UINT index,
    _In_ xaml::Core::Direct::IXamlDirectObject* pValue)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    CValue value;
    value.Wrap<valueObject>(ctl::impl_cast<CDependencyObject>(pValue));

    IFC_RETURN(pCollection->Insert(static_cast<XUINT32>(index), value));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::RemoveFromCollectionImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::IXamlDirectObject* pValue,
    _Out_ BOOLEAN* pResult)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    CValue value;
    value.SetObjectAddRef(ctl::impl_cast<CDependencyObject>(pValue));

    XINT32 index = -1;
    IFC_RETURN(pCollection->IndexOf(value, &index));
    if (*pResult = (index >= 0))
    {
        xref_ptr<CDependencyObject> pRemovedCDO;
        pRemovedCDO.attach(static_cast<CDependencyObject*>(pCollection->RemoveAt(static_cast<XUINT32>(index))));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::RemoveFromCollectionAtImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ UINT index)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);

    xref_ptr<CDependencyObject> pRemovedCDO;
    pRemovedCDO.attach(static_cast<CDependencyObject*>(pCollection->RemoveAt(static_cast<XUINT32>(index))));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::ClearCollectionImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject)
{
    CDependencyObject* pCDO = ctl::impl_cast<CDependencyObject>(pXamlDirectObject);

    CCollection* pCollection = static_cast<CCollection*>(pCDO);
    IFC_RETURN(pCollection->Clear());

    return S_OK;
}

#pragma endregion

#pragma region Events

_Check_return_ HRESULT XamlDirect::AddEventHandlerImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlEventIndex eventIndex,
    _In_ IInspectable* pHandler)
{
    return AddEventHandler_HandledEventsTooImpl(pXamlDirectObject, eventIndex, pHandler, false);
}

_Check_return_ HRESULT XamlDirect::AddEventHandler_HandledEventsTooImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlEventIndex eventIndex,
    _In_ IInspectable* pHandler,
    _In_ BOOLEAN handledEventsToo)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(ctl::impl_cast<CDependencyObject>(pXamlDirectObject), &peer));

    UIElement* pUIElement = static_cast<UIElement*>(peer.Get());
    IFC_RETURN(pUIElement->EventAddHandlerByIndex(GetKnownEventIndex(static_cast<Parser::StableEventIndex>(eventIndex)), pHandler, handledEventsToo));

    return S_OK;
}

_Check_return_ HRESULT XamlDirect::RemoveEventHandlerImpl(
    _In_ xaml::Core::Direct::IXamlDirectObject* pXamlDirectObject,
    _In_ xaml::Core::Direct::XamlEventIndex eventIndex,
    _In_ IInspectable* pHandler)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(ctl::impl_cast<CDependencyObject>(pXamlDirectObject), &peer));

    UIElement* pUIElement = static_cast<UIElement*>(peer.Get());
    IFC_RETURN(pUIElement->EventRemoveHandlerByIndex(GetKnownEventIndex(static_cast<Parser::StableEventIndex>(eventIndex)), pHandler));

    return S_OK;
}

#pragma endregion

#pragma region Helpers

void XamlDirect::CheckPropertyIndex(_In_ xaml::Core::Direct::XamlPropertyIndex propertyIndex)
{
    auto typedIndex = static_cast<std::underlying_type<Parser::StableXbfPropertyIndex>::type>(propertyIndex);
    if (typedIndex <= 0 || typedIndex >= Parser::StableXbfPropertyCount)
    {
        IFCFAILFAST(E_INVALIDARG);
    }
}

_Check_return_ HRESULT XamlDirect::CheckNeedsPeer(
    _In_ CDependencyObject *pCDO,
    _In_ const CDependencyProperty *pDP)
{
    if (pCDO->ParticipatesInManagedTree() && pDP->IsSparse())
    {
        IFC_RETURN(pCDO->EnsurePeerDuringCreate());
    }

    return S_OK;
}

#pragma endregion
