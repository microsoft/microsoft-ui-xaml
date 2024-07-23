// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AKCommon.h"

#include "paltypes.h"
#include <DependencyLocator.h>

namespace AccessKeys {

    template<class ModeContainer=AKModeContainer, class ScopeTree=AKScopeTree, class TreeAnalyzer=AKTreeAnalyzer>

    class AKInputInterceptor
    {
    public:
        AKInputInterceptor(ModeContainer& modeContainer, ScopeTree& scopeTree, TreeAnalyzer& treeAnalyzer) : m_modeContainer(modeContainer), m_scopeTree(scopeTree), m_treeAnalyzer(treeAnalyzer) {}

        //This is the entry way into access keys. This method takes the InputMessage and funnels the necessary information
        //that the AK system needs to build and invoke the scope
        //We then figure out whether we can enter AK mode. If we are successful, then we have handled the message and entered AK mode,
        //meaning that the system should not continue to process this message. If we were not successful, then return false and continue
        //processing this message
        HRESULT TryProcessInputForAccessKey(_In_ const InputMessage* const inputMessage, _Out_ bool* keyProcessed)
        {
            (*keyProcessed) = false;

            IFC_RETURN(TryProcessKeyImpl(inputMessage, keyProcessed));

            return S_OK;
        }

        HRESULT TryProcessInputForCharacterReceived(_In_ mui::ICharacterReceivedEventArgs* args, _Out_ bool* keyProcessed)
        {
            *keyProcessed = false;
            UINT32 keyCode;

            if (m_modeContainer.GetIsActive())
            {
                IFC_RETURN(args->get_KeyCode(&keyCode));
                if (keyCode != wsy::VirtualKey::VirtualKey_Escape) //We handle Escape key on Keydown, not CharacterReceived
                {
                    InputMessage message;
                    message.m_msgID = XCP_CHAR;
                    // Caution: Abuse of parameter, this represents a character (wchar_t) when msgID is XCP_Char.  It's usually an actual VKey
                    message.m_platformKeyCode = static_cast<wsy::VirtualKey>(keyCode);

                    IFC_RETURN(TryProcessKeyImpl(&message, keyProcessed));
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT ProcessPointerInput(_In_ const InputMessage* const inputMessage)
        {
            const bool isActive = m_modeContainer.GetIsActive();
            if (isActive &&
                (inputMessage->m_msgID == XCP_POINTERDOWN ||
                 inputMessage->m_msgID == XCP_POINTERUP ||
                 inputMessage->m_msgID == XCP_POINTERWHEELCHANGED))
            {
                IFC_RETURN(m_scopeTree.ExitScope(isActive));
                IFC_RETURN(m_modeContainer.SetIsActive(false));
            }

            return S_OK;
        }

    private:
        HRESULT TryProcessKeyImpl(_In_ const InputMessage* const inputMessage, _Out_ bool* keyProcessed)
        {
            AK_TRACE(L"AK> TryProcessKeyImpl %x\n", inputMessage->m_platformKeyCode);

            *keyProcessed = false;
            bool shouldEvaluate = false;

            //If we are attempting to entering AK mode, we need to scan the visual tree to verify that we have
            // access key set anywhere in the entire xaml visual tree (all visual roots included)
            if (!m_modeContainer.GetIsActive() &&
                (IsLeftAltKey(inputMessage) || IsMenuKeyDown(inputMessage)))
            {
                bool shouldActivate = false;

                IFC_RETURN(m_treeAnalyzer.DoesTreeContainAKElement(shouldActivate));

                if (!shouldActivate)
                { 
                    AK_TRACE(L"AK> TryProcessKeyImpl: AccessKey mode not activated because there are no AccessKeys in root scope.\n");
                    return S_OK;
                }
            }

            //We ask the Listener to reevaluate what mode we should be on based on whether alt was
            //pressed during a keydown and what the charactercode (unsigned int) is.
            IFC_RETURN(m_modeContainer.EvaluateAccessKeyMode(inputMessage, &shouldEvaluate));

            //We only want to process this character code if we are in AK mode
            if (shouldEvaluate)
            {
                //Send the character code to the scope tree in order to start building the scopes.
                IFC_RETURN(m_scopeTree.ProcessCharacter((wchar_t)inputMessage->m_platformKeyCode, keyProcessed));
            }

            *keyProcessed = ShouldMarkHandled(*keyProcessed, inputMessage);
            return S_OK;
        }

        bool ShouldMarkHandled(bool handled, const InputMessage* const message)
        {
            return !IsInExcludeList(message) &&
                (handled || m_modeContainer.GetIsActive()) && message->m_platformKeyCode != wsy::VirtualKey::VirtualKey_Escape;
        }

        bool IsInExcludeList(const InputMessage* message)
        {
            //If we want to force an exit from AK mode, then it means we received an input that should flow through ak and
            //be processed. This is captured through the ShouldForciblyExitAKMode. If this value is true, it means that modecontainer
            //contains an element it feels should be part of this exlusion list
            bool modeContainerExcludes = m_modeContainer.ShouldForciblyExitAKMode();

            return modeContainerExcludes;
        }

        ModeContainer& m_modeContainer;
        ScopeTree& m_scopeTree;
        TreeAnalyzer& m_treeAnalyzer;
    };
}