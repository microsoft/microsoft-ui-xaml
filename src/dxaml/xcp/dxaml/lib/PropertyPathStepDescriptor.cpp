// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPathStepDescriptor.h"
#include "SourceAccessPathStep.h"
#include "PropertyAccessPathStep.h"
#include "StringIndexerPathStep.h"
#include "IntIndexerPathStep.h"

using namespace DirectUI;
using namespace xaml_data;

PropertyPathStepDescriptor::PropertyPathStepDescriptor() noexcept
    : m_kind(PropertyPathStepDescriptorKind::None)
    , m_szText(nullptr)
{
}

PropertyPathStepDescriptor::~PropertyPathStepDescriptor() noexcept
{
    Reset();
}

PropertyPathStepDescriptor::PropertyPathStepDescriptor(PropertyPathStepDescriptor&& other) noexcept
    : m_kind(PropertyPathStepDescriptorKind::None)
    , m_szText(nullptr)
{
    MoveFrom(other);
}

PropertyPathStepDescriptor& PropertyPathStepDescriptor::operator=(PropertyPathStepDescriptor&& other) noexcept
{
    if (this != &other)
    {
        Reset();
        MoveFrom(other);
    }
    return *this;
}

void PropertyPathStepDescriptor::Reset() noexcept
{
    switch (m_kind)
    {
    case PropertyPathStepDescriptorKind::PropertyAccess:
    case PropertyPathStepDescriptorKind::StringIndexer:
        delete[] m_szText;
        break;
    default:
        break;
    }

    m_kind = PropertyPathStepDescriptorKind::None;
    m_szText = nullptr;
}

void PropertyPathStepDescriptor::MoveFrom(_Inout_ PropertyPathStepDescriptor& other) noexcept
{
    m_kind = other.m_kind;

    switch (m_kind)
    {
    case PropertyPathStepDescriptorKind::PropertyAccess:
    case PropertyPathStepDescriptorKind::StringIndexer:
        m_szText = other.m_szText;
        other.m_szText = nullptr;
        break;
    case PropertyPathStepDescriptorKind::IntIndexer:
        m_nIndex = other.m_nIndex;
        break;
    case PropertyPathStepDescriptorKind::DependencyProperty:
        m_pDP = other.m_pDP;
        break;
    default:
        m_szText = nullptr;
        break;
    }

    other.m_kind = PropertyPathStepDescriptorKind::None;
}

PropertyPathStepDescriptor PropertyPathStepDescriptor::CreateSourceAccess() noexcept
{
    PropertyPathStepDescriptor d;
    d.m_kind = PropertyPathStepDescriptorKind::SourceAccess;
    return d;
}

PropertyPathStepDescriptor PropertyPathStepDescriptor::CreatePropertyAccess(_In_z_ WCHAR* szName) noexcept
{
    PropertyPathStepDescriptor d;
    d.m_kind = PropertyPathStepDescriptorKind::PropertyAccess;
    d.m_szText = szName;
    return d;
}

PropertyPathStepDescriptor PropertyPathStepDescriptor::CreateIntIndexer(XUINT32 nIndex) noexcept
{
    PropertyPathStepDescriptor d;
    d.m_kind = PropertyPathStepDescriptorKind::IntIndexer;
    d.m_nIndex = nIndex;
    return d;
}

PropertyPathStepDescriptor PropertyPathStepDescriptor::CreateStringIndexer(_In_z_ WCHAR* szIndex) noexcept
{
    PropertyPathStepDescriptor d;
    d.m_kind = PropertyPathStepDescriptorKind::StringIndexer;
    d.m_szText = szIndex;
    return d;
}

PropertyPathStepDescriptor PropertyPathStepDescriptor::CreateDependencyProperty(_In_ const CDependencyProperty* pDP) noexcept
{
    PropertyPathStepDescriptor d;
    d.m_kind = PropertyPathStepDescriptorKind::DependencyProperty;
    d.m_pDP = pDP;
    return d;
}

_Check_return_
HRESULT
PropertyPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener* pListener,
    bool fListenToChanges,
    _Outptr_ PropertyPathStep** ppStep) const
{
    HRESULT hr = S_OK;
    WCHAR* szTempCopy = nullptr;

    switch (m_kind)
    {
    case PropertyPathStepDescriptorKind::SourceAccess:
    {
        ctl::ComPtr<SourceAccessPathStep> spStep;
        IFC(ctl::make(pListener, &spStep));
        *ppStep = spStep.Detach();
        break;
    }

    case PropertyPathStepDescriptorKind::PropertyAccess:
    {
        ctl::ComPtr<PropertyAccessPathStep> spStep;
        size_t nNameSize = wcslen(m_szText) + 1;

        szTempCopy = new WCHAR[nNameSize];
        IFCEXPECT(wcscpy_s(szTempCopy, nNameSize, m_szText) == 0);

        IFC(ctl::make<PropertyAccessPathStep>(pListener, szTempCopy, fListenToChanges, &spStep));

        szTempCopy = nullptr; // Transferred to the step
        *ppStep = spStep.Detach();
        break;
    }

    case PropertyPathStepDescriptorKind::IntIndexer:
    {
        ctl::ComPtr<IntIndexerPathStep> spStep;
        IFC(ctl::make<IntIndexerPathStep>(pListener, m_nIndex, fListenToChanges, &spStep));
        *ppStep = spStep.Detach();
        break;
    }

    case PropertyPathStepDescriptorKind::StringIndexer:
    {
        size_t nIndexSize = wcslen(m_szText) + 1;
        ctl::ComPtr<StringIndexerPathStep> spStep;

        szTempCopy = new WCHAR[nIndexSize];
        IFCEXPECT(wcscpy_s(szTempCopy, nIndexSize, m_szText) == 0);

        IFC(ctl::make<StringIndexerPathStep>(pListener, szTempCopy, fListenToChanges, &spStep));
        szTempCopy = nullptr; // Transferred to the step
        *ppStep = spStep.Detach();
        break;
    }

    case PropertyPathStepDescriptorKind::DependencyProperty:
    {
        ctl::ComPtr<PropertyAccessPathStep> spStep;
        IFC(ctl::make<PropertyAccessPathStep>(pListener, m_pDP, fListenToChanges, &spStep));
        *ppStep = spStep.Detach();
        break;
    }

    case PropertyPathStepDescriptorKind::None:
    default:
        IFC(E_INVALIDARG);
        break;
    }

Cleanup:
    delete[] szTempCopy;
    RRETURN(hr);
}

