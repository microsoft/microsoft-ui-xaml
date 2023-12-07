// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutoReentrantReferenceLock.h"

namespace ctl
{
    class WeakReferenceSourceNoThreadId;

    template <typename T, bool fPegIfReachable>
    class AutoPeg
    {
    private:        
        xaml_hosting::IReferenceTrackerInternal *m_ptr;
        bool m_fPegged;
        bool m_fReachable;
        bool m_fPeggedIfNull;

    public:
        XNOTHROW AutoPeg(_In_opt_ T* ptr)
            : m_fPeggedIfNull(FALSE)
        {
            Init(ptr);
        }

        XNOTHROW AutoPeg(_In_opt_ T* ptr, _In_opt_ bool fPeggedIfNull )
            : m_fPeggedIfNull(fPeggedIfNull)
        {
            Init(ptr);
        }

        XNOTHROW AutoPeg(AutoPeg&& other)
            : m_ptr(nullptr), m_fPegged(FALSE), m_fPeggedIfNull(FALSE)
        {
            m_ptr = other.m_ptr;
            m_fPegged = other.m_fPegged;
            m_fReachable = other.m_fReachable;
            m_fPeggedIfNull = other.m_fPeggedIfNull;

            // Reset the other state
            other.m_ptr = NULL;
            other.m_fPegged = FALSE;
            other.m_fReachable = FALSE;
            other.m_fPeggedIfNull = FALSE;
        }

        operator bool ()
        {
            return 
                // True if we have a value and it's reachable
                ((m_ptr != NULL) && (m_fReachable == TRUE))

                // Also true if explicitly requested (TrackerPtr uses this when it holds a non-null
                // to an object that isn't an IReferenceTracker).
                || (m_ptr == NULL) && m_fPeggedIfNull;
        }

        XNOTHROW ~AutoPeg()
        {
            if (m_fPegged)
            {
                ASSERT(m_ptr);    // We can only peg if we have a tracker internal 
                    
                m_ptr->UpdatePeg(false);
            }
        } 

    private:
        template<typename U>
        void Init(_In_ U* ptr)
        {
            xaml_hosting::IReferenceTrackerInternal *pTracker = NULL;

            // We might be shutting down, but we shouldn't be fully shut down.
            ASSERT(!DirectUI::DXamlServices::IsDXamlCoreShutdown() || DirectUI::DXamlServices::IsDXamlCoreShuttingDown());
            
            pTracker = ctl::query_interface<xaml_hosting::IReferenceTrackerInternal>(ptr);
            Init(pTracker);
            
            ReleaseInterface(pTracker);
        }

        template<>
        void Init<xaml_hosting::IReferenceTrackerInternal>(_In_ xaml_hosting::IReferenceTrackerInternal* pTracker)
        {
            m_fPegged = FALSE;

            // by default, the object is reachable, unless its a special IReferenceTrackerInternal, in which
            // case the state is determined by the IsReachable flag on it
            m_fReachable = TRUE; 

            // Don't call AddRef on the pointer yet; it may not be reachable, which will lead to an exception if it is
            // a composed object (where the controlling unknown may be gone).
            m_ptr = pTracker;
            if (pTracker)
            {
                if (fPegIfReachable)
                {
                    DirectUI::AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                    if (!DXamlServices::IsDXamlCoreShutdown() && pTracker->IsReachable())
                    {
                        PegTrackerObject(pTracker);
                    }
                    else
                    {
                        m_fReachable = FALSE;
                    }
                }
                else
                { 
                    // no lock required

                    PegTrackerObject(pTracker);
                }
            }

        }

        void PegTrackerObject(_In_ xaml_hosting::IReferenceTrackerInternal *pObject)
        {
            pObject->UpdatePeg(true);
            m_fPegged = TRUE;
        }

        AutoPeg(const AutoPeg& other);
        AutoPeg& operator= (const AutoPeg& other);
    };

    template <typename T>
    AutoPeg<T, FALSE> make_autopeg(T* ptr)
    {
        return AutoPeg<T, FALSE>(ptr);
    }
    
    AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE> try_make_autopeg(_In_opt_ xaml_hosting::IReferenceTrackerInternal* pTracker);
    AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE> try_make_autopeg(_In_opt_ WeakReferenceSourceNoThreadId* pTracker);
}
