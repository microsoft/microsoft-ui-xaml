// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma push_macro("max")
#undef max

#include <mutex>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <minerror.h>

namespace std {
    // A hash specialization for GUID. It will XOR the GUID in size_t chunks to create a size_t
    // hash suitable for unordered_map.
    template <> struct hash<GUID>
    {
        std::size_t operator()(const GUID& guid) const
        {
            using std::size_t;
            static_assert(sizeof(GUID) == 16, "GUID must be 16 bytes wide.");
            static_assert(sizeof(size_t) == 4 || sizeof(size_t) == 8, "unsigned int must be 4 or 8 bytes wide.");

            if constexpr (sizeof(size_t) == 4)
            {
                return (*reinterpret_cast<const size_t*>(&guid) + 0) ^
                       (*reinterpret_cast<const size_t*>(&guid) + 1) ^
                       (*reinterpret_cast<const size_t*>(&guid) + 2) ^
                       (*reinterpret_cast<const size_t*>(&guid) + 3);
            }
            else
            {
                return (*reinterpret_cast<const size_t*>(&guid) + 0) ^
                       (*reinterpret_cast<const size_t*>(&guid) + 1);
            }
        }
    };
}


// DependencyLocator solves two problems:
// - How to do lifetime management of instances of types on a per-thread or per-process
//   basis without adding to DXamlCore/CCoreServices.
// - How to resolve runtime per-thread or per-process instance dependencies between components
//   without adding to DXamlCore/CCoreServices in a way that works across DLLs without APISets
//   or special-cased DLL exports and is unit-test friendly.
// This makes the bar for adding new things to DXamlCore/CCoreServices extremely high.
//
// How to use DependencyLocator:
// 1) Add a unique [uuid("...")] tag to the type you wish to make available.
// 2) In the CPP file of the type declare a static instance of a DependencyProvider:
//    DependencyProvider<MyInterestingType> provider(
//        std::function<std::shared_ptr<MyInterestingType>()>([]() { return std::make_shared<MyInterestingType>(); }),
//        StoragePolicyFlags::PerThread);
// 3) In the CPP file of the place you wish to consume your dependency declare a static instance of a Dependency:
//    Dependency<MyInterstingType> s_myInterestingType
//    ...
//    s_myInterestingType->DoSomethingAwesome();
//
// Dependency Locator will resolve the dependency upon first access, instantiate an instance if needed, and allow
// you to use it.

namespace DependencyLocator
{
    enum StoragePolicyFlags
    {
        None = 0x00,
        PerThread = 0x01
        // FUTURE: Adding PerModule as a flag if needed.
    };

    // Call this method at the deinitialization of every thread to ensure
    // all dependencies associated with that thread are properly released.
    void UninitializeThread();

    // Call this method at the deinitialization of the application before
    // the CRT runs to ensure all dependencies associated with the process
    // are properly released before XCPLeakDetector runs and to ensure that
    // the DependencyProviders are destructed as expected.
    void UninitializeProcess();

    // Call this method at the initialization of the process to perform
    // the initial TLS index allocation.
    void InitializeProcess();

    namespace Internal
    {
        // This raw_allocator class avoids the tyranny of Jupiter's custom new/delete operators and
        // XCPLeakDetector. This enables us to declare providers as static variables and allows for
        // the heap allocations in the unordered maps to succeed.
        template <class T>
        struct raw_allocator
        {
            typedef T value_type;
            typedef value_type* pointer;
            typedef const value_type* const_pointer;
            typedef value_type& reference;
            typedef const value_type& const_reference;
            typedef std::size_t size_type;
            typedef std::ptrdiff_t difference_type;

            template<typename U>
            struct rebind {
                typedef raw_allocator<U> other;
            };

            raw_allocator()
            {}

            template <class U>
            raw_allocator(const raw_allocator<U>&)
            {}

            inline pointer address(reference r) { return &r; }
            inline const_pointer address(const_reference r) { return &r; }
            inline size_type max_size() const {
                return std::numeric_limits<size_type>::max() / sizeof(T);
            }

            inline void construct(pointer p, const T& t) { new(p)T(t); }
            inline void destroy(pointer p) { p->~T(); }

            inline bool operator==(raw_allocator const&) { return true; }
            inline bool operator!=(raw_allocator const& a) { return !operator==(a); }

            T* allocate(std::size_t n)
            {
                auto memory = HeapAlloc(GetProcessHeap(), 0, n * sizeof(T));
                if (!memory)
                {
                    XAML_FAIL_FAST();
                }
                return static_cast<T*>(memory);
            }

            void deallocate(T* p, std::size_t)
            {
                HeapFree(GetProcessHeap(), 0, p);
            }
        };

        // Specialization of raw_allocator with only the types needed for rebasing
        // and basic assignment/construction.
        template <>
        struct raw_allocator < void >
        {
            typedef void *pointer;
            typedef const void *const_pointer;
            typedef void value_type;
            template<class Other>
            struct rebind
            {
                typedef raw_allocator<Other> other;
            };

            raw_allocator()
            {}

            raw_allocator(const raw_allocator<void>&)
            {}

            template<class Other>
            raw_allocator(const raw_allocator<Other>&)
            {}

            template<class Other>
            raw_allocator<void>& operator=(const raw_allocator<Other>&)
            {
                return *this;
            }
        };

        typedef std::function<std::shared_ptr<void>()> ActivatorFunction;
        typedef std::pair<GUID, std::pair<ActivatorFunction, StoragePolicyFlags>> DependencyActivatorMapPair;
        typedef std::unordered_map<GUID, std::pair<ActivatorFunction, StoragePolicyFlags>, std::hash<GUID>, std::equal_to<GUID>, raw_allocator < DependencyActivatorMapPair >> DependencyActivatorMap;
        typedef std::pair<GUID, std::shared_ptr<void>> DependencyMapPair;
        typedef std::unordered_map<GUID, std::shared_ptr<void>, std::hash<GUID>, std::equal_to<GUID>, raw_allocator<DependencyMapPair>> DependencyMap;

        // Protects resource T& from multiple thread access
        template<class T>
        class ProtectedResource
        {
            std::lock_guard<std::recursive_mutex> m_lockGuard;
            T& m_resource;
        public:
            ProtectedResource(std::recursive_mutex& mutex, T& resource) :
                m_lockGuard(mutex),
                m_resource(resource)
            {
            }

            ProtectedResource(ProtectedResource&& other) :
                m_lockGuard(std::move(other.m_lockGuard)),
                m_resource(other.resource)
            {
            }

            T& Get()
            {
                return m_resource;
            }

            const T& Get() const
            {
                return m_resource;
            }

            ProtectedResource(const ProtectedResource&) = delete;
            ProtectedResource& operator=(const ProtectedResource&) = delete;
        };

        interface INotifyLocalDependencyStorageDestroyed;

        interface __declspec(novtable) ILocalDependencyStorage
        {
        public:
            virtual std::shared_ptr<void> Get(const GUID &guid, bool create = true) = 0;
            virtual void SetDestroyedCallback(INotifyLocalDependencyStorageDestroyed* callback) = 0;
        };

        interface __declspec(novtable) INotifyLocalDependencyStorageDestroyed
        {
        public:
            virtual void Destroyed(ILocalDependencyStorage* storage) = 0;
        };

        class LocalDependencyStorage : public ILocalDependencyStorage
        {
            friend void DependencyLocator::UninitializeThread();
            friend void DependencyLocator::UninitializeProcess();
            friend void DependencyLocator::InitializeProcess();

        public:
            LocalDependencyStorage()
                : m_initialized(false)
                , m_tlsIndex(TLS_OUT_OF_INDEXES)
                , m_callback(nullptr)
            {
            }

            LocalDependencyStorage(const LocalDependencyStorage&) = delete;
            LocalDependencyStorage& operator=(const LocalDependencyStorage&) = delete;

            ~LocalDependencyStorage()
            {
                if (m_callback)
                {
                    m_callback->Destroyed(this);
                }
            }

            static LocalDependencyStorage& Instance();

            void Reset();

            template<class T>
            void UnregisterActivator()
            {
                if (IsInitialized())
                {
                    auto activatorMap = GetActivatorMap();
                    activatorMap.Get().erase(__uuidof(T));
                }
            }

            // Destroys a created instance from dependency storage. This could be useful
            // when you need to deterministically destroy an instance of a dependency before
            // its defined scope is exited (process or thread). Right now it's used in
            // unit tests.
            template<class T>
            void ReleaseInstance()
            {
                ASSERT(IsInitialized());

                const auto& activatorData = GetActivatorData(__uuidof(T));

                if (activatorData.second & StoragePolicyFlags::PerThread)
                {
                    auto storageNoRef = EnsureThreadInitialized();
                    storageNoRef->erase(__uuidof(T));
                }
                else
                {
                    auto map = GetMap();
                    map.Get().erase(__uuidof(T));
                }
            }

            // This method is allocation-free in the create=false case when accessing
            // PerProcess members.
            std::shared_ptr<void> Get(const GUID &guid, bool create) override
            {
                ASSERT(IsInitialized());
                const auto& activatorData = GetActivatorData(guid);
                const auto& activator = activatorData.first;

                auto getOrCreateLambda = [&activator, create, guid](DependencyMap& map) -> std::shared_ptr<void> {
                    auto instanceItem = map.find(guid);
                    if (instanceItem == map.end())
                    {
                        if (create)
                        {
                            auto newInstance = activator();
                            map.insert(std::make_pair(guid, newInstance));
                            return newInstance;
                        }
                        return nullptr;
                    }
                    else
                    {
                        return (*instanceItem).second;
                    }
                };

                if (activatorData.second & StoragePolicyFlags::PerThread)
                {
                    auto storageNoRef = EnsureThreadInitialized();
                    return getOrCreateLambda(*storageNoRef);
                }
                else
                {
                    auto map = GetMap();
                    return getOrCreateLambda(map.Get());
                }
            }

            template<class T>
            void RegisterActivator(_In_ ActivatorFunction activator, _In_ StoragePolicyFlags flags)
            {
                auto activatorMap = GetActivatorMap();
                auto insertResult = activatorMap.Get().insert(std::make_pair(__uuidof(T), std::make_pair(std::move(activator), flags)));
                ASSERT(insertResult.second);
                MarkInitialized();
            }

            void SetDestroyedCallback(INotifyLocalDependencyStorageDestroyed* callback) override
            {
                m_callback = callback;
            }

        private:
            void InitializeTls();
            void UninitializeThread();
            DependencyMap* EnsureThreadInitialized();
            void MarkInitialized();

            // This method serves the purpose of preventing the static deinitialization fiasco.
            // Since primitive types don't have destructors it doesn't take part in static deinitialization
            // ordering and will be valid the entire time statically initialized objects are being
            // destructed.
            //
            // Part of the DependencyLocator contract is that UninitializeProcess will be called
            // when the main application thread shuts down, before the CRT runs through registered
            // destructors. In that method this is set to false. All the DependencyProviders, when
            // they are destructed, read this value and avoid touching the potentially destructed
            // maps when it is false, assuming their registration has already been cleared.
            bool IsInitialized() const;

            // This method right now looks through all the activators registered from DependencyProviders.
            // If one isn't found it will ASSERT. In the future to enable multiple DLLs the simplest
            // implementation will supplement this function with a static table of type UUIDs to DLL names
            // that's compiled into every DLL. If the dependency isn't found in the activator storage
            // we will attempt to LoadLibrary on the DLL the dependency should be located in. When the
            // DLL is loaded all the static DependencyProviders will run and register their dependencies.
            const std::pair<ActivatorFunction, StoragePolicyFlags>& GetActivatorData(const GUID& guid)
            {
                auto activatorMap = GetActivatorMap();
                auto activatorItem = activatorMap.Get().find(guid);
                ASSERT(activatorItem != activatorMap.Get().end());
                return (*activatorItem).second;
            }

            // These methods prevent the static initialization fiasco. They ensure that any statically-initialized
            // types will initialize these variables before accessing them. IsInitialized() below will perform the
            // opposite operation, ensuring that no method accesses them after they've been destructed.
            ProtectedResource<DependencyMap> GetMap();
            ProtectedResource<DependencyActivatorMap> GetActivatorMap();

        private:
            INotifyLocalDependencyStorageDestroyed* m_callback;
            bool m_initialized;
            std::recursive_mutex m_mapMutex;
            DependencyMap m_map;
            std::recursive_mutex m_activatorMutex;
            DependencyActivatorMap m_activatorMap;
            // We allocate a TLS index upon first PerThread dependency instantiation.
            unsigned int m_tlsIndex;
#ifdef DBG
            // An atomic int for bookkeeping if we have any pending initialized threads to
            // uninitialize. In DBG builds we ASSERT this is zero when we uninitialize the
            // process.
            std::atomic<int> m_initializedThreads;
#endif
        };

        template<class T>
        void RegisterActivator(_In_ ActivatorFunction activator, _In_ StoragePolicyFlags flags)
        {
            LocalDependencyStorage& storage = LocalDependencyStorage::Instance();
            storage.RegisterActivator<T>(activator, flags);
        }

        template<class T>
        void UnregisterActivator()
        {
            LocalDependencyStorage& storage = LocalDependencyStorage::Instance();
            storage.UnregisterActivator<T>();
        }

        // Destroys a created instance from dependency storage. This could be useful
        // when you need to deterministically destroy an instance of a dependency before
        // its defined scope is exited (process or thread). Right now it's used in
        // unit tests.
        template<class T>
        void ReleaseInstance()
        {
            LocalDependencyStorage& storage = LocalDependencyStorage::Instance();
            storage.ReleaseInstance<T>();
        }

        inline ProtectedResource<LocalDependencyStorage> GetDependencyLocatorStorage()
        {
            static std::recursive_mutex mutex;
            static LocalDependencyStorage instance;
            return ProtectedResource<LocalDependencyStorage>(mutex, instance);
        }
    }

    // Represents a way to declare a class or translation unit has a dependency
    // on a per-thread or per-process instance. Provides a way to locate an instance
    // of that dependency using a global registry and resolve it into a strong pointer.
    template<class T>
    class Dependency
    {
    protected:
        virtual Internal::ILocalDependencyStorage& GetStorage() const
        {
            return Internal::LocalDependencyStorage::Instance();
        }

    public:
        Dependency() = default;

        std::shared_ptr<T> Get(_In_ bool create = true)
        {
            return GetInternal(create);
        }

        T* operator->() const noexcept
        {
            return GetInternal().operator->();
        }

        Dependency(const Dependency&) = delete;
        Dependency& operator=(const Dependency&) = delete;

    private:
        std::shared_ptr<T> GetInternal(bool create = true) const
        {
            return std::static_pointer_cast<T>(GetStorage().Get(__uuidof(T), create));
        }
    };

    // Represents a provider for a certain dependency. The constructor takes a lambda that
    // will instantiate an instance of the dependency, and allows for lifetime to be specified.
    // For the scope of this class's lifetime the dependency will be available to any clients.
    template<typename T> class DependencyProvider
    {
    public:
        // WARNING: If DependencyProvider is statically declared the lambda provided here
        // must not contain a closure in order to take advantage of small closure optimizations
        // and avoid the unavailable global new operator. There is no strong use-case for a capture
        // group in that case. If a workaround is absolutely needed DependencyLocator's custom raw_allocator
        // can be used.
        DependencyProvider(_In_ std::function<std::shared_ptr<T>()> activator, _In_ StoragePolicyFlags flags = StoragePolicyFlags::None)
        {
            Internal::RegisterActivator<T>(std::function<std::shared_ptr<void>()>(std::allocator_arg, Internal::raw_allocator<void>(),
                [activator]() { return std::static_pointer_cast<void>(activator()); }), flags);
        }

        DependencyProvider(_In_ std::function<::Microsoft::WRL::ComPtr<T>()> activator, _In_ StoragePolicyFlags flags = StoragePolicyFlags::None)
        {
            Internal::RegisterActivator<T>(std::function<std::shared_ptr<void>()>(std::allocator_arg, Internal::raw_allocator<void>(),
                [activator]() { return std::static_pointer_cast<void>(std::shared_ptr<T>(activator().Get(), [](T* instance) { instance->Release(); })); }), flags);
        }

        DependencyProvider(const DependencyProvider& other) = delete;
        DependencyProvider& operator=(const DependencyProvider&) = delete;

        ~DependencyProvider()
        {
            Internal::UnregisterActivator<T>();
        }
    };
}

#define PROVIDE_DEPENDENCY_WITHNAME(name, typeName, ...) \
    DependencyLocator::DependencyProvider<typeName>  name( \
        std::function<std::shared_ptr<typeName>()>([]() { return std::make_shared<typeName>(); }), \
         __VA_ARGS__)

#define PROVIDE_RAW_ALLOC_DEPENDENCY_WITHNAME(name, typeName, ...) \
    DependencyLocator::DependencyProvider<typeName>  name( \
        std::function<std::shared_ptr<typeName>()>([]() { return std::allocate_shared<typeName>(DependencyLocator::Internal::raw_allocator<void>()); }), \
         __VA_ARGS__)

#define PROVIDE_DEPENDENCY(typeName, ...) PROVIDE_DEPENDENCY_WITHNAME(__COUNTER__##typeName, typeName, __VA_ARGS__)
#define PROVIDE_RAW_ALLOC_DEPENDENCY(typeName, ...) PROVIDE_RAW_ALLOC_DEPENDENCY_WITHNAME(__COUNTER__##typeName, typeName, __VA_ARGS__)

#pragma pop_macro("max")

