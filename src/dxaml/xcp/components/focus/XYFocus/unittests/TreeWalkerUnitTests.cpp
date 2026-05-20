// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "TreeWalkerUnitTests.h"
#include <TreeWalker.h>

#include "XYFocusMocks.h"

using namespace DirectUI;
using namespace Focus::XYFocusPrivate;
using namespace Microsoft::UI::Xaml::Tests;


namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {

        bool TreeWalkerUnitTests::VerifyResult(
            _In_ std::vector<::Focus::XYFocus::XYFocusParams>& vector,
            _In_ std::vector<CDependencyObject*>& target)
        {
            std::vector<CDependencyObject*> uiElementList;

            for (const auto& param : vector)
            {
                uiElementList.push_back(param.element);
            }

            return std::equal(uiElementList.begin(), uiElementList.end(), target.begin());
        }

        void TreeWalkerUnitTests::VerifyFindElement()
        {
            auto root = make_xref<XYFocusCUIElement>();

            auto current = make_xref<FocusableXYFocusCUIElement>();
            auto candidate = make_xref<FocusableXYFocusCUIElement>();
 
            VERIFY_SUCCEEDED(root->AddChild(candidate));

            auto candidateList = FindElements(root, current, nullptr, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 1);
            VERIFY_ARE_EQUAL(candidateList.at(0).element, candidate);
        }

        void TreeWalkerUnitTests::VerifyFindElementIgnoresNonFocusableChildren()
        {
            auto root = make_xref<XYFocusCUIElement>();

            auto current = make_xref<FocusableXYFocusCUIElement>();

            auto candidate = make_xref<FocusableXYFocusCUIElement>();
            auto nonFocusableCandidate = make_xref<XYFocusCUIElement>();

            VERIFY_SUCCEEDED(root->AddChild(candidate));
            VERIFY_SUCCEEDED(root->AddChild(nonFocusableCandidate));

            auto candidateList = FindElements(root, current, nullptr, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 1);
            VERIFY_ARE_EQUAL(candidateList.at(0).element, candidate);
        }

        void TreeWalkerUnitTests::VerifyRecursiveSearchOfElements()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto subRoot = make_xref<XYFocusCUIElement>();

            auto current = make_xref<FocusableXYFocusCUIElement>();

            auto candidate = make_xref<FocusableXYFocusCUIElement>();
            auto candidateB = make_xref<FocusableXYFocusCUIElement>();
            auto candidateC = make_xref<FocusableXYFocusCUIElement>();

            std::vector<CDependencyObject*> targetList;
            targetList.push_back(candidate);
            targetList.push_back(candidateB);
            targetList.push_back(candidateC);

            VERIFY_SUCCEEDED(root->AddChild(candidate));
            VERIFY_SUCCEEDED(root->AddChild(subRoot));

            VERIFY_SUCCEEDED(subRoot->AddChild(candidateB));

            VERIFY_SUCCEEDED(candidateB->AddChild(candidateC));

            auto candidateList = FindElements(root, current, nullptr, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 3);
            VERIFY_IS_TRUE(VerifyResult(candidateList, targetList));
        }

        void TreeWalkerUnitTests::VerifyOnlyElementsWithinRootSelected()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto subRoot = make_xref<XYFocusCUIElement>();

            auto current = make_xref<FocusableXYFocusCUIElement>();

            auto candidate = make_xref<FocusableXYFocusCUIElement>();
            auto candidateB = make_xref<FocusableXYFocusCUIElement>();
            auto candidateC = make_xref<FocusableXYFocusCUIElement>();

            std::vector<CDependencyObject*> targetList;
            targetList.push_back(candidateB);
            targetList.push_back(candidateC);

            VERIFY_SUCCEEDED(root->AddChild(candidate));
            VERIFY_SUCCEEDED(root->AddChild(subRoot));

            VERIFY_SUCCEEDED(subRoot->AddChild(candidateB));

            VERIFY_SUCCEEDED(candidateB->AddChild(candidateC));

            auto candidateList = FindElements(subRoot, current, nullptr, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 2);
            VERIFY_IS_TRUE(VerifyResult(candidateList, targetList));
        }

        void TreeWalkerUnitTests::VerifyCurrentElementNotIncludedInList()
        {
            auto root = make_xref<XYFocusCUIElement>();

            auto current = make_xref<FocusableXYFocusCUIElement>();

            VERIFY_SUCCEEDED(root->AddChild(current));

            auto candidateList = FindElements(root, current, nullptr, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 0);
        }

        void TreeWalkerUnitTests::VerifyElementParticipatingInScrollAddedToList()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto scrollviewer = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);
            scrollviewer->RegisterAsScroller();

            auto current = make_xref<FocusableXYFocusCUIElement>();
            auto candidate = make_xref<FocusableXYFocusCUIElement>();

            VERIFY_SUCCEEDED(root->AddChild(scrollviewer));
            VERIFY_SUCCEEDED(scrollviewer->AddChild(candidate));

            auto candidateList = FindElements(root, current, scrollviewer, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 1);
            VERIFY_ARE_EQUAL(candidateList.at(0).element, candidate);
        }

        void TreeWalkerUnitTests::VerifyElementPartOfNestedScrollingScrollviewerAddedToList()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto scrollviewer = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);
            auto scrollviewerB = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);

            scrollviewer->RegisterAsScroller();
            scrollviewerB->RegisterAsScroller();

            auto current = make_xref<FocusableXYFocusCUIElement>();
            auto candidate = make_xref<FocusableXYFocusCUIElement>();
            Expect(*candidate, IsOccluded)
                .ReturnValue(true);

            VERIFY_SUCCEEDED(root->AddChild(scrollviewer));
            VERIFY_SUCCEEDED(scrollviewer->AddChild(scrollviewerB));
            VERIFY_SUCCEEDED(scrollviewer->AddChild(candidate));

            auto candidateList = FindElements(root, current, scrollviewerB, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 1);
            VERIFY_ARE_EQUAL(candidateList.at(0).element, candidate);
        }

        void TreeWalkerUnitTests::VerifyElementInNonActiveScrollviewerAddedToList()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto scrollviewer = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);
            auto scrollviewerB = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);

            scrollviewer->RegisterAsScroller();
            scrollviewerB->RegisterAsScroller();

            auto current = make_xref<FocusableXYFocusCUIElement>();
            auto candidate = make_xref<FocusableXYFocusCUIElement>();

            VERIFY_SUCCEEDED(root->AddChild(scrollviewer));
            VERIFY_SUCCEEDED(scrollviewer->AddChild(candidate));
            Expect(*candidate, IsOccluded)
                .ReturnValue(false);

            auto candidateList = FindElements(root, current, scrollviewerB, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 1);
            VERIFY_ARE_EQUAL(candidateList.at(0).element, candidate);
        }

        void TreeWalkerUnitTests::VerifyOccludedElementInNonActiveScrollviewerNotAddedToList()
        {
            auto root = make_xref<XYFocusCUIElement>();
            auto scrollviewer = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);
            auto scrollviewerB = make_xref<XYFocusCUIElement>(KnownTypeIndex::ScrollViewer);

            scrollviewer->RegisterAsScroller();
            scrollviewerB->RegisterAsScroller();

            auto current = make_xref<FocusableXYFocusCUIElement>();
            auto candidate = make_xref<FocusableXYFocusCUIElement>();
            Expect(*candidate, IsOccluded)
                .ReturnValue(true);

            VERIFY_SUCCEEDED(root->AddChild(scrollviewer));
            VERIFY_SUCCEEDED(scrollviewer->AddChild(candidate));
            
            auto candidateList = FindElements(root, current, scrollviewerB, true, false);
            VERIFY_IS_TRUE(candidateList.size() == 0);
        }
    }}}
}}}}