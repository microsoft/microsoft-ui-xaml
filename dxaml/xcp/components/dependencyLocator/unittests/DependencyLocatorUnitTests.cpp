// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyLocatorUnitTests.h"
#include <minerror.h>
#include <DependencyLocator.h>
#include <TestEvent.h>
#include <process.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

#pragma region MockObjects
        class SimpleObjectBase
        {
        public:
            SimpleObjectBase(std::function<void()> onDestructCallback)
                : m_onDestructCallback(onDestructCallback)
            {}

            virtual ~SimpleObjectBase()
            {
                if (m_onDestructCallback)
                {
                    m_onDestructCallback();
                }
            }
        private:
            std::function<void()> m_onDestructCallback;
        };

        class __declspec(uuid("96C57EE1-2D14-4EF9-8C6A-B43A0AEDB128")) SimpleObject1
            : public SimpleObjectBase
        {
        public:
            SimpleObject1(std::function<void()> onDestructCallback)
                : SimpleObjectBase(onDestructCallback)
            {}

            int ReturnTheNumberOne() { return 1; }
        };

        class __declspec(uuid("298D049B-9630-430A-8F9D-DAA00E3A701D")) SimpleObject2
            : public SimpleObjectBase
        {
        public:
            SimpleObject2(std::function<void()> onDestructCallback)
                : SimpleObjectBase(onDestructCallback)
            {}
        };

        DependencyLocator::Dependency<SimpleObject1> simpleObject1;

        class __declspec(uuid("26D39733-8647-4584-8234-EB627C9FC37C")) SimpleObject3
            : public SimpleObjectBase
        {
        public:
            SimpleObject3(std::function<void()> onDestructCallback)
                : SimpleObjectBase(onDestructCallback)
            {
                // Verifies that we can resolve a dependency within our own
                // constructor.
                VERIFY_ARE_EQUAL(simpleObject1->ReturnTheNumberOne(), 1);
            }

            int ReturnSimpleObject1sNumberOne()
            {
                return simpleObject1->ReturnTheNumberOne();
            }
        };
#pragma endregion

        void DependencyLocatorUnitTests::ValidateRegisterGetGlobal()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([&createdObject]() {
                VERIFY_IS_FALSE(!!createdObject);
                createdObject = std::make_shared<SimpleObject1>(nullptr);
                return createdObject;
            }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            DependencyLocator::Dependency<SimpleObject1> dependency2;

            VERIFY_IS_FALSE(!!createdObject);

            auto registeredObject1 = dependency1.Get();
            auto registeredObject2 = dependency2.Get();
            VERIFY_IS_TRUE(!!createdObject);
            VERIFY_ARE_EQUAL(createdObject.get(), registeredObject1.get());
            VERIFY_ARE_EQUAL(createdObject.get(), registeredObject2.get());

            DependencyLocator::UninitializeThread();
            DependencyLocator::UninitializeProcess();
        }

        void DependencyLocatorUnitTests::ValidateRegisterGetPerThread()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([&createdObject]() {
                VERIFY_IS_FALSE(!!createdObject);
                createdObject = std::make_shared<SimpleObject1>(nullptr);
                return createdObject;
            }), DependencyLocator::StoragePolicyFlags::PerThread);

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            DependencyLocator::Dependency<SimpleObject1> dependency2;

            VERIFY_IS_FALSE(!!createdObject);
            auto registeredObject1 = dependency1.Get();
            auto registeredObject2 = dependency2.Get();
            VERIFY_IS_TRUE(!!createdObject);
            VERIFY_ARE_EQUAL(createdObject.get(), registeredObject1.get());
            VERIFY_ARE_EQUAL(createdObject.get(), registeredObject2.get());

            DependencyLocator::UninitializeThread();
            DependencyLocator::UninitializeProcess();
        }

        void DependencyLocatorUnitTests::ValidateSimultaneousThreadAndProcessProviders()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObjectA;
            std::shared_ptr<SimpleObject2> createdObjectB;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>(
                [&createdObjectA]() {
                    VERIFY_IS_FALSE(!!createdObjectA);
                    createdObjectA = std::make_shared<SimpleObject1>(nullptr);
                    return createdObjectA;
                }), DependencyLocator::StoragePolicyFlags::PerThread);

            DependencyLocator::DependencyProvider<SimpleObject2> provider2(std::function<std::shared_ptr<SimpleObject2>()>(
                [&createdObjectB]() {
                    VERIFY_IS_FALSE(!!createdObjectB);
                    createdObjectB = std::make_shared<SimpleObject2>(nullptr);
                    return createdObjectB;
                }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            DependencyLocator::Dependency<SimpleObject2> dependency2;

            auto registeredObject1 = dependency1.Get();
            auto registeredObject2 = dependency2.Get();
            VERIFY_ARE_EQUAL(createdObjectA.get(), registeredObject1.get());
            VERIFY_ARE_EQUAL(createdObjectB.get(), registeredObject2.get());

            DependencyLocator::UninitializeThread();
            DependencyLocator::UninitializeProcess();
        }

        void DependencyLocatorUnitTests::ValidateArrowOperator()
        {
            DependencyLocator::InitializeProcess();

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([]() {
                return std::make_shared<SimpleObject1>(nullptr);
            }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            VERIFY_ARE_EQUAL(dependency1->ReturnTheNumberOne(), 1);
            DependencyLocator::Internal::ReleaseInstance<SimpleObject1>();

            DependencyLocator::UninitializeProcess();
        }

        void DependencyLocatorUnitTests::ValidatePerProcessDestructorCalled()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject;
            bool destructed = false;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([&createdObject, &destructed]() {
                VERIFY_IS_FALSE(!!createdObject);
                createdObject = std::make_shared<SimpleObject1>([&destructed]() { destructed = true; });
                return createdObject;
            }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            VERIFY_IS_FALSE(!!destructed);
            VERIFY_IS_FALSE(!!createdObject);

            dependency1.Get();

            VERIFY_IS_TRUE(!!createdObject);
            createdObject.reset();
            VERIFY_IS_FALSE(!!destructed);

            DependencyLocator::UninitializeProcess();

            VERIFY_IS_TRUE(!!destructed);
        }

        void DependencyLocatorUnitTests::ValidatePerThreadDestructorCalled()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject;
            bool destructed = false;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([&createdObject, &destructed]() {
                VERIFY_IS_FALSE(!!createdObject);
                createdObject = std::make_shared<SimpleObject1>([&destructed]() { destructed = true; });
                return createdObject;
            }), DependencyLocator::StoragePolicyFlags::PerThread);

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            VERIFY_IS_FALSE(!!destructed);
            VERIFY_IS_FALSE(!!createdObject);

            dependency1.Get();

            VERIFY_IS_TRUE(!!createdObject);
            createdObject.reset();
            VERIFY_IS_FALSE(!!destructed);

            DependencyLocator::UninitializeThread();
            DependencyLocator::UninitializeProcess();
            VERIFY_IS_TRUE(!!destructed);
        }

        void DependencyLocatorUnitTests::ValidateGetWithMultipleObjects()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject1;
            std::shared_ptr<SimpleObject2> createdObject2;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>([&createdObject1]() {
                VERIFY_IS_FALSE(!!createdObject1);
                createdObject1 = std::make_shared<SimpleObject1>(nullptr);
                return createdObject1;
            }));

            DependencyLocator::DependencyProvider<SimpleObject2> provider2(std::function<std::shared_ptr<SimpleObject2>()>([&createdObject2]() {
                VERIFY_IS_FALSE(!!createdObject2);
                createdObject2 = std::make_shared<SimpleObject2>(nullptr);
                return createdObject2;
            }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            DependencyLocator::Dependency<SimpleObject2> dependency2;

            VERIFY_IS_FALSE(!!createdObject1);
            VERIFY_IS_FALSE(!!createdObject2);

            auto registeredObject1 = dependency1.Get();
            VERIFY_ARE_EQUAL(createdObject1.get(), registeredObject1.get());
            VERIFY_IS_FALSE(!!createdObject2);

            auto registeredObject2 = dependency2.Get();
            VERIFY_ARE_EQUAL(createdObject2.get(), registeredObject2.get());
            VERIFY_IS_TRUE(!!createdObject2);

            DependencyLocator::UninitializeProcess();
        }

        unsigned int RunThread(void* pData)
        {
            auto validator = static_cast<std::function<void()>*>(pData);
            (*validator)();
            return 0;
        }

        void DependencyLocatorUnitTests::ValidateUniqueObjectPerThread()
        {
            DependencyLocator::InitializeProcess();

            std::shared_ptr<SimpleObject1> createdObject1A;
            std::shared_ptr<SimpleObject1> createdObject1B;
            std::shared_ptr<SimpleObject2> createdObject2;

            bool object1ADestructed = false;
            bool object1BDestructed = false;
            bool object2Destructed = false;

            DependencyLocator::DependencyProvider<SimpleObject1> provider1(std::function<std::shared_ptr<SimpleObject1>()>(
                [&createdObject1A, &createdObject1B, &object1ADestructed, &object1BDestructed]() {
                if (!createdObject1A)
                {
                    createdObject1A = std::make_shared<SimpleObject1>([&]() { object1ADestructed = true; });
                    return createdObject1A;
                }
                else if (!createdObject1B)
                {
                    createdObject1B = std::make_shared<SimpleObject1>([&]() { object1BDestructed = true; });
                    return createdObject1B;
                }
                VERIFY_FAIL();
                return std::shared_ptr<SimpleObject1>();
            }), DependencyLocator::StoragePolicyFlags::PerThread);

            DependencyLocator::DependencyProvider<SimpleObject2> provider2(std::function<std::shared_ptr<SimpleObject2>()>(
                [&createdObject2, &object2Destructed]() {
                VERIFY_IS_FALSE(!!createdObject2);
                createdObject2 = std::make_shared<SimpleObject2>([&]() { object2Destructed = true; });
                return createdObject2;
            }));

            DependencyLocator::Dependency<SimpleObject1> dependency1;
            DependencyLocator::Dependency<SimpleObject2> dependency2;

            auto registeredObject1 = dependency1.Get();
            auto registeredObject2 = dependency2.Get();

            Microsoft::UI::Xaml::Tests::Common::Event threadFinishedEvent;

            std::function<void()> validator = [&] () {
                VERIFY_ARE_EQUAL(registeredObject2, dependency2.Get());
                auto registeredObject3 = dependency1.Get();
                VERIFY_ARE_NOT_EQUAL(registeredObject1, registeredObject3);
                VERIFY_ARE_EQUAL(registeredObject3, createdObject1B);
                DependencyLocator::UninitializeThread();
                threadFinishedEvent.Set();
            };

            _beginthread(
                [](void* pData) {
                auto validator = static_cast<std::function<void()>*>(pData);
                (*validator)();
            }, 0, static_cast<void*>(&validator));
            threadFinishedEvent.WaitForDefault();

            createdObject1B.reset();
            VERIFY_IS_FALSE(!!object1ADestructed);
            VERIFY_IS_TRUE(!!object1BDestructed);
            VERIFY_IS_FALSE(!!object2Destructed);

            DependencyLocator::UninitializeThread();
            DependencyLocator::UninitializeProcess();
        }

        void DependencyLocatorUnitTests::ValidateNestedSerivceDependency()
        {
            DependencyLocator::InitializeProcess();

            DependencyLocator::DependencyProvider<SimpleObject3> provider1(std::function<std::shared_ptr<SimpleObject3>()>([]() {
                return std::make_shared<SimpleObject3>(nullptr);
            }));

            DependencyLocator::DependencyProvider<SimpleObject1> provider2(std::function<std::shared_ptr<SimpleObject1>()>([]() {
                return std::make_shared<SimpleObject1>(nullptr);
            }));

            DependencyLocator::Dependency<SimpleObject3> dependency1;
            VERIFY_ARE_EQUAL(dependency1->ReturnSimpleObject1sNumberOne(), 1);
            DependencyLocator::UninitializeProcess();
        }
    }
} } } }
