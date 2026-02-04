// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "SmartPointerUnitTests.h"
#include <XamlLogging.h>

#include <minerror.h>
#include <minxcptypes.h>
#include <macros.h>

#include <xref_ptr.h>
#include <weakref_ptr.h>
#include <weakref_count.h>

using namespace WEX::Common;

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace SmartPointers {

// Reference tester for SmartPointers
class CRefTester
{
public:
    uint32_t AddRef()
    {
        return m_ref_count.AddRef();
    }

    uint32_t Release()
    {
        auto count = m_ref_count.Release();

        if (count == 0)
        {
            delete this;
        }

        return count;
    }

    static HRESULT Create(CRefTester **ppRefTester)
    {
        HRESULT hr = S_OK;

        *ppRefTester = new CRefTester();

        RRETURN(hr);//RRETURN_REMOVAL
    }

public:
    uint32_t GetRefCount() const { return m_ref_count.GetRefCount(); }

    xref::details::control_block* EnsureControlBlock()
    {
        return m_ref_count.ensure_control_block();
    }

private:
    CRefTester() {}
    ~CRefTester() {}

    xref::details::optional_ref_count m_ref_count;
};

void XRefPtrUnitTests::ReleaseAndGetAddressOf()
{
    xref_ptr<CRefTester> spRefTester;

    VERIFY_IS_TRUE(!spRefTester);
    VERIFY_SUCCEEDED(CRefTester::Create(spRefTester.ReleaseAndGetAddressOf()));
    VERIFY_IS_TRUE(!!spRefTester);
    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);
}

void XRefPtrUnitTests::Detach()
{
    CRefTester *pRefTester = nullptr;
    CRefTester **ppRefTester = &pRefTester;
    xref_ptr<CRefTester> spRefTester;

    VERIFY_SUCCEEDED(CRefTester::Create(spRefTester.ReleaseAndGetAddressOf()));
    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);

    *ppRefTester = spRefTester.detach();

    VERIFY_IS_TRUE(!spRefTester);
    VERIFY_ARE_EQUAL(pRefTester->GetRefCount(), 1u);

    ReleaseInterface(pRefTester);
}

void XRefPtrUnitTests::CopyTo()
{
    CRefTester *pRefTester = nullptr;
    CRefTester **ppRefTester = &pRefTester;
    xref_ptr<CRefTester> spRefTester;

    VERIFY_SUCCEEDED(CRefTester::Create(spRefTester.ReleaseAndGetAddressOf()));
    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);

    spRefTester.CopyTo(ppRefTester);

    VERIFY_ARE_EQUAL(pRefTester->GetRefCount(), 2u);
    VERIFY_IS_TRUE(!!spRefTester);
    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 2u);

    ReleaseInterface(pRefTester);
}

void XRefPtrUnitTests::WeakRefs()
{
    xref_ptr<CRefTester> spRefTester;
    VERIFY_SUCCEEDED(CRefTester::Create(spRefTester.ReleaseAndGetAddressOf()));
    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);

    xref::weakref_ptr<CRefTester> weakref;
    VERIFY_IS_FALSE(static_cast<bool>(weakref));
    weakref = xref::get_weakref(spRefTester);
    VERIFY_IS_TRUE(static_cast<bool>(weakref));
    VERIFY_IS_FALSE(weakref.expired());

    {
        auto ptr = weakref.lock();
        VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 2u);
        VERIFY_ARE_EQUAL(spRefTester, ptr);

        auto weak2 = xref::get_weakref(ptr);
        VERIFY_ARE_EQUAL(weakref, weak2);
    }

    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);

    {
        auto ptr = weakref.lock_noref();
        VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);
        VERIFY_ARE_EQUAL(spRefTester.get(), ptr);

        auto weak2 = xref::get_weakref(ptr);
        VERIFY_ARE_EQUAL(weakref, weak2);
    }

    VERIFY_ARE_EQUAL(spRefTester->GetRefCount(), 1u);

    // Clear the last ref and verify that resolving the weakref returns null
    spRefTester.reset();
    VERIFY_IS_NULL(spRefTester.get());

    // The weakref is expired, but is still pointing at a valid control block
    VERIFY_IS_TRUE(static_cast<bool>(weakref));
    VERIFY_IS_TRUE(weakref.expired());

    {
        auto ptr = weakref.lock();
        VERIFY_IS_NULL(ptr.get());

        // weak2 shouldn't be pointing at a control block, due to ptr being null
        auto weak2 = xref::get_weakref(ptr);
        VERIFY_ARE_NOT_EQUAL(weakref, weak2);

        weak2 = weakref;
        VERIFY_ARE_EQUAL(weakref, weak2);
    }

    {
        auto ptr = weakref.lock_noref();
        VERIFY_IS_NULL(ptr);

        // weak2 shouldn't be pointing at a control block, due to ptr being null
        auto weak2 = xref::get_weakref(ptr);
        VERIFY_ARE_NOT_EQUAL(weakref, weak2);

        weak2 = weakref;
        VERIFY_ARE_EQUAL(weakref, weak2);
    }

    weakref.reset();

    VERIFY_IS_FALSE(static_cast<bool>(weakref));
}

} } } } }
