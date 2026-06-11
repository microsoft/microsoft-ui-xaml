// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CxxMock.h"
#include "UIAEnums.h"
#include "AccessKey.h"

using namespace AccessKeys;

struct Base { };
class CDependencyObject {};

MOCK_CLASS(CValue, Base)
    STUB_METHOD(xstring_ptr, AsString, 0);
    STUB_METHOD(bool, AsBool, 0);
    STUB_METHOD(int, AsEnum, 0);
END_MOCK

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace AccessKeys {

    struct Base { };

    MOCK_CLASS(MockProvider, Base)
        STUB_METHOD(HRESULT, Invoke, 0)
        STUB_METHOD(HRESULT, Toggle, 0)
        STUB_METHOD(HRESULT, Select, 0)
        STUB_METHOD(HRESULT, Expand, 0)
        STUB_METHOD(HRESULT, Collapse, 0)
        STUB_METHOD(HRESULT, get_ExpandCollapseState, 1(UIAXcp::ExpandCollapseState*))
    END_MOCK

    MOCK_CLASS(MockUIAProvider, Base)
        STUB_METHOD(MockProvider*, GetPatternInterface, 0)
    END_MOCK

    MOCK_CLASS(MockPeer, Base)
        STUB_METHOD(MockUIAProvider*,GetPattern,1(int))
    END_MOCK

    class MockAKOwner;

    MOCK_CLASS(MockCDO, CDependencyObject)
        MockAKOwner* m_pOwner;

        xref::details::control_block* EnsureControlBlock()
        {
            static xref::details::control_block block(1);
            return  &block;
        }

        template <KnownTypeIndex targetTypeIndex>
        bool OfTypeByIndex()
        {
            return OfTypeByIndex(targetTypeIndex);
        }

        STUB_METHOD(MockPeer*, GetAutomationPeer, 0)
        STUB_METHOD(MockPeer*, OnCreateAutomationPeer, 0)

        STUB_METHOD(void, AddRef, 0)
        STUB_METHOD(void, Release, 0)
        STUB_METHOD(bool, IsActive, 0)
        STUB_METHOD(bool, OfTypeByIndex, 1(KnownTypeIndex))
        STUB_METHOD(MockCDO*, GetParent, 0)
        STUB_METHOD(bool, RaiseAccessKeyInvoked, 0)

        STUB_METHOD(HRESULT, GetValueByIndex, 2(KnownPropertyIndex, CValue*));

    END_MOCK


    MOCK_CLASS(MockAKOwner, Base)
        MockAKOwner() = default;

        bool operator==(const MockAKOwner& rhs) const { return m_element == rhs.m_element && m_accessKey == rhs.m_accessKey; };

        MockAKOwner(const MockAKOwner& copy) : m_element(copy.m_element), m_accessKey(copy.m_accessKey)
        {
            m_element->m_pOwner = this;
        }

        MockAKOwner(MockCDO* element, AKAccessKey accessKey) :
            m_element(element),
            m_accessKey(accessKey)
        {
            m_element->m_pOwner = this;
        }

        const AKAccessKey& GetAccessKey() const { return m_accessKey; }
        const xref::weakref_ptr<MockCDO> GetElement() const { return xref::get_weakref(m_element); }

        MockCDO* m_element;
        AKAccessKey m_accessKey;

        STUB_METHOD(HRESULT, ShowAccessKey, 1(const wchar_t*), const)
        STUB_METHOD(HRESULT, HideAccessKey, 0, const)
        STUB_METHOD(bool, Invoke, 0, const)
    END_MOCK

}}}}}
#include "AccessKeyEvents.MockSpecializations.h"
