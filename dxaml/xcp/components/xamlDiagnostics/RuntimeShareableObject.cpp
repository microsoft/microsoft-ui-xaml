// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeShareableObject.h"
#include "RuntimeObjectCache.h"
#include "RuntimeDictionary.h"

namespace Diagnostics
{
    bool RuntimeShareableObject::TryGetAsShareable(std::shared_ptr<RuntimeShareableObject>& shareable)
    {
        shareable = derived_shared_from_this<RuntimeShareableObject>();
        return true;
    }

    void RuntimeShareableObject::AddParent(std::shared_ptr<RuntimeObject> parent)
    {
        m_otherParents.push_back(parent);
        if (!HasParent())
        {
            // If we don't have an actual parent, then we are still in the RuntimeObjectCache
            // we can remove ourselves now.
            GetRuntimeObjectCache()->RemoveFromCache(shared_from_this());
        }
    }

    void RuntimeShareableObject::RemoveParent(std::shared_ptr<RuntimeObject> parent)
    {
        auto iter = std::find_if(m_otherParents.begin(), m_otherParents.end(), [parent](auto& weakParent) {
            return weakParent.lock() == parent;
        });
        if (iter != m_otherParents.end())
        {
            m_otherParents.erase(iter);
            m_otherParents.shrink_to_fit();
        }
    }

    void RuntimeShareableObject::SetParent(std::shared_ptr<RuntimeObject> parent)
    {
        std::shared_ptr<RuntimeDictionary> parentDictionary;
        const bool wasInDictionary = HasParent() && GetParent()->TryGetAsDictionary(parentDictionary);
        RuntimeObject::SetParent(parent);
        if (!HasParent() && wasInDictionary && parentDictionary->ShouldKeepAlive(shared_from_this()))
        {
            GetRuntimeObjectCache()->AddToCache(shared_from_this());
        }
    }
}
