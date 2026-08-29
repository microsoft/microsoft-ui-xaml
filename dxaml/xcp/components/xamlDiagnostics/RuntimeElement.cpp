// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeElement.h"
#include "DiagnosticsInterop.h"
#include "collection\inc\docollection.h"
#include "comInstantiation.h"
#include <runtimeEnabledFeatures\inc\RuntimeEnabledFeatures.h>
#include "RuntimeObjectCache.h"
#include "RuntimeCollection.h"

#include "XamlDiagnostics.h"
#include "dependencyLocator\inc\DependencyLocator.h"
#include "RuntimeObject.h"
#include "DependencyObject.h"
#include "DoPointerCast.h"
#include <WeakReferenceSource.h>

using namespace RuntimeFeatureBehavior;

namespace Diagnostics
{
    std::shared_ptr<RuntimeCollection> RuntimeElement::GetChildren()
    {
        EnsureChildren();
        return m_children;
    }

    void RuntimeElement::EnsureChildren()
    {
        if (m_children == nullptr)
        {
            auto interop = GetInterop();
            wrl::ComPtr<IInspectable> children;
            IFCFAILFAST(interop->GetChildren(GetBackingObject().Get(), &children));
            if (children)
            {
                m_children = GetRuntimeCollection(children.Get(), shared_from_this());
            }
        }
    }

    bool RuntimeElement::TryGetAsElement(std::shared_ptr<RuntimeElement>& element)
    {
        element = derived_shared_from_this<RuntimeElement>();
        return true;
    }

    void RuntimeElement::Initialize(_In_ IInspectable* backingObject, std::shared_ptr<RuntimeObject> parent)
    {
        RuntimeObject::Initialize(backingObject, parent);
        EnsureChildren();
    }

    bool RuntimeElement::IsRoot() const
    {
        return !HasParent() && (IsWindow() || IsDesktopWindowXamlSource() || IsXamlIsland());
    }

    void RuntimeElement::RemoveChild(std::shared_ptr<RuntimeElement> child)
    {
        size_t index = 0;
        // Depending on if this item was removed as a result of Edit-and-Continue
        // or from the app removing it, this item might already be removed from the
        // parent collection
        if (m_children && m_children->TryGetIndexOf(child, index))
        {
            m_children->ModifyInternalStateToMatchBackingCollectionChange(index, child, wfc::CollectionChange_ItemRemoved);
            if (m_children->empty())
            {
                // We don't have any more children, get rid of our collection. For most elements,
                // if we are leaving we'll be cleaned up soon, however the window element is a bit
                // different in that it can stay in the cache if we unadvise and then advise. It doesn't
                // get cleaned up until the core is deinitialized or the instance of Xaml Diagnostics
                // goes away. We want to ensure it is properly initialized again.
                m_children.reset();
            }
        }
    }

    void RuntimeElement::AddChild(std::shared_ptr<RuntimeElement> child)
    {
        EnsureChildren();
        // CDependencyObject calls Enter on all DependencyObject properties. Not all of these CDependencyObjects will have
        // a children collection
        if (m_children)
        {
            size_t index;
            if (!m_children->TryGetIndexOf(child, index))
            {
                // This is a little strange, but when we enter our ItemsControl.ItemsHost, it isn't a
                // logical child of the element in the tree, so it isn't contained in the children collection.
                // Unintentionally, we've reported this as being at index 0, even though it wasn't found. We'll maintain this for
                // compatibility reasons, and it can be addressed later to see if it's actually needed. For now, it doesn't hurt
                // anyone.
                index = 0;
            }
            m_children->ModifyInternalStateToMatchBackingCollectionChange(index, child, wfc::CollectionChange_ItemInserted);
        }
    }

    bool RuntimeElement::HasChild(std::shared_ptr<RuntimeElement> child)
    {
        EnsureChildren();
        if (m_children)
        {
            auto iter = std::find(m_children->begin(), m_children->end(), child);
            return iter != m_children->end();
        }
        return false;
    }

    wrl::ComPtr<xaml::IUIElement> RuntimeElement::GetBackingElement() const
    {
        wrl::ComPtr<xaml::IUIElement> backingElement;
        IFCFAILFAST(GetBackingObject().As(&backingElement));
        return backingElement;
    }

    void RuntimeElement::Close()
    {
        std::shared_ptr<RuntimeObject> parent;
        if (TryGetParent(parent))
        {
            std::shared_ptr<RuntimeElement> parentElement;
            if (parent->TryGetAsElement(parentElement))
            {
                parentElement->RemoveChild(derived_shared_from_this<RuntimeElement>());
            }
        }
        RuntimeObject::Close();
    }
}
