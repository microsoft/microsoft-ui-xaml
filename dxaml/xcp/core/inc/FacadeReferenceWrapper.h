// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>
#include <indexes.g.h>


// FacadeReferenceWrapper is a helper/wrapper class for IAnimatable references.
// It does book-keeping on which properties are being referenced, which allows us to keep only
// those properties being referenced up to date.
//
// A FacadeReferenceWrapper will be created for a given DO the first time a reference is
// requested through IAnimatable.  There is currently no mechanism in Composition to notify
// the framework when all references are no longer being used so this object will stay alive
// for the lifetime of the DO once a reference has been established.

class FacadeReferenceWrapper : public IUnknown
{
public:
    FacadeReferenceWrapper();
    ~FacadeReferenceWrapper() {}

    // IUnknown
    STDMETHOD(QueryInterface)(IN REFIID iid, OUT PVOID *result);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    void AddReferencedPropertyID(KnownPropertyIndex facadeID);
    bool IsPropertyIDReferenced(KnownPropertyIndex facadeID) const;

private:
    std::vector<KnownPropertyIndex> m_propertiesReferenced;
    ULONG m_refCount = 1;
};
