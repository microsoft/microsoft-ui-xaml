// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AKCommon.h"
#include "FocusProperties.h"

class VisualTree;
class CUIElementCollection;

namespace AccessKeys
{
    class AKVisualTreeLibrary;

    template<
        class Collection=CDOCollection,
        class VisualTree=VisualTree,
        class TreeLibrary=AKVisualTreeLibrary,
        class Element=CDependencyObject>
    class AKTreeAnalyzer
    {
    public:
        AKTreeAnalyzer(TreeLibrary& treeLibrary) : m_treeLibrary(treeLibrary) {}

        AKTreeAnalyzer(const AKTreeAnalyzer&) = delete;

        _Check_return_ HRESULT FindElementsForAK(_In_opt_ Element* const scopeOwner, _Inout_ std::vector<Element*>& elementList, _In_ bool returnOnFirstHit = false)
        {
            IFC_RETURN(ValidateScopeOwner(scopeOwner));

            std::vector<std::pair<Element*,Element*>> scopeOwnerMap;
            IFC_RETURN(BuildScopeOwnerMap(scopeOwnerMap));
            
            // If we don't have an element i.e. scopeOwner = nullptr , we're conceptually looking in the "root scope".  This
            // means we look at all the visual roots and gather the access keys
            if (IsRootScope(scopeOwner))
            {
                Element* roots[3] = { nullptr, nullptr, nullptr };
                m_treeLibrary.GetAllVisibleRootsNoRef(roots);
                for (const auto& root : roots)
                {
                    IFC_RETURN(WalkTreeAndFindElements(root, root, scopeOwnerMap, elementList, returnOnFirstHit));
                }
            }
            else
            {
                IFC_RETURN(WalkTreeAndFindElements(scopeOwner, scopeOwner, scopeOwnerMap, elementList, returnOnFirstHit));
            }
            return S_OK;
        }

        _Check_return_ HRESULT DoesTreeContainAKElement(_Out_ bool& containsAK)
        {
            std::vector<Element*> elementList;
            // passing nullptr as scopeOwner to search in all visual roots
            IFC_RETURN(FindElementsForAK(nullptr, elementList, true));

            containsAK = !elementList.empty();

            return S_OK;
        }

        Element* GetScopeOwner(_In_ Element* e)
        {
            if (m_treeLibrary.IsScope(e))
            {
                Element* parent = m_treeLibrary.GetParent(e);
                if (!parent)
                {
                    // Some elements, like Flyouts, may have a mentor but no parent.
                    parent = e->GetMentor();
                }
                e = parent;
            }

            return GetScope(e);
        }

        _Check_return_ HRESULT BuildScopeOwnerMap(std::vector<std::pair<Element*,Element*>>& scopeOwnerMap)
        {
            Element* roots[3] = { nullptr, nullptr, nullptr };
            m_treeLibrary.GetAllVisibleRootsNoRef(roots);
            for (const auto& root : roots)
            {
                if (root)
                {
                    IFC_RETURN(BuildScopeOwnerMapImpl(root, scopeOwnerMap));
                }
            }
            return S_OK;
        }

        _Check_return_ HRESULT BuildScopeOwnerMapImpl(_In_ Element* current, _Inout_ std::vector<std::pair<Element*,Element*>>& scopeOwnerMap)
        {
            if (current == nullptr) return S_OK;

            auto scopeOwner = m_treeLibrary.GetScopeOwner(current);
            if (scopeOwner)
            {
                // The scopeOwner must be a scope itself
                IFCEXPECT_RETURN(m_treeLibrary.IsScope(scopeOwner));
                scopeOwnerMap.emplace_back(scopeOwner, current);
            }

            Collection* collection = m_treeLibrary.GetChildren(current);

            if (collection && !collection->IsLeaving())
            {
                const unsigned int kidCount = collection->GetCount();
                for (unsigned int i = 0; i < kidCount; i++)
                {
                    xref_ptr<Element> child;
                    child.attach(static_cast<Element*>(collection->GetItemWithAddRef(i)));

                    IFC_RETURN(BuildScopeOwnerMapImpl(child.get(), scopeOwnerMap));
                }
            }
            return S_OK;
        }

        _Check_return_ HRESULT WalkTreeAndFindElements(
            _In_ const Element* const startRoot,
            _In_ Element* const currentElement,
            _In_ const std::vector<std::pair<Element*, Element*>>& scopeOwnerMap,
            _Inout_ std::vector<Element*>& elementList,
            _In_ bool returnOnFirstHit = false,
            _In_ int depth = MaxDepth
            )
        {
            if (depth == 0)
            {
                // Possible cycle
                return E_OUTOFMEMORY;
            }

            if (currentElement == nullptr)
            {
                return S_OK;
            }

            // If this element doesn't represent the scope, and it has an access key,
            // add the access key to the list
            if (startRoot != currentElement && IsValidAKElement(currentElement))
            {
                elementList.push_back(currentElement);

                //We want to find if the tree has an AK elements. If it does, there is no need to search
                //any further
                if (returnOnFirstHit) { return S_OK; }
            }

            // If we hit a new scope root, we've hit the edge of the current scope.
            // Don't walk the children.
            const bool isNewScope = startRoot != currentElement && m_treeLibrary.IsScope(currentElement);
            if (!isNewScope)
            {
                Collection* collection = m_treeLibrary.GetChildren(currentElement);
                if (collection && !collection->IsLeaving())
                {
                    const unsigned int kidCount = collection->GetCount();

                    for (unsigned int i = 0; i < kidCount; i++)
                    {
                        xref_ptr<Element> child;
                        child.attach(static_cast<Element*>(collection->GetItemWithAddRef(i)));

                        if (child.get())
                        {
                            auto owner = m_treeLibrary.GetScopeOwner(child.get());
                            // if owner is nullptr that shows that no specific scope owner is defined for this child element
                            // the start root can be considered as the scope for finding access keys
                            if (owner == nullptr)
                            {
                                IFC_RETURN(WalkTreeAndFindElements(startRoot, child.get(), scopeOwnerMap, elementList, returnOnFirstHit, depth-1));
                            }
                        }
                    }
                }

                // Find the children explicitly grafted to this scope
                for (const auto& e : scopeOwnerMap)
                {
                    if (e.first == currentElement)
                    {
                        IFC_RETURN(WalkTreeAndFindElements(startRoot, e.second, scopeOwnerMap, elementList, returnOnFirstHit, depth-1));
                    }
                }
            }
            return S_OK;
        }

        bool IsScopeOwner(_In_ Element* current) const
        {
            return m_treeLibrary.IsScope(current);
        }

        bool IsValidAKElement(_In_ Element* const element) const 
        {
            return IsAccessKey(element) && FocusProperties::IsVisible(element) 
                && FocusProperties::AreAllAncestorsVisible(element) && FocusProperties::IsEnabled(element);
        }

        bool IsAccessKey(_In_ Element* const element) const
        {
            return GetAccessKey(element).IsNullOrEmpty() == false;
        }

    private:

        Element* GetScope(_In_opt_ Element* e)
        {
            // If we're visiting too many nodes during the walk, we probably found a cycle.
            // Keep track of iterations so we don't loop forever.
            int iterations = MaxDepth;
            while (iterations-- != 0)
            {
                if (!e)
                {
                    // We walked up through the root of the tree.  Consider
                    // nullptr the root scope.
                    return nullptr;
                }
 
                if (m_treeLibrary.IsScope(e))
                {
                    return e;
                }

                Element* owner = m_treeLibrary.GetScopeOwner(e);
                if (owner != nullptr)
                {
                    e = owner;
                }
                else
                {
                    e = m_treeLibrary.GetParent(e);
                }
           };
            // Tree is unexpectedly deep, or we hit a cycle somehow.  Bail here and get watson data
            XAML_FAIL_FAST(); // this won't return
            return nullptr;
        }

        static const int MaxDepth = 200;

        static bool IsRootScope(_In_ Element* const scopeOwner)
        {
            return scopeOwner == nullptr;
        }

        HRESULT ValidateScopeOwner(_In_ Element* const scopeOwner)
        {
            if (!IsRootScope(scopeOwner))
            {
                // Element must be a scope
                IFCEXPECT_RETURN(m_treeLibrary.IsScope(scopeOwner));
            }

            return S_OK;
        }

        TreeLibrary& m_treeLibrary;
    };

}
