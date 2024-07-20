// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPath.h"
#include "PropertyPathParser.h"
#include "PropertyPathStep.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_data;

void
PropertyPathListener::AppendStep(_In_ PropertyPathStep* const pStep)
{
    if (!m_tpFirst)
    {
        SetPtrValue(m_tpFirst, pStep);
        SetPtrValue(m_tpLast, pStep);
    }
    else
    {
        m_tpLast->SetNext(pStep);
        SetPtrValue(m_tpLast, pStep);
    }
}

_Check_return_ 
HRESULT 
PropertyPathListener::Initialize(
    _In_ IPropertyPathListenerHost *pOwner, 
    _In_ PropertyPathParser *pPropertyPathParser, 
    _In_ bool fListenToChanges, 
    _In_ bool fUseWeakReferenceForSource)
{
    HRESULT hr = S_OK;
    auto itrDescriptor = pPropertyPathParser->Descriptors().begin();
    ctl::ComPtr<PropertyPathStep> spStep;

    m_pOwner = pOwner;

    for (; itrDescriptor != pPropertyPathParser->Descriptors().end(); itrDescriptor++)
    {
        IFC((*itrDescriptor)->CreateStep(this, fListenToChanges, spStep.ReleaseAndGetAddressOf()));
        AppendStep(spStep.Get());
    }

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathListener::SetSource(_In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;

    // Connect to the source
    IFC(ConnectPathStep(m_tpFirst.Get(), pSource ));

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathListener::ConnectPathStep(_In_ PropertyPathStep *pStep, _In_ IInspectable *pSource)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spCurrentObject;
    ctl::ComPtr<PropertyPathStep> spCurrentStep = pStep;
    ctl::ComPtr<IInspectable> spValue;

    spCurrentObject = pSource;

    while (spCurrentStep)
    {
        // Connext the current step to the source, and start going down from there
        IFC(spCurrentStep->ReConnect(spCurrentObject.Get()));

        // Only get the value if there's a next step 
        // that is going to be connected to it
        if (spCurrentStep->GetNextStep())
        {
            // Now we get the next value in the chain
            IFC(spCurrentStep->GetValue(spValue.ReleaseAndGetAddressOf()));

            // Get the value for the next step, we do not care what it is
            // the path will deal with it
            spCurrentObject = spValue;
        }

        // Move to the next path
        spCurrentStep = spCurrentStep->GetNextStep();
    }    

Cleanup:

    RRETURN(hr);
}


_Check_return_ 
HRESULT 
PropertyPathListener::GetValue(_Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_tpLast);

    IFC(m_tpLast->GetValue(ppValue));

Cleanup: 

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathListener::SetValue(_In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_tpLast);

    IFC(m_tpLast->SetValue(pValue));

Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathListener::GetLeafType(_Outptr_ const CClassInfo **ppLeafType)
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_tpLast);

    IFC(m_tpLast->GetType(ppLeafType));

Cleanup:

    RRETURN(hr);
}


_Check_return_ 
HRESULT 
PropertyPathListener::GetTraceString(_Outptr_ const WCHAR **pszTraceString)
{
    HRESULT hr = S_OK;

    if (m_pOwner)
    {
        IFC(m_pOwner->GetTraceString(pszTraceString));
    }
    else 
    {
        // If there's no owner no trace string will be generated
        *pszTraceString = L"";
    }
    
Cleanup:

    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathListener::PropertyPathStepChanged(_In_ PropertyPathStep *pStep)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spValue;

    // We only need to reconnect if the step that changed is not
    // the last step. If it is the last step then we will just
    // get its value later on.
    if (m_tpLast.Get() != pStep)
    {
        // Get the current value of the step that changed
        IFC(pStep->GetValue(spValue.ReleaseAndGetAddressOf()));

        // Connect the step starting with the next step to the one that changed
        IFC(ConnectPathStep(pStep->GetNextStep(), spValue.Get()));
    }

    // Notify the expression that the source has changed
    IFC(m_pOwner->SourceChanged());

Cleanup:    

    RRETURN(hr);
}

// This may be called when the owner is about to be destroyed, giving a chance to
// clear out m_pOwner to make sure there aren't any further attempts to use it.
void PropertyPathListener::ClearOwner()
{ 
    m_pOwner = NULL;
}

bool PropertyPathListener::FullPathExists() const
{
    return m_tpLast && m_tpLast->IsConnected();
}

PropertyPathStep* PropertyPathListener::DebugGetFirstStep()
{
    return m_tpFirst.Get();
}
