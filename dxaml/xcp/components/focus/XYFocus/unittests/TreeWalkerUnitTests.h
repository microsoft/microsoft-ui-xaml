// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WexTestClass.h"
#include "XYfocus.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {
        class TreeWalkerUnitTests : public WEX::TestClass<TreeWalkerUnitTests>
        {
        public:
            BEGIN_TEST_CLASS(TreeWalkerUnitTests)
                TEST_CLASS_PROPERTY(L"Classification", L"Integration")
                TEST_CLASS_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
            END_TEST_CLASS()

            BEGIN_TEST_METHOD(VerifyFindElement)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we get the focusable child from the start root")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyFindElementIgnoresNonFocusableChildren)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we skip children that are not focusable")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyRecursiveSearchOfElements)
                TEST_METHOD_PROPERTY(L"Description", L"Verifies that we continue to walk through element's children looking for focusable elements")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOnlyElementsWithinRootSelected)
                TEST_METHOD_PROPERTY(L"Description", L"Even if there are other focusable parents outside the scope of the root, we do not choose them")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyCurrentElementNotIncludedInList)
                TEST_METHOD_PROPERTY(L"Description", L"We should not include the current element in the candidate list")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyElementParticipatingInScrollAddedToList)
                TEST_METHOD_PROPERTY(L"Description", L"If an element is participating in scrolling, we should add it to the list")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyElementPartOfNestedScrollingScrollviewerAddedToList)
                TEST_METHOD_PROPERTY(L"Description", L"If an element is participating is not participating in scrolling, but has the potential to be scrolled by another scrollviewer, add it to the list")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyElementInNonActiveScrollviewerAddedToList)
                TEST_METHOD_PROPERTY(L"Description", L"If an element is part of a scrollviewer that is not the active scrollviewer, we should add it to list as long as it is not occluded")
            END_TEST_METHOD()

            BEGIN_TEST_METHOD(VerifyOccludedElementInNonActiveScrollviewerNotAddedToList)
                TEST_METHOD_PROPERTY(L"Description", L"If an occluded element is part of a scrollviewer that is not the active scrollviewer, we should not add it to list")
            END_TEST_METHOD()         

        private:
            bool VerifyResult(
                _In_ std::vector<::Focus::XYFocus::XYFocusParams>& vector, 
                _In_ std::vector<CDependencyObject*>& target);
        };
    }}}
}}}}
