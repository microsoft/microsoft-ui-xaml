// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AccessKey.h"
#include "AKCommon.h"
#include <DependencyLocator.h>
#include "TreeAnalyzer.h"
#include "AccessKeysParser.h"
#include "Scope.h"

namespace AccessKeys {

    template<class TreeAnalyzer=AKTreeAnalyzer, class Parser=AKParser, class Element=CDependencyObject, class Scope=AKScope>
    class AKScopeBuilder
    {
    public:
        AKScopeBuilder(TreeAnalyzer& treeAnalyzer) : m_treeAnalyzer(treeAnalyzer)
        {
        }

        AKScopeBuilder(const AKScopeBuilder&) = delete;

        //Returns the scope if it was created, otherwise, return a nullptr
        _Check_return_ HRESULT ConstructScope(_In_opt_ Element* const parentElementForNewScope, _Inout_ std::shared_ptr<Scope>& newScope)
        {
            //We want to figure out which scope we are currently on and attempt to build the next scope.
            //Once we've determined the UI Element that represent that scope, fetch all the eligible elements, match the elements 
            //with the appropriate access key based on the information from the parser, and then create the scope

            std::vector<Element*> elementsForNewScope;
            IFC_RETURN(GetElementsForAKScope(parentElementForNewScope, elementsForNewScope));

            //A scope cannot exist without an AKO. If the scope init list is empty, that means that this scope is invalid
            if (elementsForNewScope.empty())
            {
                return S_OK;
            }

            std::vector<std::pair<Element*, AKAccessKey>> scopeInitList;
            scopeInitList.reserve(elementsForNewScope.size());

            for (const auto& element : elementsForNewScope)
            {
                AKAccessKey accessKey;
                const xstring_ptr& accessString = GetAccessKey(element);

                bool succeeded = Parser::TryParseAccessKey(std::wstring(accessString.GetBuffer()), accessKey);

                //We only want to add to the init list if the parsing of the element was successful
                if (succeeded)
                {
                    //A scope needs a list of all the valid AccessKeys. It uses this in order to create all the AKOs
                    scopeInitList.push_back(std::make_pair(element, accessKey));
                }
            }

            if (scopeInitList.empty())
            {
                return S_OK;
            }

            //Create the new scope
            newScope = std::make_shared<Scope>(parentElementForNewScope, scopeInitList);
            return S_OK;
        }

    private:
        _Check_return_ HRESULT GetElementsForAKScope(_In_ Element* const scopeOwner, _Inout_ std::vector<Element*>& elementList)
        {
            return m_treeAnalyzer.FindElementsForAK(scopeOwner, elementList);
        }

        TreeAnalyzer& m_treeAnalyzer;
    };
}
