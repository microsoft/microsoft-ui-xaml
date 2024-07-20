// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeApplication.h"
#include "helpers.h"

namespace Diagnostics
{
    void RuntimeApplication::Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        wrl::ComPtr<xaml::IApplication> application;
        IFCFAILFAST(backingObject->QueryInterface(application.GetAddressOf()));
        RuntimeObject::Initialize(backingObject, parent);

        wrl::ComPtr<xaml::IResourceDictionary> appResources;
        IFCFAILFAST(application->get_Resources(&appResources));
        auto runtimeDictionary = GetRuntimeObject(appResources.Get(), shared_from_this());
        StoreValue(RuntimeProperty(KnownPropertyIndex::Application_Resources), runtimeDictionary);
    }

    bool RuntimeApplication::TryGetAsApplication(std::shared_ptr<RuntimeApplication>& application)
    {
        application = derived_shared_from_this<RuntimeApplication>();
        return true;
    }
}
