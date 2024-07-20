// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <TypeTableStructs.h>
#include <MetadataAPI.h>
#include <MockDependencyProperty.h>

using namespace DirectUI;
using namespace Microsoft::UI::Xaml::Tests::Metadata;

MockDependencyProperty::MockDependencyProperty(_In_ KnownPropertyIndex index) : MockDependencyProperty()
{
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(index);
    m_nIndex = dp->m_nIndex;
    m_nPropertyTypeIndex = dp->m_nPropertyTypeIndex;
    m_nDeclaringTypeIndex = dp->m_nDeclaringTypeIndex;
    m_nTargetTypeIndex = dp->m_nTargetTypeIndex;
    m_flags = dp->m_flags;
}

void MockDependencyProperty::SetIndex(KnownPropertyIndex index)
{
    m_nIndex = index;
}

bool MockDependencyProperty::IsMock(_In_ const CDependencyProperty* dp)
{
    return (dp != MetadataAPI::GetDependencyPropertyByIndex(dp->GetIndex()));
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultInheritedPropertyValue(
    _In_ CCoreServices* core,
    _Out_ CValue* defaultValue) const
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultValueFromPeer(
    _In_opt_ CDependencyObject* referenceObject,
    _Out_ CValue* defaultValue) const
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::ValidateType(
    _In_ const CClassInfo* type) const
{
    return S_OK;
}

_Check_return_ HRESULT CDependencyProperty::CreateDefaultValueObject(
    _In_ CCoreServices* core,
    _Out_ CValue* defaultValue) const
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::CreateDefaultVO(
    _In_ CCoreServices* core,
    _Out_ CValue* defaultValue) const
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetBooleanThemeResourceValue(
    _In_ CCoreServices* core,
    _In_ const xstring_ptr_view& key,
    _Out_ bool* value,
    _Out_opt_ bool* resourceExists /* = nullptr */)
{
    *value = false;
    if (resourceExists)
    {
        *resourceExists = false;
    }
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultFocusVisualBrush(
    _In_ FocusVisualType forFocusVisualType,
    _In_ CCoreServices* core,
    _In_opt_ CDependencyObject* targetObject,
    _Outptr_ CDependencyObject** ppBrush)
{
    *ppBrush = nullptr;
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultFontIconFontFamily(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** fontFamily)
{
    *fontFamily = nullptr;
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultTextControlContextFlyout(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** ppFlyout)
{
    *ppFlyout = nullptr;
    return E_NOTIMPL;
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultTextControlSelectionFlyout(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** ppFlyout)
{
    *ppFlyout = nullptr;
    return E_NOTIMPL;
}

UINT16 CDependencyProperty::GetOffset() const
{
    if (IsPropMethodCall())
    {
        return 0;
    }

    if (MockDependencyProperty::IsMock(this))
    {
        return static_cast<const MockDependencyProperty*>(this)->offset;
    }
    return 0;
}

CREATEGROUPPFN CDependencyProperty::GetGroupCreator() const
{
    if (MockDependencyProperty::IsMock(this))
    {
        return static_cast<const MockDependencyProperty*>(this)->groupCreator;
    }
    return nullptr;
}

UINT16 CDependencyProperty::GetGroupOffset() const
{
    if (MockDependencyProperty::IsMock(this))
    {
        return static_cast<const MockDependencyProperty*>(this)->groupOffset;
    }
    return 0;
}

METHODPFN CDependencyProperty::GetPropertyMethod() const
{
    if (MockDependencyProperty::IsMock(this))
    {
        return static_cast<const MockDependencyProperty*>(this)->propertyMethod;
    }
    return nullptr;
}

RENDERCHANGEDPFN CDependencyProperty::GetRenderChangedHandler() const
{
    if (MockDependencyProperty::IsMock(this))
    {
        return static_cast<const MockDependencyProperty*>(this)->renderChangedHandler;
    }
    return nullptr;
}