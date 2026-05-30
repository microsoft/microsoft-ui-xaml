// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Indexes.g.h>

#include "OcclusivityTesterUnitTests.h"
#include "OcclusivityTester.h"
#include <CxxMock.h>

using namespace CxxMock;

class CHitTestResults;

struct Base{};

MOCK_CLASS(MockElement, Base)
    MockElement(XRECTF rect) : m_rect({ rect.X, rect.Y, rect.X + rect.Width, rect.Y + rect.Height })
    {
        Expect(*this, GetGlobalBoundsLogical)
            .With(ne<XRECTF_RB*>(nullptr), eq<unsigned int>(FALSE), eq<bool>(false)) //Make sure we dont ignore clipping
            .SetOutValue<0>(m_rect)
            .ReturnValue(S_OK);
    }

    bool AllowFocusWhenDisabled()
    {
        return false;
    }

    bool IsEnabled()
    {
        return false;
    }
    
    template <KnownTypeIndex targetTypeIndex>
    bool OfTypeByIndex()
    {
        return OfTypeByIndex(targetTypeIndex);
    }

    bool OfTypeByIndex(KnownTypeIndex index)
    {
        if (index == KnownTypeIndex::UnknownType) { return true; } // This is to handle do_pointer_cast
        return GetTypeIndex() == index;
    }

    STUB_METHOD(HRESULT, GetGlobalBoundsLogical, 3(XRECTF_RB*, unsigned int, bool))
    STUB_METHOD(int, Release, 0)
    STUB_METHOD(MockElement*, GetTemplatedParent, 0)
    STUB_METHOD(KnownTypeIndex, GetTypeIndex, 0)

private:
    XRECTF_RB m_rect;
END_MOCK

MOCK_CLASS(CHitTestResults, Base)
    STUB_METHOD(HRESULT,GetAnswer,2(XUINT32*,MockElement***))
END_MOCK

class MockVisualRoot
{
    std::vector<MockElement*> hitElements;
public:
    MockVisualRoot() { }
    MockVisualRoot(MockElement* elements[], size_t n) : hitElements(elements, elements + n) { }

    MockElement** GetElements()
    {
        MockElement** ppElements = new MockElement*[hitElements.size()];

        MockElement** pElement = ppElements;
        for (auto& element : hitElements)
        {
            Expect(*element, Release)
                .Once()               // Make sure we Release the element once
                .ReturnValue(0);
            *(pElement++) = element;
        }

        return ppElements;
    }

    HRESULT HitTestEntry(const HitTestParams&, const HitTestPolygon&, BOOLEAN, BOOLEAN, BOOLEAN, CHitTestResults* hitResult)
    {
        Expect(*hitResult, GetAnswer)
            .Once()
            .SetOutValue<0>(static_cast<unsigned int>(hitElements.size()))
            .SetOutValue<1>(GetElements())
            .ReturnValue(S_OK);
        return S_OK;
    }
};

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Xaml {

        XRECTF largePopUpRect{ 0, 0, 100.0, 100.0};
        XRECTF buttonRect{ 20, 20, 80, 80 };

        void OcclusivityTesterUnitTests::ElementWithSmallerBoundsIsOccluded()
        {
            MockElement largePopup(largePopUpRect);
            MockElement button(buttonRect);
            MockElement* elements[] = { &largePopup, &button };
            MockVisualRoot root(elements, ARRAY_SIZE(elements));

            OcclusivityTester<MockVisualRoot, MockElement, MockElement> tester(&root);

            Expect(button, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(button, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            Expect(largePopup, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(largePopup, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            bool result = false;
            VERIFY_SUCCEEDED(tester.Test(&button, ToXRectFRB(buttonRect), &result));
            VERIFY_ARE_EQUAL(result, true);

            VERIFY_SUCCEEDED(tester.Test(&largePopup, ToXRectFRB(largePopUpRect), &result));
            VERIFY_ARE_EQUAL(result, false);
        }

        void OcclusivityTesterUnitTests::ElementOutsideRootIsNotOccluded()
        {
            MockElement button(buttonRect);
            MockVisualRoot root;

            Expect(button, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            OcclusivityTester<MockVisualRoot, MockElement, MockElement> tester(&root);

            bool result = false;
            VERIFY_SUCCEEDED(tester.Test(&button, ToXRectFRB(buttonRect), &result));
            VERIFY_ARE_EQUAL(result, false);
        }

        void OcclusivityTesterUnitTests::ScrollBarShouldBeIgnoredWhenFindingOccludedElement()
        {
            XRECTF scrollBarRect{ 90, 0, 10, 100 };
            MockElement scrollBar(scrollBarRect);

            MockElement button(buttonRect);

            MockElement* elements[] = { &scrollBar, &button };
            MockVisualRoot root(elements, ARRAY_SIZE(elements));

            OcclusivityTester<MockVisualRoot, MockElement, MockElement> tester(&root);

            Expect(button, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(button, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            Expect(scrollBar, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(scrollBar, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::ScrollBar);

            bool result = false;
            VERIFY_SUCCEEDED(tester.Test(&button, ToXRectFRB(buttonRect), &result));
            VERIFY_ARE_EQUAL(result, false);
        }

        void OcclusivityTesterUnitTests::ScrollBarChildShouldBeIgnoredWhenFindingOccludedElement()
        {
            XRECTF scrollBarRect{ 90, 0, 10, 100 };
            MockElement scrollBar(scrollBarRect);
            MockElement scrollBarChild(scrollBarRect);

            MockElement button(buttonRect);

            MockElement* elements[] = { &scrollBarChild, &scrollBar, &button };
            MockVisualRoot root(elements, ARRAY_SIZE(elements));

            OcclusivityTester<MockVisualRoot, MockElement, MockElement> tester(&root);

            Expect(button, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(button, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            Expect(scrollBar, GetTemplatedParent)
                .ReturnValue(nullptr);
            Expect(scrollBar, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::ScrollBar);

            Expect(scrollBarChild, GetTemplatedParent)
                .ReturnValue(&scrollBar);
            Expect(scrollBarChild, GetTypeIndex)
                .ReturnValue(KnownTypeIndex::UIElement);

            bool result = false;
            VERIFY_SUCCEEDED(tester.Test(&button, ToXRectFRB(buttonRect), &result));
            VERIFY_ARE_EQUAL(result, false);
        }
    }
}}}}
