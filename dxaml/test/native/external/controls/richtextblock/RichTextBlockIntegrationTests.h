// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <Versioning.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace RichTextBlock {

    class RichTextBlockIntegrationTests : public WEX::TestClass<RichTextBlockIntegrationTests>
    {
    public:
        BEGIN_TEST_CLASS(RichTextBlockIntegrationTests)
            TEST_CLASS_PROPERTY(L"BinaryUnderTest", L"Microsoft.UI.Xaml.dll")
            TEST_CLASS_PROPERTY(L"Classification", L"Integration")
            TEST_CLASS_PROPERTY(L"RunAs", L"UAP")
            TEST_CLASS_PROPERTY(L"__ExecutionUnit", L"a62e3c8d-69d4-44de-95b5-a62be5062286;57e0de30-efb3-4001-9ccc-b38032fd1974;cbb6c59f-3ce2-4ed3-8eaa-f598566c2755")
        END_TEST_CLASS()

        TEST_CLASS_SETUP(ClassSetup)
        TEST_METHOD_SETUP(TestSetup)
        TEST_METHOD_CLEANUP(TestCleanup)

        BEGIN_TEST_METHOD(VerifySelectingTextWithTouchShowsSelectionFlyout)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that selecting text with touch shows the selection flyout.")
            TEST_METHOD_PROPERTY(L"Hosting:Mode", L"WPF")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectionStartAndEndTrackSelection)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that SelectionStart and SelectionEnd report the normalized selection edges and are null while selection is disabled.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyGetPositionFromPointMapsToCharacterEdges)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that GetPositionFromPoint maps points to the closest character edge, including line ends and points below the last line.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBaselineOffsetReflectsPaddingAndFontSize)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that BaselineOffset includes the top padding and grows with the font size.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyDisconnectChildrenRecursiveClearsRichTextBlockContent)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that VisualTreeHelper.DisconnectChildrenRecursive clears the RichTextBlock blocks, nested inlines and embedded elements only inside the disconnected subtree.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifySelectNormalizesToClusterBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that Select never splits a surrogate pair or a base character and its combining mark.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCharacterRectsFollowBidiVisualOrder)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that character rectangles of a right-to-left run inside a left-to-right paragraph follow the visual order, also when the run wraps.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyBoundingRectanglesForRangesCrossingBidiRuns)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies the text pattern bounding rectangles of ranges that cross between left-to-right and nested right-to-left runs.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyWrapWholeWordsKeepsOverlongWordOnOneLine)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that WrapWholeWords keeps a word wider than the available width on one line while Wrap breaks it.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyCharacterRectOfEmptyParagraph)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that an empty paragraph has a character rectangle on its own line at the content origin.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyWordNavigationAcrossInlineElementBoundaries)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies that text pattern word navigation neither splits nor merges words at Bold, Italic and InlineUIContainer boundaries.")
        END_TEST_METHOD()

        BEGIN_TEST_METHOD(VerifyRichTextBlockPropertiesRoundTripThroughProjection)
            TEST_METHOD_PROPERTY(L"Description", L"Verifies RichTextBlock property defaults, that each property round-trips through its dependency property, and that the dependency property identifiers are distinct.")
        END_TEST_METHOD()
    };

} } } } } }
