// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WeakReferenceUnitTests.h"

#include <mutex>
#include <random>
#include <ComBase.h>
#include <ComObject.h>
#include <ComPtr.h>
#include <WeakReferenceSourceNoThreadId.h>
#include <wil\resource.h>
#include <XcpAllocationDebug.h>

using namespace Microsoft::WRL;
using namespace WEX;
using namespace WEX::Common;

// Prefix used when formatting numbers that needs to be platform dependent (size).
#ifdef _WIN64
#define _PFX_INT_PTR L"lli"
#else
#define _PFX_INT_PTR L"i"
#endif

namespace
{
    const unsigned int maxIterations = 100;

    void CreateThreadEvents(_Out_writes_(totalEventCount) wil::unique_handle* allThreadsReady,
        _Out_writes_(totalEventCount) HANDLE* allRawThreadReadyHandles,
        _Out_writes_(totalEventCount) wil::unique_handle* allThreadsStartWork,
        _Out_writes_(totalEventCount) HANDLE* allRawThreadStartWorkHandles,
        const int totalEventCount)
    {
        for (int i = 0; i < totalEventCount; i++)
        {
            wil::unique_handle ahThreadReady(CreateEvent(0, FALSE, FALSE, nullptr));
            if (ahThreadReady.get() == INVALID_HANDLE_VALUE)
            {
                VERIFY_FAIL();
            }
            allThreadsReady[i].swap(ahThreadReady);
            allRawThreadReadyHandles[i] = allThreadsReady[i].get();

            wil::unique_handle ahThreadStartWork(CreateEvent(0, FALSE, FALSE, nullptr));
            if (ahThreadStartWork.get() == INVALID_HANDLE_VALUE)
            {
                VERIFY_FAIL();
            }
            allThreadsStartWork[i].swap(ahThreadStartWork);
            allRawThreadStartWorkHandles[i] = allThreadsStartWork[i].get();
        }
    }

    void WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(_In_reads_(totalEventCount) HANDLE* allRawThreadReadyHandles,
        _In_reads_(totalEventCount) HANDLE* allRawThreadStartWorkHandles,
        _In_reads_(totalEventCount) HANDLE* allRawThreadHandles,
        const int totalEventCount)
    {
        // When all the threads signal that they are ready, we proceeded.
        // If signal would had happened straight away, some may miss their corresponding SetEvent and get stuck
        VERIFY_ARE_NOT_EQUAL(WaitForMultipleObjects(totalEventCount, allRawThreadReadyHandles, TRUE, INFINITE), WAIT_ABANDONED_0);
        for (int i = 0; i < totalEventCount; i++)
        {
            SetEvent(allRawThreadStartWorkHandles[i]);
        }

        WEX::Logging::Log::Comment(String().Format(L"Waiting for all threads to finish"));

        VERIFY_ARE_NOT_EQUAL(WaitForMultipleObjects(totalEventCount, allRawThreadHandles, TRUE, INFINITE), WAIT_ABANDONED_0);
    }

    namespace detail
    {
        // implementation detail of StartThread
        template <class Fn>
        DWORD WINAPI ThreadProc(
            _In_  LPVOID lpParameter
            )
        {
            std::unique_ptr<Fn> spfn( static_cast<Fn*>(lpParameter) );
            (*spfn)();
            return 0;
        }
    }

    // helper function - create a new thread, and run the specified function
    template <class Fn>
    wil::unique_handle StartThread(Fn fn)
    {
        std::unique_ptr<Fn> spfn( new Fn(fn) );
        wil::unique_handle h( CreateThread(0, 0, & detail::ThreadProc<Fn>, spfn.get(), 0, nullptr) );
        if (h.get() == INVALID_HANDLE_VALUE)
        {
            throw std::exception();
        }
        spfn.release(); // abandon (do not free, the created thread will free it when needed)
        return h;
    };

    class RCWithWeakReference
        : public ctl::WeakReferenceSourceNoThreadId
    {
    public:
        unsigned long __stdcall AddRef() { return static_cast<ComBase*>(this)->AddRef(); }
        unsigned long __stdcall Release() { return static_cast<ComBase*>(this)->Release(); }
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject) { return static_cast<ComBase*>(this)->QueryInterface(riid, ppvObject); }
    };

    class RCWithWeakReferenceExposeRefMember
        : public RCWithWeakReference
    {
    public:
        INT_PTR GetValue() { return refCount_.rawValue; }

        __inline STDMETHOD_(ULONG, AddRef2)(_Out_ INT_PTR* previousValue, _Out_ INT_PTR* afterValue)
        {
            ULONG value = 0;
            *previousValue = refCount_.rawValue;
            value = static_cast<ComBase*>(this)->AddRef();
            *afterValue = refCount_.rawValue;
            return value;
        }

        __inline STDMETHOD_(ULONG, Release2)(_Out_ INT_PTR* previousValue, _Out_ INT_PTR* afterValue)
        {
            ULONG value = 0;
            *previousValue = refCount_.rawValue;
            value = static_cast<ComBase*>(this)->Release();
            *afterValue = refCount_.rawValue;
            return value;
        }

        __inline STDMETHOD(GetWeakReference2)(_Outptr_ IWeakReference **weakReference, _Out_ INT_PTR* previousValue, _Out_ INT_PTR* afterValue)
        {
            HRESULT hr = S_OK;
            *previousValue = refCount_.rawValue;
            hr = GetWeakReference(weakReference);
            *afterValue = refCount_.rawValue;
            return hr;
        }
    };
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {

    template<typename T>
    wrl::ComPtr<T> Make()
    {
        wrl::ComPtr<T> instance;
        THROW_IF_FAILED(ctl::ComObject<T>::CreateInstance<T>(&instance));
        return instance;
    }

    bool WeakReferenceUnitTests::ClassSetup()
    {
#if XCP_MONITOR
        InitCheckedMemoryChainLock();
#endif
        return true;
    }

    bool WeakReferenceUnitTests::ClassCleanup()
    {
#if XCP_MONITOR
        DeleteCheckedMemoryChainLock();
#endif
        return true;
    }

    void WeakReferenceUnitTests::Test_WeakReferenceOptimization()
    {
        // Simple case no weak reference requested expect 1 allocation
        {
            auto rc = Make<RCWithWeakReference>();

            IInspectable* ins = rc.Get();
            VERIFY_IS_TRUE(ins->AddRef() == 2);
            VERIFY_IS_TRUE(ins->Release() == 1);
        }

        // Weak reference resolved expect 2 allocations
        {
            auto rc = Make<RCWithWeakReference>();
            VERIFY_IS_TRUE(rc != nullptr);

            WeakRef wr;
            VERIFY_IS_TRUE(SUCCEEDED(rc.AsWeak(&wr)));
            VERIFY_IS_TRUE(wr != nullptr);

            ComPtr<IInspectable> ins;
            VERIFY_IS_TRUE(SUCCEEDED(wr.As(&ins)));
            VERIFY_IS_TRUE(ins != nullptr);

            IInspectable* insRaw = ins.Get();
            VERIFY_IS_TRUE(insRaw->AddRef() == 3);
            VERIFY_IS_TRUE(insRaw->Release() == 2);
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceParallelAddReference()
    {
        const int totalThreadCount = 25;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            ComPtr<IInspectable> spResolved;
            ComPtr<IWeakReference> spWeakReference;

            for (int j = 0; j < totalThreadCount; j++)
            {
                // Generic lambda for all the threads that will AddRef
                auto AddRefBaseFunct = [&, j]()
                {
                    ULONG valueAfterAddRefCall = 0;
                    INT_PTR valueBeforeInternalAddRefCall = 0;
                    INT_PTR valueAfterInternalAddRefCall = 0;

                    // Random work to be done by all the AddRef threads
                    while (idleDist(mt) != 0) {};

                    valueAfterAddRefCall = rc.Get()->AddRef2(&valueBeforeInternalAddRefCall, &valueAfterInternalAddRefCall);
                    // Logging::Log::Comment(String().Format(L"valueAfterAddRefCall %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR L" -- Current Count %j", rc.Get()->GetValue(), valueBeforeInternalAddRefCall, valueAfterInternalAddRefCall, valueAfterAddRefCall));
                };

                allThreads[j] = StartThread([&, j]()
                {
                    SetEvent(allRawThreadReadyHandles[j]);
                    if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                    {
                        VERIFY_FAIL();
                    }
                    AddRefBaseFunct();
                });

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            // Verify that the value is NOT encoded
            VERIFY_IS_GREATER_THAN(rc.Get()->GetValue(), 0);

            VERIFY_ARE_EQUAL(static_cast<unsigned long>(totalThreadCount + 2), rc.Get()->AddRef());

            VERIFY_SUCCEEDED(rc.Get()->GetWeakReference(&spWeakReference));

            VERIFY_SUCCEEDED(spWeakReference->Resolve<IInspectable>(&spResolved));

            VERIFY_ARE_EQUAL(static_cast<unsigned long>(totalThreadCount + 4), rc.Get()->AddRef());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));

            // Leave two alive for the ComPtr rc and the spResolved
            while (rc.Get()->Release() > 2)
                ;
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceParallelRelease()
    {
        const int totalThreadCount = 25;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            ComPtr<IInspectable> spResolved;
            ComPtr<IWeakReference> spWeakReference;

            // Add one reference for each thread that will run + the extra ones needed to keep the books in order (some extra releases done in order to check the numbers).
            while (rc.Get()->AddRef() < totalThreadCount + 3)
                ;

            for (int j = 0; j < totalThreadCount; j++)
            {
                // Generic lambda for all the threads that will AddRef
                auto AddRefBaseFunct = [&, j]()
                {
                    ULONG valueAfterAddRefCall = 0;
                    INT_PTR valueBeforeInternalAddRefCall = 0;
                    INT_PTR valueAfterInternalAddRefCall = 0;

                    // Random work to be done by all the AddRef threads
                    while (idleDist(mt) != 0) {};

                    valueAfterAddRefCall = rc.Get()->Release2(&valueBeforeInternalAddRefCall, &valueAfterInternalAddRefCall);
                    //Logging::Log::Comment(String().Format(L"valueAfterAddRefCall %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR L" -- Current Count %j", rc.Get()->GetValue(), valueBeforeInternalAddRefCall, valueAfterInternalAddRefCall, valueAfterAddRefCall));
                };

                allThreads[j] = StartThread([&, j]()
                {
                    SetEvent(allRawThreadReadyHandles[j]);
                    if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                    {
                        VERIFY_FAIL();
                    }
                    AddRefBaseFunct();
                });

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            // Verify that the value is NOT encoded
            VERIFY_IS_GREATER_THAN(rc.Get()->GetValue(), 0);

            VERIFY_ARE_EQUAL(2UL, rc.Get()->Release());

            VERIFY_SUCCEEDED(rc.Get()->GetWeakReference(&spWeakReference));

            VERIFY_SUCCEEDED(spWeakReference->Resolve<IInspectable>(&spResolved));

            // Two references left, one for rc and another one for spResolved
            VERIFY_ARE_EQUAL(2UL, rc.Get()->Release());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceParallelReleaseWithFinalRelease()
    {
        const int totalThreadCount = 25;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            Logging::Log::Comment(String().Format(L"Pointer value %p", rc.Get()));

            while (rc.Get()->AddRef() < totalThreadCount)
                ;

            for (int j = 0; j < totalThreadCount; j++)
            {
                // Generic lambda for all the threads that will Release
                auto AddRefBaseFunct = [&, j]()
                {
                    ULONG valueAfterReleaseCall = 0;
                    INT_PTR valueBeforeInternalReleaseCall = 0;
                    INT_PTR valueAfterInternalReleaseCall = 0;

                    // Random work to be done by all the AddRef threads
                    while (idleDist(mt) != 0) {};

                    valueAfterReleaseCall = rc.Get()->Release2(&valueBeforeInternalReleaseCall, &valueAfterInternalReleaseCall);
                    //Logging::Log::Comment(String().Format(L"valueAfterReleaseCall %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR L" -- Current Count %j", rc.Get()->GetValue(), valueBeforeInternalReleaseCall, valueAfterInternalReleaseCall, valueAfterReleaseCall));
                    //Logging::Log::Comment(String().Format(L"Pointer value %p", rc.Get()));

                    if (valueAfterReleaseCall == 0)
                    {
                        rc.Detach();
                    }
                };

                allThreads[j] = StartThread([&, j]()
                {
                    SetEvent(allRawThreadReadyHandles[j]);
                    if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                    {
                        VERIFY_FAIL();
                    }
                    AddRefBaseFunct();
                });

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            VERIFY_IS_NULL(rc.Get());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceParallelCreateWeakReference()
    {
        const int totalThreadCount = 25;
        unsigned int loopCount = 0;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            loopCount++;

            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            ComPtr<IInspectable> spResolved;
            ComPtr<IWeakReference> spWeakReference;
            ctl::Details::WeakReferenceImpl* pRawWeakReference = nullptr;
            INT_PTR rawLongWeakReference = 0;

            for (int j = 0; j < totalThreadCount; j++)
            {
                allThreads[j] = StartThread([&, j]()
                {
                    ComPtr<IWeakReference> spWeakReferenceLocal;
                    INT_PTR valueBeforeInternalAddRefCall = 0;
                    INT_PTR valueAfterInternalAddRefCall = 0;

                    SetEvent(allRawThreadReadyHandles[j]);
                    if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                    {
                        VERIFY_FAIL();
                    }

                    while (idleDist(mt) != 0) {};

                    if (FAILED(rc.Get()->GetWeakReference2(&spWeakReferenceLocal, &valueBeforeInternalAddRefCall, &valueAfterInternalAddRefCall)))
                    {
                        VERIFY_FAIL();
                    }
                    //Logging::Log::Comment(String().Format(L"Done GetWeakReference with value %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR, rc.Get()->GetValue(), valueBeforeInternalAddRefCall, valueAfterInternalAddRefCall));
                });

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            // Verify that the value is encoded
            rawLongWeakReference = rc.Get()->GetValue();
            VERIFY_IS_LESS_THAN(rawLongWeakReference, 0);

            pRawWeakReference = ctl::Details::DecodeWeakReferencePointer(rawLongWeakReference);
            VERIFY_ARE_EQUAL(1L, pRawWeakReference->GetSourceRefCount());

            VERIFY_ARE_EQUAL(2UL, rc.Get()->AddRef());

            VERIFY_SUCCEEDED(rc.Get()->GetWeakReference(&spWeakReference));

            VERIFY_ARE_EQUAL(3UL, spWeakReference.Get()->AddRef());

            VERIFY_SUCCEEDED(spWeakReference->Resolve<IInspectable>(&spResolved));

            VERIFY_ARE_EQUAL(4UL, rc.Get()->AddRef());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));

            // Leave one alive for rc and another one for spResolved
            while (rc.Get()->Release() > 2)
                ;
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceCreateWeakReferenceAndAddRef()
    {
        const int workerThreadCount = 25;
        const int totalThreadCount = workerThreadCount + 1;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> weakReferenceThreadDist(0, totalThreadCount - 1);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            ComPtr<IInspectable> spResolved;
            ComPtr<IWeakReference> spWeakReference;
            ctl::Details::WeakReferenceImpl* pRawWeakReference = nullptr;
            INT_PTR rawLongWeakReference = 0;
            bool success = false;
            int weakReferenceThreadIndex = 0;

            weakReferenceThreadIndex = weakReferenceThreadDist(mt);

            Logging::Log::Comment(String().Format(L"WR thread # is %i", weakReferenceThreadIndex));

            for (int j = 0; j < totalThreadCount; j++)
            {
                // Only one thread tries to call GetWeakReference
                if (j != weakReferenceThreadIndex)
                {
                    allThreads[j] = StartThread([&, j]()
                    {
                        SetEvent(allRawThreadReadyHandles[j]);
                        if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                        {
                            VERIFY_FAIL();
                        }

                        ULONG valueAfterAddRefCall = 0;
                        INT_PTR valueBeforeInternalAddRefCall = 0;
                        INT_PTR valueAfterInternalAddRefCall = 0;

                        // Random work to be done by all the worker threads
                        while (idleDist(mt) != 0) {};

                        valueAfterAddRefCall = rc.Get()->AddRef2(&valueBeforeInternalAddRefCall, &valueAfterInternalAddRefCall);
                        //Logging::Log::Comment(String().Format(L"valueAfterAddRefCall %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR L" -- Current Count %j", rc.Get()->GetValue(), valueBeforeInternalAddRefCall, valueAfterInternalAddRefCall, valueAfterAddRefCall));

                        if (valueBeforeInternalAddRefCall > 0 && valueAfterInternalAddRefCall < 0)
                        {
                            // One of the AddRef threads was able to hit the case where it tried to commit the above call while the other thread swap the internal member from count to a pointer
                            success = true;
                            //Logging::Log::Comment(String().Format(L"SUCCESS. One thread was able to commit the WeakReference while this thread tried to AddRef"));
                        }
                    });
                }
                else
                {
                    allThreads[j] = StartThread([&, j]()
                    {
                        SetEvent(allRawThreadReadyHandles[j]);
                        if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                        {
                            VERIFY_FAIL();
                        }

                        while (idleDist(mt) != 0) {};

                        if (FAILED(rc.Get()->GetWeakReference(&spWeakReference)))
                        {
                            VERIFY_FAIL();
                        }
                        //Logging::Log::Comment(String().Format(L"Done GetWeakReference with value %" _PFX_INT_PTR, rc.Get()->GetValue()));
                    });
                }

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            // Verify that the value is encoded
            rawLongWeakReference = rc.Get()->GetValue();
            VERIFY_IS_LESS_THAN(rawLongWeakReference, static_cast<INT_PTR>(0));

            pRawWeakReference = ctl::Details::DecodeWeakReferencePointer(rawLongWeakReference);
            VERIFY_ARE_EQUAL((workerThreadCount + 1), pRawWeakReference->GetSourceRefCount());

            VERIFY_ARE_EQUAL(static_cast<unsigned long>(workerThreadCount + 2), rc.Get()->AddRef());

            VERIFY_SUCCEEDED(spWeakReference->Resolve<IInspectable>(&spResolved));

            VERIFY_ARE_EQUAL(static_cast<unsigned long>(workerThreadCount + 4), rc.Get()->AddRef());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));

            if (success)
            {
                // Under some circumstances, during development and/or long runs,
                // this place holder can be used to stop the execution of the loop because
                // a condition has been meet;
                // break;
            }

            // Leave one alive for rc and one for spResolved
            while (rc.Get()->Release() > 2)
                ;
        }
    }

    void WeakReferenceUnitTests::Test_WeakReferenceCreateWeakReferenceAndRelease()
    {
        const int workerThreadCount = 25;
        const int totalThreadCount = workerThreadCount + 1;

        std::mt19937 mt(2014);
        std::uniform_int_distribution<int> weakReferenceThreadDist(0, totalThreadCount - 1);
        std::uniform_int_distribution<int> idleDist(0, 16);

        for (UINT i = 0; i < maxIterations; i++)
        {
            // wil::unique_handle array to hold the handles for all the threads that will be created.
            // The raw HANDLE array is what will be used in WaitForMultipleObjects
            wil::unique_handle allThreads[totalThreadCount];
            HANDLE allRawThreadHandles[totalThreadCount] = {};

            // Same as above, but these handles are used by each thread to signal this thread
            // that they are ready to start working
            wil::unique_handle allThreadsReady[totalThreadCount];
            HANDLE allRawThreadReadyHandles[totalThreadCount] = {};

            // Same as above, but these are the handles that each thread waits for in order to start working
            wil::unique_handle allThreadsStartWork[totalThreadCount];
            HANDLE allRawThreadStartWorkHandles[totalThreadCount] = {};

            CreateThreadEvents(allThreadsReady, allRawThreadReadyHandles, allThreadsStartWork, allRawThreadStartWorkHandles, totalThreadCount);

            auto rc = Make<RCWithWeakReferenceExposeRefMember>();
            VERIFY_IS_NOT_NULL(rc.Get());
            ComPtr<IInspectable> spResolved;
            ComPtr<IWeakReference> spWeakReference;
            ctl::Details::WeakReferenceImpl* pRawWeakReference = nullptr;
            INT_PTR rawLongWeakReference = 0;
            bool success = false;
            int weakReferenceThreadIndex = 0;

            weakReferenceThreadIndex = weakReferenceThreadDist(mt);

            while (rc.Get()->AddRef() != (workerThreadCount + 2))
                ;

            Logging::Log::Comment(String().Format(L"WR thread # is %i", weakReferenceThreadIndex));

            for (int j = 0; j < totalThreadCount; j++)
            {
                if (j != weakReferenceThreadIndex)
                {
                    allThreads[j] = StartThread([&, j]()
                    {
                        SetEvent(allRawThreadReadyHandles[j]);
                        if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                        {
                            VERIFY_FAIL();
                        }

                        ULONG valueAfterReleaseCall = 0;
                        INT_PTR valueBeforeInternalReleaseCall = 0;
                        INT_PTR valueAfterInternalReleaseCall = 0;

                        // Random work to be done by all the AddRef threads
                        while (idleDist(mt) != 0) {};

                        valueAfterReleaseCall = rc.Get()->Release2(&valueBeforeInternalReleaseCall, &valueAfterInternalReleaseCall);
                        //Logging::Log::Comment(String().Format(L"valueAfterAddRefCall %" _PFX_INT_PTR L" -- Internal Value Before %" _PFX_INT_PTR L" -- Internal Value After %" _PFX_INT_PTR L" -- Current Count %j", rc.Get()->GetValue(), valueBeforeInternalReleaseCall, valueAfterInternalReleaseCall, valueAfterReleaseCall));

                        if (valueBeforeInternalReleaseCall > 0 && valueAfterInternalReleaseCall < 0)
                        {
                            // One of the AddRef threads was able to hit the case where it tried to commit an add ref while the other thread swap the internal member from count to a pointer
                            success = true;
                            //Logging::Log::Comment(String().Format(L"********* SUCCESS"));
                        }
                    });
                }
                else
                {
                    allThreads[j] = StartThread([&, j]()
                    {
                        SetEvent(allRawThreadReadyHandles[j]);
                        if (WaitForSingleObject(allRawThreadStartWorkHandles[j], INFINITE) != WAIT_OBJECT_0)
                        {
                            VERIFY_FAIL();
                        }

                        while (idleDist(mt) != 0) {};

                        if (FAILED(rc.Get()->GetWeakReference(&spWeakReference)))
                        {
                            VERIFY_FAIL();
                        }
                        //Logging::Log::Comment(String().Format(L"Done GetWeakReference with value %" _PFX_INT_PTR, rc.Get()->GetValue()));
                    });
                }

                allRawThreadHandles[j] = allThreads[j].get();
            }

            Logging::Log::Comment(String().Format(L"Starting count %" _PFX_INT_PTR, rc.Get()->GetValue()));

            WaitForThreadsToBeReadySignalAndWaitForAllOfThemToFinish(allRawThreadReadyHandles, allRawThreadStartWorkHandles, allRawThreadHandles, totalThreadCount);

            // Verify that the value is encoded
            rawLongWeakReference = rc.Get()->GetValue();
            VERIFY_IS_LESS_THAN(rawLongWeakReference, static_cast<INT_PTR>(0));

            pRawWeakReference = ctl::Details::DecodeWeakReferencePointer(rawLongWeakReference);
            VERIFY_ARE_EQUAL(2, pRawWeakReference->GetSourceRefCount());

            VERIFY_ARE_EQUAL(1UL, rc.Get()->Release());

            VERIFY_SUCCEEDED(spWeakReference->Resolve<IInspectable>(&spResolved));

            VERIFY_ARE_EQUAL(3UL, rc.Get()->AddRef());

            Logging::Log::Comment(String().Format(L"-----------------------------------------------------------------------------"));

            if (success)
            {
                break;
            }

            // Leave one alive for rc and another one for spResolved
            while (rc.Get()->Release() > 2)
                ;
        }
    }

} } } } }
