// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "BubblingUnitTests.h"
#include "Bubbling.h"

#include <UIElement.h>
#include <MockDependencyProperty.h>
#include <Type.h>

#include "XYFocusMocks.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml { namespace Focus { namespace XYFocus {

        using namespace ::Focus::XYFocusPrivate;
        using namespace Microsoft::UI::Xaml::Tests;

        void BubblingUnitTests::VerifyXYFocusPropertyRetrieval()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            auto elementLeft = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto elementRight = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto elementUp = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto elementDown = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            Metadata::MockDependencyProperty dpLeft(KnownPropertyIndex::UIElement_XYFocusLeft);
            Metadata::MockDependencyProperty dpRight(KnownPropertyIndex::UIElement_XYFocusRight);
            Metadata::MockDependencyProperty dpUp(KnownPropertyIndex::UIElement_XYFocusUp);
            Metadata::MockDependencyProperty dpDown(KnownPropertyIndex::UIElement_XYFocusDown);

            CValue value;

            value.WrapObjectNoRef(elementLeft);
            VERIFY_SUCCEEDED(element->SetValue(&dpLeft, value));

            value.WrapObjectNoRef(elementRight);
            VERIFY_SUCCEEDED(element->SetValue(&dpRight, value));

            value.WrapObjectNoRef(elementUp);
            VERIFY_SUCCEEDED(element->SetValue(&dpUp, value));

            value.WrapObjectNoRef(elementDown);
            VERIFY_SUCCEEDED(element->SetValue(&dpDown, value));

            CDependencyObject* retirevedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Left);
            VERIFY_ARE_EQUAL(retirevedElement, elementLeft);

            retirevedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(retirevedElement, elementRight);

            retirevedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Up);
            VERIFY_ARE_EQUAL(retirevedElement, elementUp);

            retirevedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Down);
            VERIFY_ARE_EQUAL(retirevedElement, elementDown);
        }

        void BubblingUnitTests::VerifyNullWhenXYFocusPropertyRetrievalFailed()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            CDependencyObject* retirevedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Left);
            VERIFY_IS_NULL(retirevedElement);
        }

        void BubblingUnitTests::VerifyCorrectOverrideChosenWhenTargetElementHasOverride()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto candidate = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto parent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto directionOverrideOfParent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto overrideElement = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            VERIFY_SUCCEEDED(element->AddParent(parent));

            Metadata::MockDependencyProperty dp(KnownPropertyIndex::UIElement_XYFocusRight);

            CValue value;
            value.WrapObjectNoRef(directionOverrideOfParent);
            VERIFY_SUCCEEDED(parent->SetValue(&dp, value));

            value.WrapObjectNoRef(overrideElement);
            VERIFY_SUCCEEDED(element->SetValue(&dp, value));

            CDependencyObject* retrievedElement = TryXYFocusBubble(element, candidate, nullptr, DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(retrievedElement, overrideElement);
        }

        void BubblingUnitTests::VerifyCorrectOverrideChosenWhenBubbling()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto candidate = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto parent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto directionOverrideOfParent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            VERIFY_SUCCEEDED(element->AddParent(parent));

            Metadata::MockDependencyProperty dp(KnownPropertyIndex::UIElement_XYFocusRight);

            CValue value;
            value.WrapObjectNoRef(directionOverrideOfParent);
            VERIFY_SUCCEEDED(parent->SetValue(&dp, value));

            CDependencyObject* retrievedElement = TryXYFocusBubble(element, candidate, nullptr, DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(retrievedElement, directionOverrideOfParent);
        }

        void BubblingUnitTests::VerifyCandidateChosenWhenDescendant()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto candidate = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto parent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto directionOverrideOfParent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            VERIFY_SUCCEEDED(element->AddParent(parent));
            VERIFY_SUCCEEDED(candidate->AddParent(parent));

            Metadata::MockDependencyProperty dp(KnownPropertyIndex::UIElement_XYFocusRight);

            CValue value;
            value.WrapObjectNoRef(directionOverrideOfParent);
            VERIFY_SUCCEEDED(parent->SetValue(&dp, value));

            CDependencyObject* retrievedElement = TryXYFocusBubble(element, candidate, nullptr, DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(retrievedElement, candidate);
        }

        void BubblingUnitTests::VerifyNullWhenCandidateNull()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::Control);
            CDependencyObject* retrievedElement = TryXYFocusBubble(element, nullptr, nullptr, DirectUI::FocusNavigationDirection::Right);
            VERIFY_IS_NULL(retrievedElement);
        }

        void BubblingUnitTests::VerifyOverrideAncestorOfSearchRoot()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto candidate = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto parent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto directionOverrideOfParent = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto overrideElement = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto searchRoot = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);

            VERIFY_SUCCEEDED(element->AddParent(parent));

            Metadata::MockDependencyProperty dp(KnownPropertyIndex::UIElement_XYFocusRight);

            CValue value;
            value.WrapObjectNoRef(directionOverrideOfParent);
            VERIFY_SUCCEEDED(parent->SetValue(&dp, value));

            value.WrapObjectNoRef(overrideElement);
            VERIFY_SUCCEEDED(element->SetValue(&dp, value));

            CDependencyObject* retrievedElement = TryXYFocusBubble(element, candidate, searchRoot, DirectUI::FocusNavigationDirection::Right);
            VERIFY_ARE_EQUAL(retrievedElement, candidate);
        }

        void BubblingUnitTests::VerifyNonFocusableDirectionOverrideChosen()
        {
            auto element = make_xref<FocusableXYFocusCUIElement>(KnownTypeIndex::UIElement);
            auto elementLeft = make_xref<XYFocusCUIElement>(KnownTypeIndex::UIElement);

            Metadata::MockDependencyProperty dpLeft(KnownPropertyIndex::UIElement_XYFocusLeft);

            CValue value;

            value.WrapObjectNoRef(elementLeft);
            VERIFY_SUCCEEDED(element->SetValue(&dpLeft, value));

            CDependencyObject* retrievedElement = GetDirectionOverride(element, nullptr, DirectUI::FocusNavigationDirection::Left, true);
            VERIFY_ARE_EQUAL(retrievedElement, elementLeft);
        }
    }}}
}}}}