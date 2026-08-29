// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines XcpAutoLock, a RAII-style class for critical sections.

#pragma once

//
// XcpAutoCriticalSection is a RAII-style class to automatically 
// initialize and deinitialize a critical section.
//
// Instances of this class will typically be global variables,  
// static class members, or non-static members of classes whose
// instances are global variables or static members.
//
class XcpAutoCriticalSection
{
public:
    // 
    // Creates an XcpAutoCriticalSection, initializing the critical section.
    // When the XcpAutoCriticalSection goes out of scope and is destructed,
    // the critical section will be deinitialized.
    //
    XcpAutoCriticalSection()
    {
        InitializeCriticalSection(&m_criticalSection);
    }
    
    // 
    // Deinitializes the critical section.
    //
    ~XcpAutoCriticalSection()
    {
        DeleteCriticalSection(&m_criticalSection);
    }

    //
    // Returns the critical section managed by this instance.
    //
    CRITICAL_SECTION* Get()
    {
        return &m_criticalSection;
    }

private:
    CRITICAL_SECTION m_criticalSection;

    //
    // prohibit copying and assignment
    //
    XcpAutoCriticalSection(const XcpAutoCriticalSection&);
    XcpAutoCriticalSection& operator=(const XcpAutoCriticalSection&);
};

//
// XcpAutoLock is a RAII-style class to automatically Enter/Leave a 
// critical section.
//
class XcpAutoLock
{
public:
    //
    // Creates an XcpAutoLock, acquiring the critical section.
    //
    // When the XcpAutoLock goes out of scope and is destructed,
    // the critical section will be released.
    //
    // pCriticalSection - the critical section to manage. 
    // Must already be initialized.
    //
    XcpAutoLock(_In_ CRITICAL_SECTION* pCriticalSection) :
        m_pCriticalSection(pCriticalSection)
    {
        Initialize();
    }

    //
    // Creates an XcpAutoLock, acquiring the critical section from
    // the XcpAutoCriticalSection.
    //
    // When the XcpAutoLock goes out of scope and is destructed,
    // the critical section will be released.
    //
    // pCriticalSection - the critical section to manage. 
    // Must already be initialized.
    //
    XcpAutoLock(XcpAutoCriticalSection& autoCriticalSection) :
        m_pCriticalSection(autoCriticalSection.Get())
    {
        Initialize();
    }

    //
    // Releases the critical section.
    //
    ~XcpAutoLock()
    {
        LeaveCriticalSection(m_pCriticalSection);
    }

private:
    void Initialize()
    {
        ASSERT(m_pCriticalSection);
        EnterCriticalSection(m_pCriticalSection);
    }        
    
    //
    // The critical section that's entered while this instance is alive.
    //
    CRITICAL_SECTION* m_pCriticalSection;

    //
    // prohibit copying, assignment, and heap allocation
    //
    XcpAutoLock(const XcpAutoLock&);
    XcpAutoLock& operator=(const XcpAutoLock&);
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};

