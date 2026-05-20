// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "VisualTransitionTableOptimizedLookupUnitTests.h"
#include <VisualTransitionTableOptimizedLookup.h>
#include <VisualStateGroupCollectionCustomRuntimeData.h>
#include <StreamOffsetToken.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        std::array<std::pair<const wchar_t*, unsigned int>, 3> s_stateMap =
        {
            std::make_pair(L"State0", 0),
            std::make_pair(L"State1", 1),
            std::make_pair(L"State2", 2),
        };

        DECLARE_CONST_STRING_IN_TEST_CODE(s_state0x, L"State0");
        DECLARE_CONST_STRING_IN_TEST_CODE(s_state1x, L"State1");
        DECLARE_CONST_STRING_IN_TEST_CODE(s_state2x, L"State2");

        bool MockTryGetIndexFromVisualStateName(const wchar_t* name, unsigned int* index)
        {
            for (auto& pair : s_stateMap)
            {
                if (name && wcscmp(name, pair.first) == 0)
                {
                    *index = pair.second;
                    return true;
                }
            }
            return false;
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateEmptyLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> emptyTable;
            lookup.BuildLookupTable(emptyTable);

            unsigned int transitionIndex = 0;
            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateDefaultGroupLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(2);

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(2, 1, transitions[1]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 0);

            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(1, 0, 1, &transitionIndex));

            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(2, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateToOnlyLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(2);

            transitions[0].m_toState = s_state0x;
            transitions[1].m_toState = s_state1x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(0, 1, transitions[1]);
            
            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);

            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 1, 0, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 0);

            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(1, 0, 2, &transitionIndex));
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateFromOnlyLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(2);

            transitions[0].m_fromState = s_state0x;
            transitions[1].m_fromState = s_state1x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(0, 1, transitions[1]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 0);

            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 1, 0, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);

            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(1, 2, 1, &transitionIndex));
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateBothLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(3);

            transitions[0].m_fromState = s_state0x;
            transitions[0].m_toState = s_state1x;

            transitions[1].m_fromState = s_state0x;
            transitions[1].m_toState = s_state2x;

            transitions[2].m_fromState = s_state1x;
            transitions[2].m_toState = s_state2x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(0, 1, transitions[1]);
            lookup.RecordTransitionGroup(0, 2, transitions[2]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 0);

            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 2, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);

            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 1, 2, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 2);

            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(1, 2, 1, &transitionIndex));
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateExplicitExclusionLookup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(1);

            transitions[0].m_fromState = s_state0x;
            transitions[0].m_toState = s_state1x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_FALSE(lookup.TryGetVisualTransitionIndex(0, 0, 2, &transitionIndex));
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateFallbackOnGroup()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(2);

            transitions[0].m_fromState = s_state0x;
            transitions[0].m_toState = s_state1x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(0, 1, transitions[1]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 2, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);
        }

        void VisualTransitionTableOptimizedLookupUnitTests::ValidateScoringSystem()
        {
            VisualTransitionTableOptimizedLookup lookup(&MockTryGetIndexFromVisualStateName);
            std::vector<VisualTransitionEssence> transitions;
            transitions.resize(2);

            transitions[0].m_fromState = s_state0x;
            transitions[1].m_toState = s_state1x;

            lookup.RecordTransitionGroup(0, 0, transitions[0]);
            lookup.RecordTransitionGroup(0, 1, transitions[1]);

            lookup.BuildLookupTable(transitions);

            unsigned int transitionIndex = 0;
            VERIFY_IS_TRUE(lookup.TryGetVisualTransitionIndex(0, 0, 1, &transitionIndex));
            VERIFY_ARE_EQUAL(transitionIndex, 1);
        }

    }
} } } }
