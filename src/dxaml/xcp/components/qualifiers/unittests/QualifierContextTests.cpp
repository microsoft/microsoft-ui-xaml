// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "QualifierContextTests.h"
#include "XamlLogging.h"
#include <WexTestClass.h>
#include "QualifierContext.h"
#include "IQualifierContextCallback.h"
#include <functional>
#include "MocksAndHelpers.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Qualifiers {

        void QualifierContextUnitTests::HeightContextChanged()
        {
            LOG_OUTPUT(L"Create QualifierContext and listener. Register listener for width only.");
            QualifierContext qualifierContext;
            QualifierContextTestListener listener;
            qualifierContext.RegisterChangedCallback(&listener, (QualifierFlags)QualifierFlags::Height);
            QualifierContextTestListener listener2;
            qualifierContext.RegisterChangedCallback(&listener2, (QualifierFlags)QualifierFlags::Width);

            LOG_OUTPUT(L"Simulate window change and verify listener was notified");
            qualifierContext.OnWindowChanged(-1, 100);
            VERIFY_IS_TRUE(listener.HasBeenCalledBack);
            VERIFY_IS_FALSE(listener2.HasBeenCalledBack);
        }

        void QualifierContextUnitTests::WidthContextChanged()
        {
            LOG_OUTPUT(L"Create QualifierContext and listener. Register listener for width only.");
            QualifierContext qualifierContext;
            QualifierContextTestListener listener;
            qualifierContext.RegisterChangedCallback(&listener, (QualifierFlags)QualifierFlags::Width);
            QualifierContextTestListener listener2;
            qualifierContext.RegisterChangedCallback(&listener2, (QualifierFlags)QualifierFlags::Height);

            LOG_OUTPUT(L"Simulate window change and verify listener was notified");
            qualifierContext.OnWindowChanged(100, -1);
            VERIFY_IS_TRUE(listener.HasBeenCalledBack);
            VERIFY_IS_FALSE(listener2.HasBeenCalledBack);
        }

        void QualifierContextUnitTests::PlatformContextChanged()
        {
            // TODO: The platform API QualifierContext will consume is not implemented yet.
        }

        void QualifierContextUnitTests::UnregisterListener()
        {
            LOG_OUTPUT(L"Create QualifierContext and listener. Register listener.");
            QualifierContext qualifierContext;
            QualifierContextTestListener listener;
            qualifierContext.RegisterChangedCallback(&listener, (QualifierFlags)-1);

            LOG_OUTPUT(L"Unregister listener");
            qualifierContext.RegisterChangedCallback(&listener, (QualifierFlags)NULL);

            LOG_OUTPUT(L"Simulate all context changes and verify listeners were not called back");
            qualifierContext.OnWindowChanged(99, 99);

            LOG_OUTPUT(L"Verify listener was not notified");
            VERIFY_IS_FALSE(listener.HasBeenCalledBack);
        }

        void QualifierContextUnitTests::UpdateListener()
        {
            LOG_OUTPUT(L"Create QualifierContext and listener. Register listener for width only.");
            QualifierContext qualifierContext;
            QualifierContextTestListener listener;
            qualifierContext.RegisterChangedCallback(&listener, QualifierFlags::Width);

            LOG_OUTPUT(L"Re-register listener for width only ");
            qualifierContext.RegisterChangedCallback(&listener, QualifierFlags::Height);

            LOG_OUTPUT(L"Simulate window width change and verify listener was *not* notified");
            qualifierContext.OnWindowChanged(99, -1);
            VERIFY_IS_FALSE(listener.HasBeenCalledBack);

            LOG_OUTPUT(L"Simulate event and verify listener was notified");
            qualifierContext.OnWindowChanged(-1, 99);
            VERIFY_IS_TRUE(listener.HasBeenCalledBack);
        }

        void QualifierContextUnitTests::ListenersAreCalledBack()
        {
            // Create QualifierContext
            LOG_OUTPUT(L"Create QualifierContext");
            QualifierContext qualifierContext;

            // Create a listener and add it to qualifierContext
            LOG_OUTPUT(L"Create QualifierContext and register listeners");
            QualifierContextTestListener qualifierContextListener;
            qualifierContext.RegisterChangedCallback(&qualifierContextListener, QualifierFlags::Width);

            // Simulate window change event for width
            qualifierContext.OnWindowChanged(100, 0);

            LOG_OUTPUT(L"Verify width listener was called back");
            VERIFY_IS_TRUE(qualifierContextListener.HasBeenCalledBack);

            // Register a second listener for height only and reset object listening to width only
            qualifierContextListener.HasBeenCalledBack = false;
            QualifierContextTestListener qualifierContextListener2;
            qualifierContext.RegisterChangedCallback(&qualifierContextListener2, QualifierFlags::Height);

            // Simulate window change event for height
            qualifierContext.OnWindowChanged(100, 100);

            LOG_OUTPUT(L"Verify height listener (listener2) was called back");
            VERIFY_IS_TRUE(qualifierContextListener2.HasBeenCalledBack);
            LOG_OUTPUT(L"Verify width listener (listener1) was *not* called back");
            VERIFY_IS_FALSE(qualifierContextListener.HasBeenCalledBack);
        }

        void QualifierContextUnitTests::MultipleListeners()
        {
            LOG_OUTPUT(L"Create QualifierContext");
            QualifierContext qualifierContext;

            LOG_OUTPUT(L"Create listeners");
            QualifierContextTestListener registeredListeners[10];
            QualifierContextTestListener unregisteredListeners[10];
            for(int i = 0; i < 10; ++i)
            {
                qualifierContext.RegisterChangedCallback(&registeredListeners[i],
                        (QualifierFlags)(QualifierFlags::Height));

                qualifierContext.RegisterChangedCallback(&unregisteredListeners[i],
                        (QualifierFlags)(QualifierFlags::Height));

            }

            for(int i = 0; i < 10; ++i)
            {
                qualifierContext.RegisterChangedCallback(&unregisteredListeners[i],
                        QualifierFlags::None);
            }

            LOG_OUTPUT(L"Simulate Window size change");
            qualifierContext.OnWindowChanged(100, 100);

            LOG_OUTPUT(L"Verify listeners were notified");
            for(int i = 0; i < 10; ++i)
            {
                VERIFY_IS_TRUE(registeredListeners[i].HasBeenCalledBack);
            }

            LOG_OUTPUT(L"Verify unregistered listeners were not notified");
            for(int i = 0; i < 10; ++i)
            {
                VERIFY_IS_FALSE(unregisteredListeners[i].HasBeenCalledBack);
            }
        }
    }
} } } }

