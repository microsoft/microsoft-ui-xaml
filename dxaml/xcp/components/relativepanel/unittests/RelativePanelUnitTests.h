// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <WexTestClass.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

class RelativePanelUnitTests : public WEX::TestClass<RelativePanelUnitTests>
{
public:
    BEGIN_TEST_CLASS(RelativePanelUnitTests)
        TEST_METHOD_PROPERTY(L"Classification", L"Integration")
        TEST_METHOD_PROPERTY(L"TestPass:IncludeOnlyOn", L"Desktop")
    END_TEST_CLASS()

    BEGIN_TEST_METHOD(VerifyHorizontalDependencyResolution)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a horizontal dependency is resolved before trying to resolve the dependant node.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyVerticalDependencyResolution)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a vertical dependency is resolved before trying to resolve the dependant node.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyDownwardPrecedence)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that constraint precedence is respected in respect of AlignTopWithPanel vs. AlignTopWith vs. AlignVerticalCenterWith vs. Below vs. AlignVerticalCenterWithPanel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyUpwardPrecedence)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that constraint precedence is respected in respect of AlignBottomWithPanel vs. AlignBottomWith vs. AlignVerticalCenterWith vs. Above vs. AlignVerticalCenterWithPanel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyRightwardPrecedence)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that constraint precedence is respected in respect of AlignLeftWithPanel vs. AlignLeftWith vs. AlignHorizontalCenterWith vs. RightOf vs. AlignHorizontalCenterWithPanel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyLeftwardPrecedence)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that constraint precedence is respected in respect of AlignRightWithPanel vs. AlignRightWith vs. AlignHorizontalCenterWith vs. LeftOf vs. AlignHorizontalCenterWithPanel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyVerticallyFloatingAndOverlapping)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that floating elements (i.e. elements with no sibling-dependencies) can be aligned vertically inside the panel and overlap.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyHorizontallyFloatingAndOverlapping)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that floating elements (i.e. elements with no sibling-dependencies) can be aligned horizontally inside the panel and overlap.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyAboveAndOutOfPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element is vertically 'collapsed' when it is positioned above a sibling that is aligned to the top of the panel. ")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyBelowAndOutOfPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element is vertically 'collapsed' when it is positioned below a sibling that is aligned to the bottom of the panel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyLeftAndOutOfPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element is horizontally 'collapsed' when it is positioned to the left of a sibling that is aligned to the left of the panel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyRightAndOutOfPanel)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element is horizontally 'collapsed' when it is positioned to the right of a sibling that is aligned to the right of the panel.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyVerticalMultiDirectionalLeaf)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be 'sandwiched' by simulataneously positioning it below a sibling and above another sibling.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyHorizontalMultiDirectionalLeaf)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be 'sandwiched' by simulataneously positioning it to the left of a sibling and to the right of another sibling.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyAboveVerticallyCenteredRoot)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be positioned above a sibling that is vertically centered, causing the panel to expand upward and downward simulatenously.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyBelowVerticallyCenteredRoot)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be positioned below a sibling that is vertically centered, causing the panel to expand upward and downward simulatenously.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyLeftOfHorizontallyCenteredRoot)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be positioned to the left of a sibling that is horizontally centered, causing the panel to expand leftward and rightward simulatenously.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyRightOfHorizontallyCenteredRoot)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be positioned to the right of a sibling that is horizontally centered, causing the panel to expand leftward and rightward simulatenously.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyVerticallyCenteredChains)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a chain of elements attached above or below a vertically centered root can correctly expand the panel up and down.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyHorizontallyCenteredChains)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a chain of elements attached to the left or to the right of a horizontally centered root can correctly expand the panel left and right.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifySimultaneousTopAndBottomAlignment)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be simultaneously aligned to the top and bottom of a sibling (or siblings) and be correctly constrained by its (or their) total height.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifySimultaneousLeftAndRightAlignment)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be simultaneously aligned to the left and right of a sibling (or siblings) and be correctly constrained by its (or their) total width.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifySimultaneousTopAndBottomAlignmentToChains)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be simultaneously aligned to the top and bottom of a chain (or chains).")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifySimultaneousLeftAndRightAlignmentToChains)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that an element can be simultaneously aligned to the left and right of a chain (or chains).")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyChainWithHorizontallyCenteredSiblings)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a chain of elements containing siblings horizontally centered with respect to each other can correctly expand the panel left and right.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyChainWithVerticallyCenteredSiblings)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a chain of elements containing siblings vertically centered with respect to each other can correctly expand the panel up and down.")
    END_TEST_METHOD()

    BEGIN_TEST_METHOD(VerifyPhysicallyImpossibleDefinitions)
        TEST_METHOD_PROPERTY(L"Description", L"Verifies that a RelativePanel does not fail even if contraints specify a definition that is physically impossible to arrange.")
    END_TEST_METHOD()
};

}}}}}
