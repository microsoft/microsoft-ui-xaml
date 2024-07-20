// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeObject.h"
namespace Diagnostics
{
    class RuntimeApplication final : public RuntimeObject
    {
        friend std::shared_ptr<RuntimeApplication> GetRuntimeApplication(_In_ IInspectable* backingObject);
    public:
        void Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent) final;
        bool TryGetAsApplication(std::shared_ptr<RuntimeApplication>& shareable) final;
    private:
        RuntimeApplication() = default;
    };
}
