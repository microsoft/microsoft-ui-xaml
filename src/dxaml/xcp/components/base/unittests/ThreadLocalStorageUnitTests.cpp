// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ThreadLocalStorageUnitTests.h"
#include <ThreadLocalStorage.h>
#include <exception>
#include <memory>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Base {

    struct MyData : public std::enable_shared_from_this < MyData >
    {
        std::unique_ptr<int> m_ptr;

        MyData() = default;

        // Verify constructor param forwarding
        explicit MyData(int arg) : m_ptr(new int(arg)) {}
    };

    // Explicitly instantiate all methods of the template class to check that all methods make sense
    TlsProvider < MyData > myTldProvider;
    TlsProviderHelper < MyData > myTlsProviderHelper;

    void ThreadLocalStorageUnitTests::ValidateBasicLifetime()
    {
        auto sp = TlsProvider<MyData>::GetWrappedObject();
        VERIFY_IS_NULL(sp.get());

        sp = TlsProvider<MyData>::CreateWrappedObject();
        VERIFY_IS_NOT_NULL(sp.get());

        std::weak_ptr<MyData> wp = TlsProvider<MyData>::GetWrappedObject();
        VERIFY_IS_FALSE(wp.expired());
        VERIFY_ARE_EQUAL(wp.lock(), sp);

        sp.reset();
        VERIFY_IS_TRUE(wp.expired());
        VERIFY_IS_NULL(wp.lock().get());

        VERIFY_IS_NULL(TlsProvider<MyData>::GetWrappedObject().get());
    }

    void ThreadLocalStorageUnitTests::ValidateConstructorForwarding()
    {
        const int answer = 42;

        auto sp = TlsProvider<MyData>::CreateWrappedObject(answer);
        VERIFY_IS_NOT_NULL(sp.get());
        VERIFY_ARE_EQUAL(answer, *(sp->m_ptr));

        std::weak_ptr<MyData> wp = TlsProvider<MyData>::GetWrappedObject();
        VERIFY_IS_FALSE(wp.expired());
        VERIFY_ARE_EQUAL(wp.lock(), sp);

        sp.reset();
        VERIFY_IS_NULL(TlsProvider<MyData>::GetWrappedObject().get());
    }

} } } } }
