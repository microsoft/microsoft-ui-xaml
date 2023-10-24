// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      RAII Pointer wrappers for passing arounds graphics interfaces that
//      need to be locked before use.
//
//      We use both the D3D and D2D locks, because we have scenarios were we
//      only have D3D.  In those cases, we need to use the D3D lock to protect
//      from concurrency with DComp.  However, if we have a D2D device, since it doesn't
//      take the D3D lock, we need to also take the D2D Lock.
//
//      Note that the order that we take the locks is important.  D2D can take the D3D lock
//      after taking the D2D lock, but never the reverse.  Thus we need to take these locks
//      in the same order (as does DComp).
//
//      Releasing the locks is less critical (you can't cause a deadlock), but we release
//      them in the opposite order so there is less chance of contention for the lock.  Since
//      in the majority of cases we will be taking both locks, if we were to release the
//      D2D lock and then lose our time slice before releasing the D3D lock, it is possible
//      that a thread waiting on the D2D lock will be released and then try to take the
//      D3D lock that is still being held by the first thread.

#pragma once

template <class T> struct CLockedGraphicsPointer;

interface ID2D1Multithread;
interface ID3D10Multithread;

template <class T>
struct CLockableGraphicsPointer
{
    friend struct CLockedGraphicsPointer<T>;

public:
    CLockableGraphicsPointer(_In_ T* p, _In_opt_ ID3D10Multithread * d3dMultithread, _In_opt_ ID2D1Multithread * d2dMultithread)
    {
        m_pointer = p;
        m_d2dMultithread = d2dMultithread;
        m_d3dMultithread = d3dMultithread;
    }

    ~CLockableGraphicsPointer()
    {
    }

    CLockedGraphicsPointer<T> GetLocked()
    {
        return CLockedGraphicsPointer<T>(m_pointer.Get(), m_d3dMultithread.Get(), m_d2dMultithread.Get());
    }

    // Copy constructor and assignment for efficient return from a method
    CLockableGraphicsPointer(const CLockableGraphicsPointer& other)
    {
        this->m_pointer = other.m_pointer;
        this->m_d2dMultithread = other.m_d2dMultithread;
        this->m_d3dMultithread = other.m_d3dMultithread;
    }

    CLockableGraphicsPointer& operator=(const CLockableGraphicsPointer& other)
    {
        if (this != &other)
        {
            this->m_pointer = other.m_pointer;
            this->m_d2dMultithread = other.m_d2dMultithread;
            this->m_d3dMultithread = other.m_d3dMultithread;
        }
        return *this;
    }

    CLockableGraphicsPointer& operator=(CLockableGraphicsPointer&& other)
    {
        // Even though we have an r-value, we swap so the natural destruction of
        // the r-value will clean up the ref count on our pointers.
        this->m_pointer.Swap(other.m_pointer);
        this->m_d2dMultithread.Swap(other.m_d2dMultithread);
        this->m_d3dMultithread.Swap(other.m_d3dMultithread);
        return *this;
    }

private:
    Microsoft::WRL::ComPtr<T> m_pointer;
    Microsoft::WRL::ComPtr<ID2D1Multithread> m_d2dMultithread;
    Microsoft::WRL::ComPtr<ID3D10Multithread> m_d3dMultithread;

    // Force on stack
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};

template <class T>
struct CLockedGraphicsPointer
{
public:
    CLockedGraphicsPointer(_In_ T* p, _In_opt_ ID3D10Multithread * d3dMultithread, _In_opt_ ID2D1Multithread * d2dMultithread)
    {
        m_pointer = p;
        m_d2dMultithread = d2dMultithread;
        m_d3dMultithread = d3dMultithread;

        // Note the order is important.  See header for more information
        if (m_d2dMultithread) m_d2dMultithread->Enter();
        if (m_d3dMultithread) m_d3dMultithread->Enter();
    }

    ~CLockedGraphicsPointer()
    {
        if (m_d3dMultithread) m_d3dMultithread->Leave();
        if (m_d2dMultithread) m_d2dMultithread->Leave();
    }

    T * operator ->()
    {
        return m_pointer;
    }

    // Move constructor and assignment for efficient return from a method
    CLockedGraphicsPointer(CLockedGraphicsPointer&& other)
    {
        this->m_pointer = other.m_pointer;
        this->m_d2dMultithread = other.m_d2dMultithread;
        this->m_d3dMultithread = other.m_d3dMultithread;
        other.m_pointer = nullptr;
        other.m_d2dMultithread = nullptr;
        other.m_d3dMultithread = nullptr;
    }

    CLockedGraphicsPointer& operator=(CLockedGraphicsPointer&& other)
    {
        if (this != &other)
        {
            if (this->m_d3dMultithread) this->m_d3dMultithread->Leave();
            if (this->m_d2dMultithread) this->m_d2dMultithread->Leave();
            this->m_pointer = other.m_pointer;
            this->m_d2dMultithread = other.m_d2dMultithread;
            this->m_d3dMultithread = other.m_d3dMultithread;
            other.m_pointer = nullptr;
            other.m_d2dMultithread = nullptr;
            other.m_d3dMultithread = nullptr;
        }
        return *this;
    }

    // Move constructor and assignment from a LockableGraphicsPointer
    CLockedGraphicsPointer(const CLockableGraphicsPointer<T>&& other)
    {
        this->m_pointer = other.m_pointer.Get();
        this->m_d2dMultithread = other.m_d2dMultithread.Get();
        this->m_d3dMultithread = other.m_d3dMultithread.Get();

        // Note the order is important.  See header for more information
        if (this->m_d2dMultithread) this->m_d2dMultithread->Enter();
        if (this->m_d3dMultithread) this->m_d3dMultithread->Enter();
    }

    CLockedGraphicsPointer& operator=(const CLockableGraphicsPointer<T>&& other)
    {
       this->m_pointer = other.m_pointer;
       this->m_d2dMultithread = other.m_d2dMultithread;
       this->m_d3dMultithread = other.m_d3dMultithread;

       // Note the order is important.  See header for more information
       if (this->m_d2dMultithread) this->m_d2dMultithread->Enter();
       if (this->m_d3dMultithread) this->m_d3dMultithread->Enter();
       return *this;
    }

private:
    // Note we don't use smart pointers here or add references because, like
    // the code this is replacing, we assume that somewhere up the stack is
    // is something that already is holding a reference.  This is also why we
    // don't allow copy or lvalue assignment.
    T * m_pointer;
    ID2D1Multithread * m_d2dMultithread;
    ID3D10Multithread * m_d3dMultithread;

    // Disable copy constructor and assignment and force on stack
    CLockedGraphicsPointer(const CLockedGraphicsPointer& other);
    CLockedGraphicsPointer& operator=(CLockedGraphicsPointer& other);
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};
