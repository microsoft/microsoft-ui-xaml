// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "RuntimeObject.h"
#include "RuntimeCollection.h"
#include <vector>
#include <memory>

enum class KnownTypeIndex : UINT16;
enum class KnownPropertyIndex : UINT16;

namespace Diagnostics
{
    class RuntimeElement final : public RuntimeObject
    {
        friend std::shared_ptr<RuntimeObject> GetRuntimeObject(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent);
    public:
        std::shared_ptr<RuntimeCollection> GetChildren();

        bool TryGetAsElement(std::shared_ptr<RuntimeElement>& element) final;
        // When a RuntimeElement leaves the tree it needs to unhook itself from the parent
        void Close() final;

        // The following set of *Child APIs check/modify the local state of the RuntimeElement
        // and are in response to visual tree mutations coming from the app. They don't modify
        // the tree in any way.
        void RemoveChild(std::shared_ptr<RuntimeElement> child);
        bool HasChild(std::shared_ptr<RuntimeElement> child);
        void AddChild(std::shared_ptr<RuntimeElement> child);

        Microsoft::WRL::ComPtr<xaml::IUIElement> GetBackingElement() const;
        bool IsRoot() const;
    private:
        void EnsureChildren();
        void Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent) final;
        RuntimeElement() = default;

    private:
        std::shared_ptr<RuntimeCollection> m_children;
    };
}
