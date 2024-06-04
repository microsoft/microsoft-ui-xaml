// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeObject.h"
#include <vector>

namespace Diagnostics
{
    // RuntimeShareableObject is used to represent objects that could potentially have multiple objects. These
    // are things like Styles and Brushes. This is eerily similar to CMultiParentShareableDependencyObject, and
    // that is intentional. Note that currently, Style is **not** a CMultiParentShareableDependencyObject, but by
    // golly it should be. In this case, the parent stored on the base class is the actual parent of the object.
    // So if the style or brush is stored in a ResourceDictionary, it will be the dictionary. Any other parents
    // represent objects this was applied to.
    class RuntimeShareableObject final : public RuntimeObject
    {
        friend std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    public:
        bool TryGetAsShareable(std::shared_ptr<RuntimeShareableObject>& shareable) final;

        void AddParent(std::shared_ptr<RuntimeObject> parent);
        void RemoveParent(std::shared_ptr<RuntimeObject> parent);
        void SetParent(std::shared_ptr<RuntimeObject> parent) final;
    private:
        RuntimeShareableObject() = default;
        std::vector<std::weak_ptr<RuntimeObject>> m_otherParents;
    };
}
