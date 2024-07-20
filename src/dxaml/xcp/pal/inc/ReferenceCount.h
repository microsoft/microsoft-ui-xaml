// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <minpal.h>
#include "PalDebugging.h"

// Provides base object for regular and interlocked reference counts.
//
// In debug builds includes an 8 byte magic word and methods for
// validatihg it and the refernce count value.
class CReferenceCountBase
{
public:
    mutable XUINT32 m_cReferences;

#if DBG
    XUINT64 m_magic; // Shows up in memory as 'refcount' or as 'destroyd'
#endif

    CReferenceCountBase();

    virtual ~CReferenceCountBase();

#if DBG
    void AddRefPreCheck() const;

    void AddRefPostCheck(XUINT32 cReferences) const;

    void ReleasePreCheck() const;

    void ReleasePostCheck(XUINT32 cReferences) const;
#endif
};

// Provides a mixin class for objects requiring a non multi-threaded
// reference count.
//
// In debug builds includes the following checks:
//   - Magic word (8 bytes) has the correct value
//   - Reference count in range
//   - All AddRefs and Releases called from the same thread
//   - Deletion is by Release, not 'delete'
class CReferenceCount : protected CReferenceCountBase
{
protected:
#if DBG && i386 && XCP_MONITOR
    mutable IThreadMonitor *m_pThreadMonitor;

    void ThreadCheck(const WCHAR *pszMessage) const;
#endif

public:
    CReferenceCount();

    ~CReferenceCount() override;

    XUINT32 AddRef() const;

    XUINT32 Release() const;
};
