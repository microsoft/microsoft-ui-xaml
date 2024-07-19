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

_Check_return_ 
HRESULT 
SourceAccessPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener *pListener, 
    bool fListenToChanges, 
    _Outptr_ PropertyPathStep **ppStep)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<SourceAccessPathStep> spStep;

    IFC(ctl::make(pListener, &spStep));    

    *ppStep = spStep.Detach();
    
Cleanup:

    RRETURN(hr);
}

PropertyAccessPathStepDescriptor::PropertyAccessPathStepDescriptor(_In_z_ WCHAR *szName):
    m_szName(szName)
{ }

PropertyAccessPathStepDescriptor::~PropertyAccessPathStepDescriptor()
{
    delete[] m_szName;
}

_Check_return_ 
HRESULT 
PropertyAccessPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener *pListener, 
    bool fListenToChanges, 
    _Outptr_ PropertyPathStep **ppStep)
{
    HRESULT hr = S_OK;
    WCHAR *szNameCopy = NULL;
    ctl::ComPtr<PropertyAccessPathStep> spStep;
    size_t nNameSize = wcslen(m_szName) + 1;

    szNameCopy = new WCHAR[nNameSize];

    IFCEXPECT(wcscpy_s(szNameCopy, nNameSize, m_szName) == 0);

    IFC(ctl::make<PropertyAccessPathStep>(pListener, szNameCopy, fListenToChanges, &spStep));
    
    szNameCopy = NULL;  // Transferred he copy to the step
    *ppStep = spStep.Detach();
    
Cleanup:

    delete[] szNameCopy;

    RRETURN(hr);
}
    
IntIndexerPathStepDescriptor::IntIndexerPathStepDescriptor(XUINT32 nIndex):
    m_nIndex(nIndex)
{ }

_Check_return_ 
HRESULT 
IntIndexerPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener *pListener, 
    bool fListenToChanges, 
    _Outptr_ PropertyPathStep **ppStep)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IntIndexerPathStep> spStep;

    IFC(ctl::make<IntIndexerPathStep>(pListener, m_nIndex, fListenToChanges, &spStep));
    
    *ppStep = spStep.Detach();

Cleanup:

    RRETURN(hr);
}

StringIndexerPathStepDescriptor::StringIndexerPathStepDescriptor(_In_z_ WCHAR *szIndex):
    m_szIndex(szIndex)
{ }

StringIndexerPathStepDescriptor::~StringIndexerPathStepDescriptor()
{
    delete[] m_szIndex;
}

_Check_return_ 
HRESULT 
StringIndexerPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener *pListener, 
    bool fListenToChanges, 
    _Outptr_ PropertyPathStep **ppStep)
{
    HRESULT hr = S_OK;
    WCHAR *szIndexCopy = NULL;
    size_t nIndexSize = wcslen(m_szIndex) + 1;
    ctl::ComPtr<StringIndexerPathStep> spStep;

    szIndexCopy = new WCHAR[nIndexSize];

    IFCEXPECT(wcscpy_s(szIndexCopy, nIndexSize, m_szIndex) == 0);

    IFC(ctl::make<StringIndexerPathStep>(pListener, szIndexCopy, fListenToChanges, &spStep));
    szIndexCopy = NULL; // Transferred to the path step 

    *ppStep = spStep.Detach();

Cleanup:

    delete[] szIndexCopy;

    RRETURN(hr);
}

DependencyPropertyPathStepDescriptor::DependencyPropertyPathStepDescriptor(_In_ const CDependencyProperty *pDP)
{ 
    m_pDP = pDP;
}

DependencyPropertyPathStepDescriptor::~DependencyPropertyPathStepDescriptor()
{
}

_Check_return_ 
HRESULT 
DependencyPropertyPathStepDescriptor::CreateStep(
    _In_ PropertyPathListener *pListener, 
    bool fListenToChanges, 
    _Outptr_ PropertyPathStep **ppStep)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PropertyAccessPathStep> spStep;

    IFC(ctl::make<PropertyAccessPathStep>(pListener, m_pDP, fListenToChanges, &spStep));

    *ppStep = spStep.Detach();

Cleanup:

    RRETURN(hr);
}

