// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------
//
//  class:  CReentrancyGuard
//
//  Synopsis:
//      Protects against reentrancy into a function/method by a
//  single thread. Does not protect against concurrent access. Not
//  thread safe.
//
//------------------------------------------------------------------------

class CReentrancyGuard 
{
public:
    // *pbGuard must be initialized once to FALSE.
    CReentrancyGuard(bool *pbGuard)
    {
        m_pbGuard = pbGuard;
        ASSERT(m_pbGuard);

        // Remember current guard value
        m_bPrevGuard = *m_pbGuard;
        
        // Entering function      
        *m_pbGuard = true;
    }
    
    ~CReentrancyGuard()
    {
        // Leaving function. Restore previous guard value
        *m_pbGuard = m_bPrevGuard; 
    }

    // Has reentrancy occurred?
    //
    // Returns failure code if reentrancy has occurred.
    //
    // Change default value of bTerminateOnReentrancy to FAIL_FAST() by default
    // on reentrancy -- caller can override (FAIL_FAST() is useful to get 
    // a good callstack in retail builds at the point of reentrancy.)
    HRESULT CheckReentrancy(bool bTerminateOnReentrancy = true) 
    { 
        HRESULT hr = S_OK;
        
        if (m_bPrevGuard)
        {
            hr = E_FAIL;

            ASSERT(0);
            
            if (bTerminateOnReentrancy)
            {
                // terminate on reentrancy
                // if needed
                XAML_FAIL_FAST();
            }
        }

        return hr; 
    }

    bool HasReentered() const
    {
        return m_bPrevGuard;
    }

private:
    // Pointer to user provided boolean. TRUE indicates function has been
    // entered, else FALSE.
    bool *m_pbGuard;

    // Value of guard before the function was entered
    bool m_bPrevGuard;
};

