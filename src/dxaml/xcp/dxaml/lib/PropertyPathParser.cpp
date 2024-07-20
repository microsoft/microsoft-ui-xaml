// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPathParser.h"

using namespace DirectUI;
using namespace xaml_data;

PropertyPathParser::PropertyPathParser()
{ }

PropertyPathParser::~PropertyPathParser()
{
    std::for_each(m_descriptors.begin(), m_descriptors.end(),
        [](PropertyPathStepDescriptor *pDescriptor)
        {
            delete pDescriptor;
        });
}

_Check_return_ 
HRESULT 
PropertyPathParser::SetSource(_In_opt_z_ const WCHAR *szPath, _In_opt_ ::XamlServiceProviderContext* context)
{
    // The source can only be called once
    IFCEXPECT_RETURN(m_descriptors.empty());
    IFC_RETURN(Parse(szPath, context));

    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyPathParser::Parse(_In_opt_z_ const WCHAR *szPropertyPath, _In_opt_ ::XamlServiceProviderContext* context)
{
    HRESULT hr = S_OK;
    const WCHAR *pPropertyPath = nullptr;
    const WCHAR *pCurrentProperty = nullptr;
    WCHAR *szCurrentProperty = nullptr;
    WCHAR *szIndex = nullptr;
    PropertyPathStepDescriptor *pCurrentStep = nullptr;
    bool fExpectingProperty = false;

    // If the property path is empty or NULL then this means that we're binding
    // directly to the source
    if (szPropertyPath == NULL || szPropertyPath[0] == L'\0')
    {
        pCurrentStep = new SourceAccessPathStepDescriptor();

        // This will be the only step in the chain
        IFC(AppendStepDescriptor(pCurrentStep));

        pCurrentStep = NULL;
        goto Cleanup;
    }

    // This "parser" will go through the characters collecting the different types
    // of path steps supported
    pPropertyPath = szPropertyPath;
    pCurrentProperty = pPropertyPath;

    while (TRUE)
    {
        // We found a typed property, (Class.Property) and thus we will have to 
        // collect all of the property name
        if (*pPropertyPath == L'(')
        {
            // Collect all of the property name
            const WCHAR *pProperty = pPropertyPath + 1;
            XUINT32 cProperty = 0;

            while (*pPropertyPath != L')' && *pPropertyPath != L'\0')
            {
                cProperty++;
                pPropertyPath++;
            }

            // If we couldn't find the ')' then this is an invalid
            // property path
            if (*pPropertyPath == L'\0')
            {
                IFC(E_INVALIDARG);
            }

            IFC(CreateDependencyPropertyPathStepDescriptor(cProperty - 1, pProperty, context, &pCurrentStep));
            
            // Add the new step
            IFC(AppendStepDescriptor(pCurrentStep));
            pCurrentStep = NULL;

            // Go to the next character
            fExpectingProperty = FALSE;
            pPropertyPath++;

            // Adjust the pointer to look for the 
            // next step in the path
            if (*pPropertyPath == L'\0')
            {
                // We're done with the parsing
                break;
            }
            else if (*pPropertyPath == L'.')
            {
                pPropertyPath++;
            }
            else if (*pPropertyPath != L'[')
            {
                IFC(E_INVALIDARG);
            }

            pCurrentProperty = pPropertyPath;
        }
        
        // We found a separator then we need to separate the strings that represent the 
        // property name and create another instance of an step. The end of the string
        // also counts as a separator
        if (*pPropertyPath == L'.' || *pPropertyPath == L'[' || *pPropertyPath == L'\0')
        {
            // The name of the property starts after the last separator until
            // the current character
            XUINT32 cProperty = (XUINT32)(pPropertyPath - pCurrentProperty);
            const WCHAR  *pProperty = pCurrentProperty;
            bool fHitIndexer = *pPropertyPath == L'[';

            // Only if actually have characters to collect can we create
            // a property, if we have something like [0][1][2] then there
            // will not be any characters to collect and thus no PropertyAccessPathStep to create
            if (cProperty > 0)
            {
                szCurrentProperty = new WCHAR[cProperty + 1];   // +1 for the 0 at the end

                // Fill the string with the current property name, which will be from the last 
                // separator until the '.'
                wcsncpy_s(szCurrentProperty, cProperty + 1, pProperty, cProperty);

                // Update the pointer for the current property
                pCurrentProperty = pPropertyPath + 1;

                // Now we can create a property path step
                pCurrentStep = new PropertyAccessPathStepDescriptor(szCurrentProperty);
                szCurrentProperty = NULL;

                // Now add the step to the list
                IFC(AppendStepDescriptor(pCurrentStep));
                pCurrentStep = NULL;

                // If the separator found was a '.' then the next 
                // step must be a property otherwise it is an indexer
                fExpectingProperty = !fHitIndexer;
            }
            else
            {
                // We were expecting a property but we got the empty string instead
                // this is an error
                if (fExpectingProperty)
                {
                    IFC(E_INVALIDARG);
                }
            }

            // If this is the last char then just break the loop
            if (*pPropertyPath == L'\0')
            {
                break;
            }

            // If we are now inside of an indexer, separated by a '[', let's extract the 
            // index, looking for the matching ']' and analize it
            // We know that at this point we're not at the end of the string, the previous 
            // condition makes sure of that
            if (fHitIndexer)
            {
                const WCHAR  *pIndex = pPropertyPath + 1;
                XUINT32 cIndex = 0;

                // Look for the matching ']' or the end of the string, whatever
                // happends first
                while (*pPropertyPath != L'\0' && *pPropertyPath != L']')
                {
                    pPropertyPath++;
                }

                // If we found the end of the string, this is a bad property path
                if (*pPropertyPath == L'\0')
                {
                    IFC(E_INVALIDARG);
                }

                cIndex = (XUINT32) (pPropertyPath - pIndex);

                szIndex = new WCHAR[cIndex + 1];

                // Fill the string with the index
                if (0 != wcsncpy_s(szIndex, cIndex + 1, pIndex, cIndex))
                {
                    IFC(E_INVALIDARG);
                }

#pragma prefast(push)
                // wcsncpy_s will always null-terminate szIndex on success
#pragma prefast(disable: __WARNING_BUFFER_OVERFLOW, "Read overflow of null terminated buffer using expression '(WCHAR *)szIndex'")
                // Create the right type of indexer
                if (IsNumericIndex(szIndex))
                {
                    pCurrentStep = new IntIndexerPathStepDescriptor(_wtoi(szIndex));
                    delete[] szIndex;
                    szIndex = NULL;
                }
                else
                {
                    // TODO: Implement the string index, perhaps it is redundant?
                    pCurrentStep = new StringIndexerPathStepDescriptor(szIndex);
                    szIndex = NULL; // The indexer now owns the string
                }
#pragma prefast(pop)

                // Now add the step to the list
                IFC(AppendStepDescriptor(pCurrentStep));
                pCurrentStep = NULL;

                // Move the char pointer to the begining of the next step
                pPropertyPath++;
                pCurrentProperty = pPropertyPath;

                // If the next character is the end of the string, then we're done
                if (*pPropertyPath == L'\0')
                {
                    break;
                }
                else if (*pPropertyPath == L'.')
                {
                    
                    // If the next character is a '.' then skip it, go to the next char so we can
                    // start collecting the next property 
                    pPropertyPath ++;
                    pCurrentProperty = pPropertyPath;
                    fExpectingProperty = TRUE;
                }
                else if (*pPropertyPath != L'[')
                {
                    // The only other thing that is legal after an indexer is another indexer or a .
                    // this is neither so error out
                    IFC(E_INVALIDARG);
                }

                // On the next step
                continue;
            }
        }

        pPropertyPath++;
    }

Cleanup:

    delete pCurrentStep;
    delete[] szCurrentProperty;
    delete[] szIndex;
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
PropertyPathParser::AppendStepDescriptor(_In_ PropertyPathStepDescriptor *pDescriptor)
{
    HRESULT hr = S_OK;

    m_descriptors.push_back(pDescriptor);

    RRETURN(hr);//RRETURN_REMOVAL
}

bool PropertyPathParser::IsNumericIndex(_In_z_ const WCHAR *szIndex)
{
    while (*szIndex != L'\0')
    {
        if (!iswdigit(*szIndex))
        {
            return false;
        }
        szIndex++;
    }

    return true;
}

_Check_return_ 
HRESULT 
PropertyPathParser::CreateDependencyPropertyPathStepDescriptor(
    _In_ XUINT32 nPropertyLength,
    _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
    _In_opt_ ::XamlServiceProviderContext* context,
    _Outptr_ PropertyPathStepDescriptor **ppDescriptor)
{
    const CDependencyProperty *pDP = nullptr;

    IFC_RETURN(GetDPFromName(nPropertyLength, pchProperty, context, &pDP));
    if (pDP == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppDescriptor = new DependencyPropertyPathStepDescriptor(pDP);
    
    return S_OK;
}

_Check_return_ 
HRESULT 
PropertyPathParser::GetDPFromName(
    _In_ XUINT32 nPropertyLength,
    _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
    _In_opt_ ::XamlServiceProviderContext* context,
    _Outptr_result_maybenull_ const CDependencyProperty **ppDP)
{
    IFC_RETURN(MetadataAPI::TryGetDependencyPropertyByFullyQualifiedName(
        XSTRING_PTR_EPHEMERAL2(pchProperty, nPropertyLength),
        context,
        ppDP));

    return S_OK;
}

