// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//
//      Character span support

inline XUINT32 Min(XUINT32 a, XUINT32 b)
{
    return a < b ? a : b;
}

//------------------------------------------------------------------------
//
//  Class:  SpanBase - very very simple dynamic array base class
//
//  Synopsis:
//
//      Supports minimal vector functionality:
//
//         SpanBase[]              - directly address indexed element
//                                   (index checked in checked builds)
//         SpanBase.MakeRoom(size) - Allocate memory for at least
//                                   size elements
//         SpanBase.Shrink(size)   - Reduce vector to exactly size
//
//------------------------------------------------------------------------


template <class C> class SpanBase
{
public:
    SpanBase() :
        m_cAllocated (0),
        m_pElements  (NULL)
    {}

    ~SpanBase()
    {
        delete [] m_pElements;
        m_pElements = NULL;
    }

    XCP_FORCEINLINE C &operator[] (XUINT32 index)
    {
        ASSERT(index >= 0  &&  index < m_cAllocated);
        return m_pElements[index];
    }

    XCP_FORCEINLINE const C &operator[] (XUINT32 index) const
    {
        ASSERT(index >= 0  &&  index < m_cAllocated);
        return m_pElements[index];
    }

    XCP_FORCEINLINE _Check_return_ XUINT32 GetAllocatedCount()
    {
        return m_cAllocated;
    }


    //------------------------------------------------------------------------
    //
    //  Method:  SpanBase::MakeRoom
    //
    //  Synopsis:
    //
    //      Allocate at least enough memory for the amount requested,
    //      and enough extra to reduce the likelihood of further
    //      allocations to the point where allocation is a reasonably
    //      small overhead.
    //
    //------------------------------------------------------------------------

    _Check_return_ HRESULT MakeRoom(XUINT32 cTarget)
    {
        if (cTarget > m_cAllocated)
        {
            XUINT32 cNewSize = cTarget;   // Required minimum new size

            // Round up to nearest higher power of sqrt(2). The idea is to
            // grow at least exponentially, but not as fast as doubling each
            // time.

            // First find nearest higher power of 2.

            cNewSize |= cNewSize >> 1;
            cNewSize |= cNewSize >> 2;
            cNewSize |= cNewSize >> 4;
            cNewSize |= cNewSize >> 8;
            cNewSize |= cNewSize >> 16;
            cNewSize++;

            // We now know that cNewSize is a power of two
            // and that cTarget is between cNewSize/2 and cNewSize.

            // Adjust roundup to power of sqrt(2) by seeing which side of
            // 3/4 cNewSize cTarget falls.

            if (cTarget < cNewSize - (cNewSize >> 2))
            {
                // cTarget is between 1/2 and 3/4 the next higher power
                // of two - reduce cNewSize by 1/4 cNewSize.

                cNewSize -= cNewSize >> 2;

                // (This isn't exactly powers of root 2 as the intermediate steps
                // are 1.5 times the next lower power of two when they should be
                // 1.414 times. But this is more than good enough.)
            }

            IFC_RETURN(SetExactSize(cNewSize));
        }

        return S_OK;
    }


    //------------------------------------------------------------------------
    //
    //  Method:  SpanBase::Shrink
    //
    //  Synopsis:
    //
    //      Reduce memory to targetted size, or leave at current size
    //      if already smaller.
    //
    //------------------------------------------------------------------------

    _Check_return_ HRESULT Shrink(XUINT32 cTarget)
    {
        if (cTarget < m_cAllocated)
        {
            IFC_RETURN(SetExactSize(cTarget));
        }

        return S_OK;
    }

private:
    //------------------------------------------------------------------------
    //
    //  Method:  SpanBase::SetExactSize (private)
    //
    //  Synopsis:
    //
    //      Allocate at least enough memory for the amount requested,
    //      and enough extra to reduce the likelihood of further
    //      allocations to the point where allocation is a reasonably
    //      small overhead.
    //
    //------------------------------------------------------------------------
    _Check_return_ HRESULT SetExactSize(XUINT32 cNewSize)
    {
        HRESULT  hr           = S_OK;
        C       *pNewElements = NULL;
        XUINT32 cElementsToCopy = Min(m_cAllocated, cNewSize);

        if (cNewSize > 0)
        {
            pNewElements = new C[cNewSize];
        }

        if (cElementsToCopy > 0)
        {
            memcpy(pNewElements, m_pElements, cElementsToCopy*sizeof(C));
        }

        delete [] m_pElements;
        m_pElements = pNewElements;
        m_cAllocated = cNewSize;

        RRETURN(hr);//RRETURN_REMOVAL
    }




    XUINT32  m_cAllocated;
    C       *m_pElements;
};





//------------------------------------------------------------------------
//
//  Class:  Span - an entry in a span vector
//
//  Synopsis:
//
//      A span is a simple pairing of a pointer to an arbitrary
//      run attribute, and a run length.
//
//      Contain just an element (always a pointer), and a character count.
//
//------------------------------------------------------------------------

template <class C> class Span {
public:

    Span() :
        m_element     (NULL),
        m_cCharacters (0)
    {}

    Span(const C initElement, XUINT32 initLength) :
        m_element     (initElement),
        m_cCharacters (initLength)
    {}

    ~Span() {}

    C       m_element;
    XUINT32 m_cCharacters;
};




//------------------------------------------------------------------------
//
//  Class:  SpanVector - a list of Spans representing the changing value
//                       of a single property along a string
//
//  Synopsis:
//
//      A SpanVector is an array of spans with support routines to update
//      the spans over character ranges.
//
//      The span vector is a list of pairs
//          <attribute value, how many characters it affects>
//
//      SpanVectors usually contain few entries, hence the efficiency of
//      the representation. In abnormal cases where there are many changes
//      of attribute the SpanVector code must spend significant time counting
//      run lengths to reach a given character position.
//
//      It is common for character attributes to be left to their default
//      value for the entire character run, and the SpanVector in this case
//      contains inly the default value and no vector is allocated.
//
//      When changing, inserting or deleting attributes of character ranges,
//      the SpanVector takes care of optimizing the insertions to maintain
//      a minimal length span vector, by merging adjacent spans sharing the
//      same attribute value.
//
//      A span whose element has value NULL is considered equivalent to a
//      missing span.
//
//      Main methods:
//
//          HRESULT SetSpan(first, m_cCharacters, value)
//
//              Sets the range [first .. first+m_cCharacters-1]  to the given
//              value. Previous values (if any) for the range are lost.
//              Adjacent ranges are merged where possible.
//
//          HRESULT OrSpan(first, m_cCharacters, value)
//
//              Sets the range [first .. first+m_cCharacters-1]  to the result of
//              OR combination between given and existing value. Duplicate
//              ranges are merged where possible.
//
//          const C &spanVector.GetElement(INT)
//
//               Returns element at position i, default if none
//
//          const C &spanvector.GetDefault()    - gets default value
//          XUINT32  spanvector.GetSpanCount()  - returns number of spans in vector
//          Span<C> &spanvector[XUINT32]        - returns indexed Span
//          void     spanvector.Free()          - free every non-null element
//
//------------------------------------------------------------------------

template <class C> class SpanRider;

template <class C> class SpanVector {
    friend class SpanRider<C>;

public:

    SpanVector(
        _In_ C defaultValue
    ) :
        m_default (defaultValue),
        m_cSpans  (0)
    {}


    /**************************************************************************\
    *
    * template <class C> void SpanVector<C>::SetSpan:
    *
    *   Update span vector with an Element over a range
    *
    * Arguments:
    *
    *   IN    first   - first character having this attribute
    *   IN    length  - number of characters having this attribute
    *   IN    Element - attribute to record for this range
    *
    * Return Value:
    *
    *   none
    *
    * Algorithm
    *
    *   Identify first and last existing Spans affected by the change
    *   Where status adjacent to the change is the same as the change
    *   update the change range to include adjacent equal value.
    *   Calculate how many Spans need to be added or removed.
    *   Insert null Spans or delete Spans after first affected span.  The first
    *   affected span may be updated, but is never removed.
    *
    *
    \**************************************************************************/

    _Check_return_ HRESULT SetSpan(
        XUINT32  first,
        XUINT32  length,
        _In_ C   newElement
    )
    {
        C        trailingElement = NULL;
        XUINT32  trailingLength = 0;


        // Identify first span affected by update

        XUINT32 fs = 0;  // First affected span index
        XUINT32 fc = 0;  // Character position at start of first affected span
        XUINT32 ls;
        XUINT32 lc;
        XINT32 spanDelta;

        while (    fs < m_cSpans
               &&  fc + m_spans[fs].m_cCharacters <= first)
        {
            fc += m_spans[fs].m_cCharacters;
            fs++;
        }


        // If the span list terminated before first, just add the new span

        if (fs >= m_cSpans)
        {
            // Ran out of Spans before reaching first

            ASSERT(fc <= first);

            if (fc < first)
            {
                // Create default run up to first
                IFC_RETURN(Add(Span<C>(m_default, first-fc)));
            }

            if (    m_cSpans > 0
                &&  m_spans[m_cSpans-1].m_element == newElement)
            {
                // New Element matches end Element, just extend end Element
                m_spans[m_cSpans-1].m_cCharacters += length;
            }
            else
            {
                IFC_RETURN(Add(Span<C>(newElement, length)));
            }

            return S_OK; // All done OK. Early out.
        }


        // fs = index of first span partly or completely updated
        // fc = character index at start of fs

        // Now find the last span affected by the update

        ls = fs;
        lc = fc;

        while (    ls < m_cSpans
               &&  lc + m_spans[ls].m_cCharacters <= first+length)
        {
            lc += m_spans[ls].m_cCharacters;
            ls++;
        }


        // ls = first span following update to remain unchanged in part or in whole
        // lc = character index at start of ls

        // expand update region backwards to include existing spans of identical
        // element type

        if (first == fc)
        {
            // Item at [fs] is completely replaced. Check prior item

            if (    fs > 0
                &&  m_spans[fs-1].m_element == newElement)
            {
                // Expand update backward to include previous run of equal
                // classification
                ASSERT(fs>0);
                fs--;
                ASSERT(fc>=m_spans[fs].m_cCharacters);
                fc -= m_spans[fs].m_cCharacters;
                first = fc;
                length += m_spans[fs].m_cCharacters;
            }

        }
        else
        {
            // Item at [fs] is partially replaced. Check if it is same as update
            if (m_spans[fs].m_element == newElement)
            {
                // Expand update area back to start of first affected equal valued run
                ASSERT(first+length >= fc);
                length = first+length-fc;
                first = fc;
            }
        }


        // Expand update region forwards to include existing Spans of identical
        // Element type

        if (    ls < m_cSpans
            &&  m_spans[ls].m_element == newElement)
        {
            // Extend update region to end of existing split run

            length = lc + m_spans[ls].m_cCharacters - first;
            lc += m_spans[ls].m_cCharacters;
            ls++;
        }


        // If no old spans remain beyond area affected by update, handle easily:

        if (ls >= m_cSpans)
        {
            // None of the old span list extended beyond the update region

            if (fc < first)
            {
                // Updated region leaves some of [fs]

                if (m_cSpans != fs+2)
                {
                    IFC_RETURN(m_spans.MakeRoom(fs+2));
                    m_cSpans = fs+2;
                }
                ASSERT(first >= fc);
                m_spans[fs].m_cCharacters = first - fc;
                m_spans[fs+1] = Span<C>(newElement, length);
            }
            else
            {
                // Updated item replaces [fs]

                if (m_cSpans != fs+1)
                {
                    IFC_RETURN(m_spans.MakeRoom(fs+1));
                    m_cSpans = fs+1;
                }
                m_spans[fs] = Span<C>(newElement, length);
            }

            return S_OK; // All done OK. Early out.
        }


        // Record partial elementtype at end, if any

        if (first+length > lc)
        {
            trailingElement = m_spans[ls].m_element;
            ASSERT(lc + m_spans[ls].m_cCharacters >= first + length);
            trailingLength  = lc + m_spans[ls].m_cCharacters - (first + length);
        }


        // Calculate change in number of Spans

        ASSERT(ls >= fs);
        spanDelta        =  1                          // The new span
                         +  (first  > fc ? 1 : 0)      // part span at start
                         -  (ls-fs);                   // existing affected span count

        // Note part span at end doesn't affect the calculation - the run may need
        // updating, but it doesn't need creating.

        if (spanDelta < 0)
        {
            IFC_RETURN(Erase(fs + 1, -spanDelta));
        }
        else if (spanDelta > 0)
        {
            IFC_RETURN(Insert(fs + 1, spanDelta));
            // Initialize inserted Spans
            for (XINT32 i=0; i<spanDelta; i++)
            {
                m_spans[fs+1+i] = Span<C>(NULL, 0);
            }
        }


        // Assign Element values

        // Correct length of split span before updated range

        if (fc < first)
        {
            m_spans[fs].m_cCharacters = first-fc;
            fs++;
        }

        // Record Element type for updated range

        m_spans[fs] = Span<C>(newElement, length);
        fs++;

        // Correct length of split span following updated range

        if (lc < first+length)
        {
            m_spans[fs] = Span<C>(trailingElement, trailingLength);
        }

        // Phew, all done ....

        return S_OK;
    }



    const C &GetDefault()   const {return m_default;}

    XUINT32  GetSpanCount() const {return m_cSpans;}

    const Span<C> &operator[] (XUINT32 i) const
    {
        ASSERT(i < m_cSpans);
        return m_spans[i];
    }



    Span<C> &operator[] (XUINT32 i)
    {
        ASSERT(i < m_cSpans);
        return m_spans[i];
    }



    void Reset(XINT32 bShrink = FALSE)
    {
        m_cSpans = 0;
        if (bShrink)
        {
            // Eat the return value - there's no possibility of error
            // for shrinking to size zero
            HRESULT hr = m_spans.Shrink(0);
            ASSERTSUCCEEDED(hr);
            (void)hr;
        }
    }



    _Check_return_ HRESULT SetSpanCount(XUINT32 cSpans)
    {
        // We only support removal of spans
        if (cSpans > m_cSpans)
        {
            return E_FAIL;
        }
        else
        {
            m_cSpans = cSpans;
            return S_OK;
        }
    }



private:

    _Check_return_ HRESULT Erase(XUINT32 first, XUINT32 count)
    {
        if (    first + count >= m_cSpans
            &&  first < m_cSpans)
        {
            // Erase at end
            IFC_RETURN(m_spans.MakeRoom(first));
            m_cSpans = first;
        }
        else
        {
            ASSERT(m_cSpans > first+count);
            memmove(
                &m_spans[first],
                &m_spans[first+count],
                sizeof(Span<C>) * (m_cSpans - (first+count))
            );
            IFC_RETURN(m_spans.MakeRoom(m_cSpans-count));
            ASSERT(m_cSpans >= count);
            m_cSpans -= count;
        }

        return S_OK;
    }



    _Check_return_ HRESULT Insert(XUINT32 first, XUINT32 count)
    {
        if (first >= m_cSpans)
        {
            // All new entries are beyond existing entries
            IFC_RETURN(!m_spans.MakeRoom(first+count));
            m_cSpans = first+count;
        }
        else
        {
            // Make room for <count> more entries, and move all entries from
            // first to the old end up to the new end.

            ASSERT(m_cSpans > first);
            XUINT32 amountToMove = m_cSpans-first;

            IFC_RETURN(m_spans.MakeRoom(m_cSpans+count));

            m_cSpans += count;
            memmove(&m_spans[first+count], &m_spans[first], sizeof(Span<C>) * amountToMove);
        }

        return S_OK;
    }



    _Check_return_ HRESULT Add(const Span<C> &newSpan)
    {
        IFC_RETURN(m_spans.MakeRoom(m_cSpans+1));

        m_cSpans++;
        m_spans[m_cSpans-1] = newSpan;

        return S_OK;
    }




    C                     m_default;
    SpanBase< Span< C > > m_spans;
    XUINT32               m_cSpans;
};





//------------------------------------------------------------------------
//
//  Class:  SpanRider - a class for efficiently running a cursor along
//                      a span vector
//
//
//------------------------------------------------------------------------

template <class C> class SpanRider {

public:

    SpanRider(SpanVector<C>  *pSpans) :
        m_pSpanVector         (pSpans),
        m_defaultSpan         (pSpans->m_default, 0xFFFFFFFF),
        m_currentOffset       (0),
        m_currentElement      (0),
        m_currentElementIndex (0)
    {}

    _Check_return_ HRESULT SetPosition(XUINT32 newOffset)
    {
        #if DBG
            // Check that current position details are valid

            if (newOffset > 0)
            {
                XUINT32 offset = 0;
                XUINT32 element = 0;
                while (    offset < m_currentElementIndex
                       &&  element < m_pSpanVector->m_cSpans)
                {
                    offset += m_pSpanVector->m_spans[element].m_cCharacters;
                    element++;
                }

                ASSERT(element <= m_pSpanVector->m_cSpans);
                ASSERT(element == m_currentElement);
                ASSERT(offset == m_currentElementIndex);

                if (element < m_pSpanVector->m_cSpans)
                {
                    // m_currentOffset is somewhere before the end of the run containing
                    // the current position, or is 0 if just initialized.
                    ASSERT((m_currentOffset < offset + m_pSpanVector->m_spans[element].m_cCharacters)
                        || (m_currentOffset == 0));
                }
            }
        #endif


        if (newOffset < m_currentElementIndex)
        {
            // Need to start at the beginning again
            m_currentOffset       = 0;
            m_currentElement      = 0;
            m_currentElementIndex = 0;
        }

        // Advance to element containing new offset

        while (    m_currentElement < m_pSpanVector->m_cSpans
               &&  (    m_pSpanVector->m_spans[m_currentElement].m_cCharacters == 0   // Skip over empty elements
                    ||  m_currentElementIndex + m_pSpanVector->m_spans[m_currentElement].m_cCharacters <= newOffset))
        {
            m_currentElementIndex += m_pSpanVector->m_spans[m_currentElement].m_cCharacters;
            m_currentElement++;
        }

        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            m_currentOffset = newOffset;
            return S_OK;
        }
        else
        {
            m_currentOffset = Min(newOffset, m_currentElementIndex);
            return E_FAIL;
        }
    }

    XUINT32 IsAtEnd()
    {
        return m_currentElement >= m_pSpanVector->m_cSpans;
    }

    void operator++(int)
    {
        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            m_currentElementIndex += m_pSpanVector->m_spans[m_currentElement].m_cCharacters;
            m_currentElement++;
        }
    }

    C &operator[] (XUINT32 offset)
    {
        if (    m_pSpanVector->m_cSpans > 0
            &&  SetPosition(offset))
        {
            return m_pSpanVector->m_spans[m_currentElement].m_element;
        }
        else
        {
            return m_pSpanVector->m_default;
        }
    }

    const Span<C> &GetCurrentSpan() const
    {
        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            return m_pSpanVector->m_spans[m_currentElement];
        }
        else
        {
            return m_defaultSpan;
        }
    }

    const C &GetCurrentElement() const
    {
        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            return m_pSpanVector->m_spans[m_currentElement].m_element;
        }
        else
        {
            return m_pSpanVector->m_default;
        }
    }

    C &GetCurrentElement()
    {
        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            return m_pSpanVector->m_spans[m_currentElement].m_element;
        }
        else
        {
            return m_pSpanVector->m_default;
        }
    }

    const C &GetPrecedingElement() const
    {
        if (   m_currentElement > 0
            && m_currentElement <= m_pSpanVector->m_cSpans)
        {
            return m_pSpanVector->m_spans[m_currentElement - 1].m_element;
        }
        else
        {
            return m_pSpanVector->m_default;
        }
    }

    XUINT32 GetCurrentOffset() const {return m_currentOffset;}

    XUINT32 GetUniformLength() const {
        if (m_currentElement < m_pSpanVector->m_cSpans)
        {
            return m_currentElementIndex + m_pSpanVector->m_spans[m_currentElement].m_cCharacters - m_currentOffset;
        }
        else
        {
            return 0xFFFFFFFF;  // There's no limit to this span
        }
    }

    XUINT32 GetCurrentSpanStart() const {return m_currentElementIndex;}


    // SpanRider has its own SetSpan that must be used to guarantee that
    // the rider remains accurate.

    _Check_return_ HRESULT SetSpan(XUINT32 first, XUINT32 length, C element)
    {
        // Position the rider at the beginning. This change will require
        // the rider to be recalculated.
        m_currentOffset       = 0;
        m_currentElement      = 0;
        m_currentElementIndex = 0;
        return m_pSpanVector->SetSpan(first, length, element);
    }


private:
    SpanVector<C> *m_pSpanVector;
    Span<C>        m_defaultSpan;
    XUINT32        m_currentOffset;
    XUINT32        m_currentElement;
    XUINT32        m_currentElementIndex;
};