// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {
        class BubblingUnitTests : public WEX::TestClass<BubblingUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(BubblingUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(VerifyXYFocusPropertyRetrieval)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we successfully detect an element that is completely inside the vision cone. Verifies all directions")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNullWhenXYFocusPropertyRetrievalFailed)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that when a direction override is not specified, we return null")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCorrectOverrideChosenWhenTargetElementHasOverride)
                TEST_METHOD_PROPERTY(L"Description", L"Verify that when we inpsect an element, we check if that element has an override before going to its parent")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCorrectOverrideChosenWhenBubbling)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we properly bubble and check if a parent has a direction override set")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCandidateChosenWhenDescendant)
                TEST_METHOD_PROPERTY(L"Description", L"When we have find an override root and the candidate is a descendant of that root, we should return the candidate")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNullWhenCandidateNull)
                TEST_METHOD_PROPERTY(L"Description", L"If a null candidate is passed in, we should return null")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOverrideAncestorOfSearchRoot)
                TEST_METHOD_PROPERTY(L"Description", L"When we have a search root, the overridden element should still be within the subtree of the search root.")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyNonFocusableDirectionOverrideChosen)
                TEST_METHOD_PROPERTY(L"Description", L"Return a direction override, even when it is not focusable")
            END_TEST_METHOD()
        };
    }}}
}}}}
