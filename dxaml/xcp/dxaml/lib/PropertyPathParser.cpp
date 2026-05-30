// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPathParser.h"
#include "PropertyPathCommonNames.h"
#include <new>

using namespace DirectUI;
using namespace xaml_data;

// ============================================================================
// PropertyPathParser - Iterator support
// ============================================================================

size_t PropertyPathParser::size() const noexcept
{
    if (m_descriptors[0].GetKind() == PropertyPathStepDescriptorKind::None)
    {
        return 0;
    }
    if (m_descriptors[0].GetKind() == PropertyPathStepDescriptorKind::HeapStorage)
    {
        return m_descriptors[0].GetHeapStorage()->count;
    }
    // Inline storage: 1 or 2 elements
    return m_descriptors[1].GetKind() == PropertyPathStepDescriptorKind::None ? 1 : 2;
}

PropertyPathStepDescriptor* PropertyPathParser::begin() noexcept
{
    if (empty())
    {
        return nullptr;
    }
    // If first descriptor is HeapStorage, iterate over the heap array
    if (m_descriptors[0].GetKind() == PropertyPathStepDescriptorKind::HeapStorage)
    {
        return m_descriptors[0].GetHeapStorage()->begin();
    }
    // Otherwise iterate over inline array
    return &m_descriptors[0];
}

PropertyPathStepDescriptor* PropertyPathParser::end() noexcept
{
    if (empty())
    {
        return nullptr;
    }
    if (m_descriptors[0].GetKind() == PropertyPathStepDescriptorKind::HeapStorage)
    {
        return m_descriptors[0].GetHeapStorage()->end();
    }
    return &m_descriptors[0] + size();
}

// Called from PropertyPathParser::Parse to move descriptors from stack storage into the object.
// Must only be called once per object.
void PropertyPathParser::FinalizeDescriptors(Jupiter::stack_vector<PropertyPathStepDescriptor, 2>& source)
{
    // Assert this hasn't been called for the object yet
    ASSERT(size() == 0);

    const size_t count = source.m_vector.size();

    if (count == 0)
    {
        return;
    }

    if (count <= 2)
    {
        // Move directly into inline storage
        for (size_t i = 0; i < count; ++i)
        {
            m_descriptors[i] = std::move(source.m_vector[i]);
        }
    }
    else
    {
        // Allocate heap storage and move all descriptors there
        HeapDescriptorStorage* heapStorage = HeapDescriptorStorage::Allocate(count);
        PropertyPathStepDescriptor* heapDescriptors = heapStorage->begin();
        for (size_t i = 0; i < count; ++i)
        {
            heapDescriptors[i] = std::move(source.m_vector[i]);
        }
        // Store the heap storage pointer in the first inline slot
        m_descriptors[0] = PropertyPathStepDescriptor::CreateHeapStorage(heapStorage);
    }

    source.m_vector.clear();
}

// ============================================================================
// PropertyPathParser implementation
// ============================================================================

_Check_return_ 
HRESULT 
PropertyPathParser::SetSource(_In_opt_z_ const WCHAR *szPath, _In_opt_ ::XamlServiceProviderContext* context)
{
    // The source can only be called once
    IFCEXPECT_RETURN(!HasParsedPath());
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
    PropertyPathStepDescriptor currentStep;
    bool fExpectingProperty = false;

    // Build up descriptors in a local stack_vector during parsing.
    // At the end, we'll transfer to our storage with FinalizeDescriptors.
    Jupiter::stack_vector<PropertyPathStepDescriptor, 2> localDescriptors;

    // If the property path is empty or NULL then this means that we're binding
    // directly to the source
    if (szPropertyPath == NULL || szPropertyPath[0] == L'\0')
    {
        currentStep = PropertyPathStepDescriptor::CreateSourceAccess();
        localDescriptors.m_vector.emplace_back(std::move(currentStep));
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

            IFC(CreateDependencyPropertyPathStepDescriptor(cProperty - 1, pProperty, context, &currentStep));
            localDescriptors.m_vector.emplace_back(std::move(currentStep));

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
                // Check if this is a common property name we can use without allocation
                const WCHAR* commonName = TryGetCommonPropertyName(pProperty, cProperty);
                if (commonName != nullptr)
                {
                    // Use the shared static string - no heap allocation needed
                    currentStep = PropertyPathStepDescriptor::CreatePropertyAccessShared(commonName);
                }
                else
                {
                    szCurrentProperty = new WCHAR[cProperty + 1];   // +1 for the 0 at the end

                    // Fill the string with the current property name, which will be from the last 
                    // separator until the '.'
                    wcsncpy_s(szCurrentProperty, cProperty + 1, pProperty, cProperty);

                    // Now we can create a property path step
                    currentStep = PropertyPathStepDescriptor::CreatePropertyAccess(szCurrentProperty);
                    szCurrentProperty = NULL; // Descriptor owns the string now
                }

                // Update the pointer for the current property
                pCurrentProperty = pPropertyPath + 1;

                localDescriptors.m_vector.emplace_back(std::move(currentStep));

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
            // index, looking for the matching ']' and analyze it
            // We know that at this point we're not at the end of the string, the previous 
            // condition makes sure of that
            if (fHitIndexer)
            {
                const WCHAR  *pIndex = pPropertyPath + 1;
                XUINT32 cIndex = 0;

                // Look for the matching ']' or the end of the string, whatever
                // happens first
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

                // Check if this is a numeric index without allocating
                if (IsNumericIndex(pIndex, cIndex))
                {
                    // _wtoi stops at first non-digit (should be something like ']'), so no copy needed
                    currentStep = PropertyPathStepDescriptor::CreateIntIndexer(_wtoi(pIndex));
                }
                else
                {
                    // String indexer - need to allocate and copy
                    szIndex = new WCHAR[cIndex + 1];

                    // Fill the string with the index
                    if (0 != wcsncpy_s(szIndex, cIndex + 1, pIndex, cIndex))
                    {
                        IFC(E_INVALIDARG);
                    }

                    // TODO: Implement the string index, perhaps it is redundant?
                    currentStep = PropertyPathStepDescriptor::CreateStringIndexer(szIndex);
                    szIndex = NULL; // The descriptor now owns the string
                }


                // Now add the step to the list
                localDescriptors.m_vector.emplace_back(std::move(currentStep));

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

    delete[] szCurrentProperty;
    delete[] szIndex;

    // Transfer descriptors to optimal storage if parsing succeeded
    if (SUCCEEDED(hr))
    {
        FinalizeDescriptors(localDescriptors);
    }

    RRETURN(hr);
}

// pIndex might not be null-terminated
bool PropertyPathParser::IsNumericIndex(_In_reads_(length) const WCHAR* pIndex, size_t length)
{
    if (length == 0)
    {
        return false;
    }

    for (size_t i = 0; i < length; ++i)
    {
        if (!iswdigit(pIndex[i]))
        {
            return false;
        }
    }

    return true;
}

_Check_return_ 
HRESULT 
PropertyPathParser::CreateDependencyPropertyPathStepDescriptor(
    _In_ XUINT32 nPropertyLength,
    _In_reads_(nPropertyLength + 1) const WCHAR *pchProperty,
    _In_opt_ ::XamlServiceProviderContext* context,
    _Out_ PropertyPathStepDescriptor *pDescriptor)
{
    const CDependencyProperty *pDP = nullptr;

    IFC_RETURN(GetDPFromName(nPropertyLength, pchProperty, context, &pDP));
    if (pDP == nullptr)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *pDescriptor = PropertyPathStepDescriptor::CreateDependencyProperty(pDP);
    
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

