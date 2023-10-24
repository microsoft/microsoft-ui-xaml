// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ModeContainer.h"
#include <DependencyLocator.h>
#include "MUX-ETWEvents.h"
#include <Scope.h>
#include <ScopeBuilder.h>
#include <FocusProperties.h>
#include "AKCommon.h"
#include "TypeTableStructs.h"
#include "EnumDefs.g.h"

namespace AccessKeys {
    template <
        class Scope=AKScope,
        class ScopeBuilder=AKScopeBuilder,
        class ModeContainer=AKModeContainer,
        class Element=CDependencyObject,
        class TreeAnalyzer=AKTreeAnalyzer,
        class CFocusManager=CFocusManager>
    class AKScopeTree
    {
    public:
        AKScopeTree(ScopeBuilder& builder, TreeAnalyzer& treeAnalyzer, ModeContainer& modeContainer) :
            m_scopeBuilder(builder),
            m_treeAnalyzer(treeAnalyzer),
            m_pFocusManager(nullptr),
            m_modeContainer(modeContainer)
        {
        }

        _Check_return_ HRESULT ProcessCharacter(_In_ const wchar_t character, _Out_ bool* wasInvoked)
        {
            (*wasInvoked) = false;

            const bool wasActive = m_modeContainer.GetIsActive();

            // First, check to see if we're exiting with an alt key. In this case Mnemonics mode is not active (GetIsActive == false on entering this method) unlike the case
            // Where ProcessCharacter is called with a mode entering alt (GetIsActive == true in that case)
            // We can also exit ak mode based on certain input.  ShouldForciblyExitAKMode in mode container captures this detail, therefore, we should also
            // exit when this is true
            // If we are not, and if we are just entering AK mode, then we want to create the new scope but not invoke it. This is because the scope being 
            // created is the root scope that contains all the scopes that the user can interact with
            if ((!wasActive && character == ALT) || m_modeContainer.ShouldForciblyExitAKMode())
            {
                IFC_RETURN(ExitScope(true));
            }
            else if (m_modeContainer.HasAKModeChanged())
            {
                // If this is a hotkey or the first 'normal' invocation build an access key scope
                IFC_RETURN(UpdateScopeImpl(wasActive, GetFocusedElementNoRef()));
            }

            if (character == ESC)
            {
                IFC_RETURN(ProcessEscapeKey(wasInvoked));
            }
            else if(character != ALT)
            {
                IFC_RETURN(ProcessNormalKey(character, wasActive, wasInvoked));
            }

            const bool isActive = m_modeContainer.GetIsActive();
            const bool wasDeactivated = wasActive && !isActive;
            // The way hotkeys flows through this code is that we do not set mnemonics mode active (e.g. GetIsActive=false) but we set HasAKModeChanged to true.
            // Todo: Refactor this obscure state into an enum so it's a little more clear what exactly is being handled (e.g. normal AK, escape, hotkey etc).
            const bool wasHotkeyInvocation = !isActive && m_modeContainer.HasAKModeChanged();  // If this hotkey invocation did not enter a scope then exit the scope.
            if (wasDeactivated || wasHotkeyInvocation)
            {
                IFC_RETURN(ExitScope(wasActive));
            }

            return S_OK;
        }

        // Called by CFocusManager
        _Check_return_ HRESULT UpdateScope()
        {
            const bool isActive = m_modeContainer.GetIsActive();
            if (!isActive)
            {
                return S_OK;
            }

            return UpdateScopeImpl(isActive, GetFocusedElementNoRef());
        }

        _Check_return_ HRESULT EnterScope()
        {
            return UpdateScopeImpl(true, GetFocusedElementNoRef());
        }

        _Check_return_ HRESULT ExitScope(_In_ const bool isActive)
        {
            AK_TRACE(L"AK> ExitScope\n");
            if (m_current)
            {
                if (isActive)
                {
                    IFC_RETURN(m_current->HideAccessKeys());
                }
                m_current.reset();
            }

            return S_OK;
        }

        void SetFocusManager(_In_ CFocusManager* pFocusManager)
        {
            m_pFocusManager = pFocusManager;
        }

        _Check_return_ HRESULT AddElement(_In_ Element* const element)
        {
            ASSERT(m_modeContainer.GetIsActive());

            std::shared_ptr<Scope> currentScope = m_current; 

            if (currentScope && m_treeAnalyzer.IsValidAKElement(element))
            {
                Element* const owner = m_treeAnalyzer.GetScopeOwner(element);
                Element* const scopeParent = currentScope->GetScopeParent().lock_noref();

                if (currentScope->ShouldElementEnteringTreeUpdateScope(owner))
                {
                    //For us to have reached this code path means we have to be in AK mode, so it
                    //is safe for us to pass in true
                    IFC_RETURN(UpdateScopeImpl(true, element));
                }
                else if (owner == scopeParent)
                {
                    IFC_RETURN(currentScope->AddToAccessKeyOwner(element));
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT RemoveElement(_In_ Element* const element)
        {
            ASSERT(m_modeContainer.GetIsActive());

            std::shared_ptr<Scope> currentScope = m_current;

            if (currentScope && m_treeAnalyzer.IsAccessKey(element))
            {
                Element* const owner = m_treeAnalyzer.GetScopeOwner(element);
                Element* const scopeParent = currentScope->GetScopeParent().lock_noref();

                if (owner == scopeParent)
                {
                    IFC_RETURN(currentScope->RemoveFromAccessKeyOwner(element));
                }
                //there could be the situation where the scope owner is being removed. In that case, we
                //should update the entire scope
                else if (element == scopeParent && scopeParent && m_treeAnalyzer.IsValidAKElement(scopeParent))
                {
                    IFC_RETURN(UpdateScopeImpl(true, scopeParent));
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT OnIsEnabledChanged(_In_ Element* const element, bool isEnabled)
        {
            std::shared_ptr<Scope> currentScope = m_current;

            if (currentScope && m_treeAnalyzer.IsAccessKey(element))
            {
                if (isEnabled)
                {
                    IFC_RETURN(AddElement(element));
                }
                else
                {
                    IFC_RETURN(RemoveElement(element));
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT OnVisibilityChanged(_In_ Element* const element, const DirectUI::Visibility& visibility)
        {
            std::shared_ptr<Scope> currentScope = m_current;

            if (currentScope  && m_treeAnalyzer.IsAccessKey(element))
            {
                if (visibility == DirectUI::Visibility::Visible)
                {
                    IFC_RETURN(AddElement(element));
                }
                else if (visibility == DirectUI::Visibility::Collapsed)
                {
                    IFC_RETURN(RemoveElement(element));
                }
            }

            return S_OK;
        }

    private:

        _Check_return_ HRESULT UpdateScopeImpl(_In_ bool isActive, _In_ Element* scopeElement)
        {
            Element* pNewOwner = nullptr;

            // If this is a hotkey invocation then isActive == false.  
            // This will cause the root scope to be entered in the call to EnterScope at the end of the method
            if (isActive)
            {

                pNewOwner = scopeElement ? m_treeAnalyzer.GetScopeOwner(scopeElement) : nullptr;
                if (m_current)
                {
                    auto pOldOwner = m_current->GetScopeParent().lock();
                    if (pNewOwner == pOldOwner.get())
                    {
                        return S_OK;
                    }
                }
            }

            return EnterScope(pNewOwner, isActive);
        }

        Element* GetFocusedElementNoRef()
        {
            Element* pFocusedElement = nullptr;
            if (m_pFocusManager)
            {
                pFocusedElement = m_pFocusManager->GetFocusedElementNoRef();
            }
            return pFocusedElement;
        }

        _Check_return_ HRESULT ProcessEscapeKey(_Out_ bool* wasInvoked)
        {
            std::shared_ptr<Scope> current = m_current;
            *wasInvoked = false;

            if (current)
            {
                // If we are filtering scope owners, back off one letter.  
                // If we are in the root scope (GetScope on the parent) returns nullptr, then exit mnemonics mode.
                // If a scope has no defined parent (this is set at construction), or if the parent is part of the root scope leave mnemonics mode.
                // Otherwise, we will attempt to 'pop' the scope by entering the scope of the parent element
                if (current->IsScopeFilteringInput())
                {
                    IFC_RETURN(current->ProcessEscapeKey());
                }
                else
                {
                    IFC_RETURN(BackOutToNextValidParentScope(current.get()));
                }
            }

            return S_OK;
        }

        // Walk up the scope parents to find the closest valid scope.  Enter that scope.
        // Exit AccessKey DisplayMode if no valid ancestor scope is found.  The caller is responsible for
        // calling ExitScope in that case (ProcessCharacter will do this).
        _Check_return_ HRESULT BackOutToNextValidParentScope(_In_ Scope* initialScope)
        {
            // In the past, we only called UpdateScopeImpl here when IsValidAKElement() returned true from
            // GetScopeParent().  But this resulted in some situations where the user gets stuck in a scope and can't
            // back out. Instead, we walk up the scope parent tree until we find a valid parent scope we can back
            // up into.
            int triesLeft = 100;
            Element* scopeParent = initialScope->GetScopeParent().lock();
            while (scopeParent)
            {
                IFC_RETURN(UpdateScopeImpl(m_modeContainer.GetIsActive(), scopeParent));
                const bool didScopeChange = (initialScope != m_current.get());
                if (didScopeChange)
                {
                    // We successfully entered a new valid scope.  All done.
                    return S_OK;
                }
                
                // The scope will be unchanged here if the AK scope that contains "scopeParent" doesn't have any AccessKeys.
                // If the scope is unchanged, back it out again.
                scopeParent = m_treeAnalyzer.GetScopeOwner(scopeParent);

                // If we hit this failfast, it means the scope tree is 100 levels deep.  It's more likely we hit some
                // kind of cycle in the logic to walk the scope tree, so we just failfast.  Better to crash than spin.
                FAIL_FAST_ASSERT(triesLeft-- != 0);
            }

            // We walked all the way up without finding any valid scopes.  Exit AccessKey DisplayMode.
            m_modeContainer.SetIsActive(false);
            
            return S_OK;
        }

        _Check_return_ HRESULT ConstructScope(_In_opt_ Element* e, _Inout_ std::shared_ptr<Scope>& newScope)
        {
            TraceAccessKeyScopeBuilderConstructScopeBegin();
            HRESULT hr = m_scopeBuilder.ConstructScope(e, newScope);
            TraceAccessKeyScopeBuilderConstructScopeEnd();
            return hr;
        }

        _Check_return_ HRESULT ProcessNormalKey(_In_ const wchar_t character, _In_ const bool wasActive, _Out_ bool* wasInvoked)
        {
            // Invoke can be reentrant, we need to protect m_current by having our own reference
            std::shared_ptr<Scope> current = m_current;
            if (current)
            {
                // If an AccessKeyOwner was found and Invoked or if there was partial matching, wasInvoked <- true
                //
                // In the case where ProcessNormalKey is called in HotKey mode, wasActive will be false.  Passing this into the scope will supress
                // the partial matching feature.
                AKInvokeReturnParams<Element> invokeResult;
                IFC_RETURN(current->Invoke(character, wasActive /* allow partial match filtering in the scope */, &invokeResult));

                *wasInvoked = invokeResult.invokeAttempted;

                xref_ptr<Element> invokedElement = invokeResult.invokedElement.lock();

                // If the AKO invoked is a scope owner, Call update scope to change scope to that one
                // If the invoked element is nullptr, don't change to this scope becasue it's root scope
                // Allowing a navigation into root scope would allow for scope cycles to form.  
                if (invokedElement != nullptr)
                {
                    // We successfully found an element to be invoked, but it failed to find a valid pattern. As a result, we will give focus to the element
                    if (!invokeResult.invokeFoundValidPattern)
                    {
                        if (FocusProperties::IsFocusable(invokedElement.get()))
                        {
                            const Focus::FocusMovementResult result = m_pFocusManager->SetFocusedElement(
                                Focus::FocusMovement(
                                    invokedElement,
                                    DirectUI::FocusNavigationDirection::None,
                                    DirectUI::FocusState::Keyboard));
                            IFC_RETURN(result.GetHResult());
                        }
                    }

                    if (m_treeAnalyzer.IsScopeOwner(invokedElement))
                    {
                        // This is the case that a hotkey invokes a scope owner - we need to set AK mode active to prevent the scope from going stale.
                        // The invoke handles entering the scope.
                        if (!wasActive)
                        {
                            m_modeContainer.SetIsActive(true);
                        }
                        IFC_RETURN(EnterScope(invokedElement.get(), wasActive));
                    }
                    // If the AKO invoked has DismissAccessKeyOnInvoke set to true, exit AK mode now
                    // Intentionally not allowing a navigation to also dismiss AK mode.
                    else if (DismissOnInvoked<Element>(invokedElement))
                    {
                        IFC_RETURN(m_modeContainer.SetIsActive(false));  // Note this will propogate responsibility of exiting the scope to ScopeTree::ProcessCharacter
                    }
                    else if(wasActive) // If this was not a hotkey invoke, e.g. wasActive == false...
                    {
                        IFC_RETURN(m_current->ShowAccessKeys()); // If not dismissing on invoke, then let's refresh the visuals if AKMode was active.
                    }
                }

            }
            return S_OK;
        }

        _Check_return_ HRESULT EnterScope(_In_opt_ Element* element, _In_ const bool isActive)
        {
            AK_TRACE(L"AK> EnterScope %p\n", element);

            std::shared_ptr<Scope> newScope;
            IFC_RETURN(ConstructScope(element, newScope));

            //We only want to change the current scope if the creation of the new scope was valid
            if (newScope)
            {
                IFC_RETURN(ExitScope(isActive));
                m_current = newScope;
                if (isActive)
                {
                    IFC_RETURN(m_current->ShowAccessKeys());
                }
            }

            return S_OK;
        }

        bool IsRootScope() const
        {
            xref::weakref_ptr<Element> scopeSeed = m_current->GetScopeParent();

            // If the seed element is nullptr, then this is a root scope and we should exit
            return scopeSeed.lock_noref() == nullptr;
        }

    private:
        ScopeBuilder& m_scopeBuilder;
        TreeAnalyzer& m_treeAnalyzer;
        std::shared_ptr<Scope> m_current;
        CFocusManager* m_pFocusManager;
        ModeContainer& m_modeContainer;
    };

}
