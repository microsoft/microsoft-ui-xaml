// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FacadeReferenceWrapper.h"

FacadeReferenceWrapper::FacadeReferenceWrapper()
{
}

ULONG FacadeReferenceWrapper::AddRef()
{
    ULONG returnValue = InterlockedIncrement(&m_refCount);

    return returnValue;
}

ULONG FacadeReferenceWrapper::Release()
{
    ULONG returnValue = InterlockedDecrement(&m_refCount);
    if (returnValue == 0)
    {
        delete this;
    }
    return returnValue;
}

HRESULT FacadeReferenceWrapper::QueryInterface(IN REFIID iid, OUT PVOID* result)
{
    if (iid == __uuidof(IUnknown))
    {
        *result = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// Called whenever a property is known to be referenced
void FacadeReferenceWrapper::AddReferencedPropertyID(KnownPropertyIndex facadeID)
{
    for (std::size_t i =0; i < m_propertiesReferenced.size(); i++)
    {
        if (m_propertiesReferenced[i] == facadeID)
        {
            return;
        }
    }

    m_propertiesReferenced.push_back(facadeID);
}

// Returns true if this property being referenced
bool FacadeReferenceWrapper::IsPropertyIDReferenced(KnownPropertyIndex facadeID) const
{
    for (std::size_t i =0; i < m_propertiesReferenced.size(); i++)
    {
        if (m_propertiesReferenced[i] == facadeID)
        {
            return true;
        }
    }

    return false;
}
