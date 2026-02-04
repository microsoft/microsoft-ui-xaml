// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "LockableGraphicsPointerUnitTests.h"
#include "D2d1_1.h"
#include "D3D10.h"

using namespace ::Windows::UI::Xaml::Tests::Graphics;

/* ------------------------------------------------------------------------------------------
Mock IUnknown object used to track reference counts
-------------------------------------------------------------------------------------------*/
class MockIUnknowObject : public IUnknown
{
private:
    ULONG m_refCount;
    ULONG m_maxRefCount;

public:
    MockIUnknowObject() :
        m_refCount(0),
        m_maxRefCount(0)
        {}

    STDMETHOD_(ULONG, AddRef)()  { VERIFY_IS_TRUE(++m_refCount <= m_maxRefCount); return m_refCount; }
    STDMETHOD_(ULONG, Release)() { VERIFY_IS_TRUE(m_refCount-- > 0); return m_refCount; }
    STDMETHOD(QueryInterface)(REFIID refid, void** ppVoid) { return E_NOTIMPL; }

    MockIUnknowObject* GetThis() { return this; }
    void SetMaxRefCount(ULONG maxRefCount) { m_maxRefCount = maxRefCount; }
    ULONG GetRefCount() { return m_refCount; }
};


/* ------------------------------------------------------------------------------------------
Mock implementation of ID2D1Multithread and ID3D10Multithread.
-------------------------------------------------------------------------------------------*/

class MockMultithreadBase
{
    ULONG m_refCount;
    ULONG m_maxRefCount;

    ULONG m_lockCount;
    ULONG m_maxLockCount;

public:
    MockMultithreadBase() :
        m_refCount(0), 
        m_lockCount(0), 
        m_maxRefCount(0),
        m_maxLockCount(0) 
    {}
   
    STDMETHOD_(ULONG, AddRef)()  { VERIFY_IS_TRUE(++m_refCount <= m_maxRefCount); return m_refCount; }
    STDMETHOD_(ULONG, Release)() { VERIFY_IS_TRUE(m_refCount-- > 0); return m_refCount; }

    STDMETHOD_(void, Enter)() { VERIFY_IS_TRUE(++m_lockCount <= m_maxLockCount); }
    STDMETHOD_(void, Leave)() { VERIFY_IS_TRUE(m_lockCount-- > 0); }
    
    void SetMaxLockCount(ULONG maxLockCount) { m_maxLockCount = maxLockCount; }
    void SetMaxRefCount(ULONG maxRefCount) { m_maxRefCount = maxRefCount; }
    ULONG GetLockCount() { return m_lockCount; }
    ULONG GetRefCount() { return m_refCount;  }
};

class MockD2D1Multithread : public MockMultithreadBase, public ID2D1Multithread
{
    STDMETHOD_(ULONG, AddRef)()  { return MockMultithreadBase::AddRef(); }
    STDMETHOD_(ULONG, Release)() { return MockMultithreadBase::Release(); }
    STDMETHOD(QueryInterface)(REFIID refid, void** ppVoid) { return E_NOTIMPL; }

    STDMETHOD_(void, Enter)() { return MockMultithreadBase::Enter(); }
    STDMETHOD_(void, Leave)() { return MockMultithreadBase::Leave(); }
    STDMETHOD_(BOOL, GetMultithreadProtected)() const { return TRUE; }
};

class MockD3D10Multithread : public MockMultithreadBase, public ID3D10Multithread
{
    STDMETHOD_(ULONG, AddRef)()  { return MockMultithreadBase::AddRef(); }
    STDMETHOD_(ULONG, Release)() { return MockMultithreadBase::Release(); }
    STDMETHOD(QueryInterface)(REFIID refid, void** ppVoid) { return E_NOTIMPL; }

    STDMETHOD_(void, Enter)() { return MockMultithreadBase::Enter(); }
    STDMETHOD_(void, Leave)() { return MockMultithreadBase::Leave(); }
    STDMETHOD_(BOOL, GetMultithreadProtected)() { return TRUE; }
    STDMETHOD_(BOOL, SetMultithreadProtected)(BOOL) { VERIFY_IS_TRUE(FALSE); return TRUE; }
};

/* ------------------------------------------------------------------------------------------
    LockableGraphicsPointer Unit Tests
-------------------------------------------------------------------------------------------*/

void LockableGraphicsPointerUnitTests::LockableGraphicsPointer()
{
    MockD2D1Multithread mockD2DMultithread;
    MockD3D10Multithread mockD3DMultithread;
    MockIUnknowObject mockObject;

    // Simple creation 

    {
        mockD2DMultithread.SetMaxRefCount(1);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(1);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(1);


        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Copy assignment

    {
        MockIUnknowObject mockObject2;

        mockD2DMultithread.SetMaxRefCount(2);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(2);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(1);
        mockObject2.SetMaxRefCount(2);


        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer1(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer2(&mockObject2, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockObject2.GetRefCount(), 1);

        lockablePointer1 = lockablePointer2;

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockObject2.GetRefCount(), 2);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Self assignment

    {
        mockD2DMultithread.SetMaxRefCount(1);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(1);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(1);

        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        lockablePointer = lockablePointer;

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Copy constructor
    {
        mockD2DMultithread.SetMaxRefCount(2);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(2);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(2);

        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer2 = lockablePointer;

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 2);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 2);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Move assignment 

    {
        MockIUnknowObject mockObject2;
        mockD2DMultithread.SetMaxRefCount(2);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(2);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(1);
        mockObject2.SetMaxRefCount(1);

        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer = GetLockableGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        lockablePointer = GetLockableGraphicsPointer(&mockObject2, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockObject2.GetRefCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Move constructor 
    {
        mockD2DMultithread.SetMaxRefCount(1);
        mockD2DMultithread.SetMaxLockCount(0);
        mockD3DMultithread.SetMaxRefCount(1);
        mockD3DMultithread.SetMaxLockCount(0);
        mockObject.SetMaxRefCount(1);

        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer = GetLockableGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // GetLocked 
    {
        mockD2DMultithread.SetMaxRefCount(1);
        mockD2DMultithread.SetMaxLockCount(1);
        mockD3DMultithread.SetMaxRefCount(1);
        mockD3DMultithread.SetMaxLockCount(1);
        mockObject.SetMaxRefCount(1);

        CLockableGraphicsPointer<MockIUnknowObject> lockablePointer = GetLockableGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer = lockablePointer.GetLocked();

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
}

void LockableGraphicsPointerUnitTests::LockedGraphicsPointer()
{
    MockD2D1Multithread mockD2DMultithread;
    MockD3D10Multithread mockD3DMultithread;
    MockIUnknowObject mockObject;

    // Simple creation 

    {
        mockD2DMultithread.SetMaxRefCount(0);
        mockD2DMultithread.SetMaxLockCount(1);
        mockD3DMultithread.SetMaxRefCount(0);
        mockD3DMultithread.SetMaxLockCount(1);
        mockObject.SetMaxRefCount(0);

        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_IS_TRUE(lockedPointer->GetThis() == &mockObject);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Copy construction and assignment are not valid for CLockedGraphicsPointer because we don't
    // want them held onto long term.

    // Move constructor

    {
        mockD2DMultithread.SetMaxRefCount(0);
        mockD2DMultithread.SetMaxLockCount(1);
        mockD3DMultithread.SetMaxRefCount(0);
        mockD3DMultithread.SetMaxLockCount(1);
        mockObject.SetMaxRefCount(0);

        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer = GetLockedGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_IS_TRUE(lockedPointer->GetThis() == &mockObject);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Move assignment 
    {
        MockIUnknowObject mockObject2;

        mockD2DMultithread.SetMaxRefCount(0);
        mockD2DMultithread.SetMaxLockCount(2);
        mockD3DMultithread.SetMaxRefCount(0);
        mockD3DMultithread.SetMaxLockCount(2);
        mockObject.SetMaxRefCount(1);
        mockObject2.SetMaxRefCount(1);

        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer = GetLockedGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);
        lockedPointer = GetLockedGraphicsPointer(&mockObject2, &mockD3DMultithread, &mockD2DMultithread);

        // Even though we briefly take two locks (one for each object), we should only have one once we have replace
        // the original contents of lockedPointer.
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockObject2.GetRefCount(), 0);
        VERIFY_IS_TRUE(lockedPointer->GetThis() == &mockObject2);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Move from lockable pointer
    {
        mockD2DMultithread.SetMaxRefCount(1);  // Lockable Pointer will take a reference
        mockD2DMultithread.SetMaxLockCount(1);
        mockD3DMultithread.SetMaxRefCount(1);  // Lockable Pointer will take a reference
        mockD3DMultithread.SetMaxLockCount(1);
        mockObject.SetMaxRefCount(1); // Lockable Pointer will take a reference

        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer = GetLockableGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread);

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);  // But we shouldn't have held any referenced
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);  // But we shouldn't have held any referenced
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
        VERIFY_IS_TRUE(lockedPointer->GetThis() == &mockObject);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);

    // Construction from lockable pointer
    {
        mockD2DMultithread.SetMaxRefCount(1);  // Lockable Pointer will take a reference
        mockD2DMultithread.SetMaxLockCount(1);
        mockD3DMultithread.SetMaxRefCount(1);  // Lockable Pointer will take a reference
        mockD3DMultithread.SetMaxLockCount(1);
        mockObject.SetMaxRefCount(1); // Lockable Pointer will take a reference

        CLockedGraphicsPointer<MockIUnknowObject> lockedPointer(GetLockableGraphicsPointer(&mockObject, &mockD3DMultithread, &mockD2DMultithread));

        VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);  // But we shouldn't have held any referenced
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);  // But we shouldn't have held any referenced
        VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
        VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 1);
        VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 1);
        VERIFY_IS_TRUE(lockedPointer->GetThis() == &mockObject);
    }
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD2DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetRefCount(), 0);
    VERIFY_ARE_EQUAL(mockD3DMultithread.GetLockCount(), 0);
    VERIFY_ARE_EQUAL(mockObject.GetRefCount(), 0);
}


CLockedGraphicsPointer<MockIUnknowObject> LockableGraphicsPointerUnitTests::GetLockedGraphicsPointer(MockIUnknowObject* pTestPointer, ID3D10Multithread * pD3DMultithread, ID2D1Multithread * pD2DMultithread)
{
    return CLockedGraphicsPointer<MockIUnknowObject>(pTestPointer, pD3DMultithread, pD2DMultithread);
}

CLockableGraphicsPointer<MockIUnknowObject> LockableGraphicsPointerUnitTests::GetLockableGraphicsPointer(MockIUnknowObject* pTestPointer, ID3D10Multithread * pD3DMultithread, ID2D1Multithread * pD2DMultithread)
{
    return CLockableGraphicsPointer<MockIUnknowObject>(pTestPointer, pD3DMultithread, pD2DMultithread);
}

