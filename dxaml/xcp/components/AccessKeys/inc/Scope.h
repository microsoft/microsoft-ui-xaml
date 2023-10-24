// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <AccessKey.h>
#include <AccessKeysOwner.h>
#include "AKCommon.h"
#include <unordered_set>


namespace AccessKeys {

    // Represents a set of Access Key owners of which any could be invoked.  Checks current input fed from the ScopeTree
    // for matching keys and invokes on a match.
    template<class Element = CDependencyObject, class Owner = AKOwner<CDependencyObject>>
    class AKScope
    {
    public:
        AKScope(_In_ Element* const scopeParentElement, _In_ const std::vector<std::pair<Element*, AKAccessKey>>& initList):
            inputAccumulator(L""),
            scopeParentElement(xref::get_weakref<Element>(scopeParentElement))
        {
            accessKeyOwners.reserve(initList.size());
            for (const auto& item : initList)
            {
                accessKeyOwners.insert(std::make_pair(item.second, std::make_shared<Owner>(item.first,item.second)));
            }
        }

        _Check_return_ HRESULT Invoke(_In_ const wchar_t inputCharacter, _In_ bool allowPartialMatching, _Out_ AKInvokeReturnParams<Element>* invokeResult)
        {
            inputAccumulator += inputCharacter;
            AKAccessKey keyToMatch(inputAccumulator);

            *invokeResult = TryInvokeDirectMatch(keyToMatch);
            if (invokeResult->invokeAttempted)
            {
                // Successfully found and invoked a match.
                inputAccumulator = L"";
                return S_OK;
            }
            else if (allowPartialMatching && HasPartialMatch(keyToMatch))
            {
                // Have a partial match, update the visual state of the AccessKeys
                IFC_RETURN(UpdatePartialMatchAccessKeyVisibility(keyToMatch));
                // Do not reset the input accumulator, return false (because no match - scope is likely still valid).
                invokeResult->invokeAttempted = true;
                return S_OK;
            }

            // No direct or partial matches.
            // Don't reset the inputAccumulator if filtering, but pop off the last character.
            if (inputAccumulator.length() > 1)
            {
                inputAccumulator.pop_back();
                invokeResult->invokeAttempted = false;
                return S_OK;
            }
            else
            {
                inputAccumulator = L"";
                invokeResult->invokeAttempted = false;
                return S_OK;
            }
        }

        // Shows all access keys in the scope.  Calls ShowAccessKey for each AccessKey owner in this scope.
        _Check_return_ HRESULT ShowAccessKeys() const
        {
            for (const auto pair : accessKeyOwners)
            {
                std::shared_ptr<Owner> owner = pair.second;
                IFC_RETURN(owner->ShowAccessKey(L"" /*pressed keys - empty string means no keys pressed*/));
            }

            return S_OK;
        }

        _Check_return_ HRESULT HideAccessKeys() const
        {
            for (const auto pair : accessKeyOwners)
            {
                std::shared_ptr<Owner> owner = pair.second;
                IFC_RETURN(owner->HideAccessKey());
            }

            return S_OK;
        }

        _Check_return_ HRESULT AddToAccessKeyOwner(_In_ Element* const element)
        {
            xstring_ptr accessString = GetAccessKey(element);
            std::shared_ptr<Owner> owner = std::make_shared<Owner>(element, std::wstring(accessString.GetBuffer()));

            if (!accessString.IsNullOrEmpty() && !ContainsOwner(owner->GetAccessKey()))
            {
                accessKeyOwners.insert(std::make_pair(owner->GetAccessKey(), owner));
            }

            //We always fire the show when an element is added for scenerios where an element has
            //been added to the scope, but failed to fire. This can happen in nested flyout scenarios
            //where the flyout is visible in the tree, but has not fired yet
            AKAccessKey keyToMatch(inputAccumulator);
            if (keyToMatch.IsPartialMatch(owner->GetAccessKey()))
            {
                IFC_RETURN(owner->ShowAccessKey(keyToMatch.GetAccessKeyString()));
            }

            return S_OK;
        }

        _Check_return_ HRESULT RemoveFromAccessKeyOwner(_In_ Element* const element)
        {
            xstring_ptr accessString = GetAccessKey(element);
            std::shared_ptr<Owner> owner = std::make_shared<Owner>(element, std::wstring(accessString.GetBuffer()));
            AKAccessKey keyToMatch(inputAccumulator);

            if (!accessString.IsNullOrEmpty() && ContainsOwner(owner->GetAccessKey()))
            {
                auto matchingEntry = accessKeyOwners.find(owner->GetAccessKey());

                // Make sure the element getting removed is the same one that we have in the accessKeyOwners
                // map.  It could be that the caller is calling about an element that we're no longer tracking,
                // that just happens to have the same access key (true story!  bug 8455086)
                if (matchingEntry->second->GetElement().lock().get() == element)
                {
                    accessKeyOwners.erase(matchingEntry);

                    if (keyToMatch.IsPartialMatch(owner->GetAccessKey()))
                    {
                        IFC_RETURN(owner->HideAccessKey());
                    }
                }
            }

            return S_OK;
        }

        bool ShouldElementEnteringTreeUpdateScope(_In_ const Element* const scopeOwner) const
        {
            return lastInvokeResult.invokedElement.lock_noref() == scopeOwner && lastInvokeResult.invokeAttempted;
        }

        xref::weakref_ptr<Element> GetScopeParent() const { return scopeParentElement; }


        bool IsScopeFilteringInput() const
        {
            return !inputAccumulator.empty();
        }

        _Check_return_ HRESULT ProcessEscapeKey()
        {
            if (IsScopeFilteringInput())
            {
                inputAccumulator.pop_back();
                // Call show on the elements that have now been filtered in
                AKAccessKey keyToMatch(inputAccumulator);
                IFC_RETURN(UpdatePartialMatchAccessKeyVisibility(keyToMatch));
            }
            return S_OK;
        }

    private:
        std::wstring inputAccumulator;

        // To gaurd against reentrancy, we should always take a copy of the owner instead of a reference when iterating
        std::unordered_map<AKAccessKey, std::shared_ptr<Owner>> accessKeyOwners;

        xref::weakref_ptr<Element> scopeParentElement;
        AKInvokeReturnParams<Element> lastInvokeResult;

        // Returns true when a match was found.  It is invoked.  Otherwise false.
        AKInvokeReturnParams<Element> TryInvokeDirectMatch(_In_ const AKAccessKey& inputKey)
        {
            AKInvokeReturnParams<Element> invokeResult;
            invokeResult.invokeAttempted = false;
            for (const auto& pair : accessKeyOwners)
            {
                std::shared_ptr<Owner> owner = pair.second;

                if (owner->GetAccessKey() == inputKey)
                {
                    invokeResult.invokeFoundValidPattern = owner->Invoke();

                    invokeResult.invokeAttempted = true;
                    invokeResult.invokedElement = owner->GetElement();

                    lastInvokeResult = invokeResult;
                    return invokeResult;
                }
            }
            return invokeResult;
        }

        // Returns true if a partial match has been found.  A partial match means that the key sequence entered thus far
        // matches the starting key sequence of at least one AKOwner's AKAccessKey.  False otherwise.
        bool HasPartialMatch(const AKAccessKey& inputKey) const
        {
            for (const auto pair : accessKeyOwners)
            {
                std::shared_ptr<Owner> owner = pair.second;

                if (inputKey.IsPartialMatch(owner->GetAccessKey()))
                {
                    return true;
                }
            }
            return false;
        }

        _Check_return_ HRESULT UpdatePartialMatchAccessKeyVisibility(const AKAccessKey& inputKey) const
        {
            for (const auto pair : accessKeyOwners)
            {
                std::shared_ptr<Owner> owner = pair.second;

                // For partialMatches and non-matches, send a showAccessKey/HideAccessKey event to the owner->  This way
                // visuals can be updated to reflect each key stroke (both positive match feedback, and negative match feedback).
                if (inputKey.IsPartialMatch(owner->GetAccessKey()))
                {
                    IFC_RETURN(owner->ShowAccessKey(inputKey.GetAccessKeyString()));
                }
                else
                {
                    IFC_RETURN(owner->HideAccessKey());
                }
            }

            return S_OK;
        }

        bool ContainsOwner(_In_ const AKAccessKey& accessKey) const
        {
            return accessKeyOwners.count(accessKey) > 0;
        }

    };

    typedef AKScope<> Scope;
}
