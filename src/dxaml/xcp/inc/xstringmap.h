// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Trie-based string map
#pragma once

#include <intsafe.h>

enum vsNodeType
{
    vsNodeTypeInterior,
    vsNodeTypeTerminal,
    vsNodeTypePack
};

#define NODE_TYPE_INTERIOR 0
#define NODE_TYPE_TERMINAL 1
#define NODE_TYPE_PACK     2

#define NODE_MAX_CHARS     9

//------------------------------------------------------------------------
//
//  Struct: XNode
//
// The data structure for an unpacked node in a trie. Making this a POD
// structure to allow propert 'const' functionality for the statically
// built tries.
//
//------------------------------------------------------------------------

template <typename Ty>
class CNode;

namespace xstringmap_details
{
    template<typename Ty> struct is_pod
    {
        static const bool value =
               !__is_class(Ty)
            || (__is_pod(Ty) && __has_trivial_constructor(Ty));
    };
}

template <bool, typename Ty>
struct XNode_ValueOrNodePtr
{
    union
    {
        const CNode<Ty> *m_pcDown;
        CNode<Ty> *m_pDown;
        CNode<Ty> **m_ppPackedNodes;
        Ty m_value;
    };
};

template <typename Ty>
struct XNode_ValueOrNodePtr<false, Ty>
{
    union
    {
        const CNode<Ty> *m_pcDown;
        CNode<Ty> *m_pDown;
        CNode<Ty> **m_ppPackedNodes;
    };

    Ty m_value;
};

template <typename Ty>
struct XNode
{
    XNode_ValueOrNodePtr<xstringmap_details::is_pod<Ty>::value, Ty> m_valueOrNode;

    union
    {
        const CNode<Ty> *m_pcRight;
        CNode<Ty> *m_pRight;
    };

    XUINT16 m_cch      : 8;
    XUINT16 m_NodeType : 7;
    XUINT16 m_hasValue : 1;

    union
    {
        WCHAR   aChar[NODE_MAX_CHARS];

        struct
        {
            WCHAR chLast;
            WCHAR chFirst;

            // for a packed node, there is likely to be a large interval between
            // a node that contains a NULL at aChar[0] (which would be a terminal node with no
            // characters of its own)  and the first node that does not. Since we have
            // the space for it, reserve a special slot so that the NULL-containing
            // node doesn't skew the pack density.
            CNode<Ty> *pNodeZero;

        } PackInfo;
    };
};

//------------------------------------------------------------------------
//
//  Class:  CNode
//
//  Synopsis:
//      A node in an unpacked state of the trie
//
//------------------------------------------------------------------------

#define PACK_THRESHOLD  4
#define PACK_DENSITY_THRESHOLD 0.0625

template <typename Ty>
class CNode : public XNode<Ty>
{
public:
    CNode()
    {
        this->m_valueOrNode.m_pcDown = nullptr;
        this->m_valueOrNode.m_value = Ty();
        this->m_pcRight = nullptr;
        this->m_NodeType = NODE_TYPE_INTERIOR;
        this->m_cch = 0;
        this->m_hasValue = false;
        memset(this->aChar, 0, sizeof(this->aChar));
    }

   ~CNode()
    {
        if (this->m_NodeType == NODE_TYPE_PACK && this->m_valueOrNode.m_ppPackedNodes)
        {
            delete [] this->m_valueOrNode.m_ppPackedNodes;
            this->m_valueOrNode.m_ppPackedNodes = nullptr;
        }
    }

public:
    CNode<Ty>* getDown()            { return this->m_NodeType == NODE_TYPE_INTERIOR ? this->m_valueOrNode.m_pDown : NULL; }
    CNode<Ty>* getRight()           { return this->m_pRight; }
    const Ty& getValue()            { return this->m_valueOrNode.m_value; }
    vsNodeType getNodeType()        { return (vsNodeType)this->m_NodeType; }

    _Ret_writes_(PackInfo.chLast - PackInfo.chFirst + 1)
        CNode<Ty>** getPackedNodes()    { return this->m_valueOrNode.m_ppPackedNodes; }

    WCHAR getFirstChar()            { return this->aChar[0]; }

    __range(0, NODE_MAX_CHARS)
    XUINT32 getCount()              { return this->m_cch; }

    void setDown(CNode<Ty>* pNode)      { this->m_valueOrNode.m_pDown = pNode; }

    void setRight(CNode<Ty>* pNode)     { this->m_pRight = pNode; }

    void setValue(const Ty& value)      { this->m_valueOrNode.m_value = value; this->m_hasValue = true; }

    void clearValue()                   { this->m_valueOrNode.m_value = Ty(); this->m_hasValue = false; }

    bool hasValue() const               { return this->m_hasValue; }

    void setNodeType(vsNodeType nodeType)       { this->m_NodeType = (XUINT8)nodeType; }

    void setPackedNodes(CNode<Ty> **ppNodes)    { this->m_valueOrNode.m_ppPackedNodes = ppNodes; }

    static _Check_return_ HRESULT Make(
        _Out_ CNode<Ty>*& allocatedNode
        )
    {
        allocatedNode = new CNode<Ty>();
        return S_OK; //RRETURN_REMOVAL
    }

    // Split node at specified index.  If ichSplit > m_cch-1, then
    // an empty terminal node will be split off.
    _Check_return_ HRESULT SplitAt(
        _In_range_(0, NODE_MAX_CHARS) XUINT32 ichSplit
        )
    {
        HRESULT hr = S_OK;
        CNode<Ty> *pNode = NULL;
        XUINT32 ichLast = 0;
        XUINT32 bForceTerminal = 0;
        XUINT32 cchRemaining = 0;

        // If count is zero, we're about to walk off into random memory.
        // Fix bug upstream that built this invalid node.  In the meantime, halt with E_UNEXPECTED.
        IFCEXPECT(getCount() > 0);

        ichLast = getCount() - 1;
        bForceTerminal = ichSplit > ichLast;
        cchRemaining = bForceTerminal ? 0 : ichLast - ichSplit + 1;

        IFC(CNode<Ty>::Make(pNode));

        IFCEXPECT(ichSplit <= sizeof(this->aChar) / sizeof(this->aChar[0]));

        if (cchRemaining)
        {
            for (XUINT32 i = ichSplit, j = 0; i <= ichLast; i++, j++)
            {
                pNode->aChar[j] = this->aChar[i];
            }
        }

        ASSERT(cchRemaining <= UINT8_MAX);
        pNode->m_cch = (XUINT8)cchRemaining;
        this->m_cch -= (XUINT8)cchRemaining;

        if (bForceTerminal && this->m_NodeType != NODE_TYPE_TERMINAL)
        {
            pNode->m_NodeType = NODE_TYPE_TERMINAL;
            pNode->m_pRight = this->m_valueOrNode.m_pDown;
        }
        else
        {
            pNode->m_NodeType = this->m_NodeType;
            pNode->m_valueOrNode.m_pDown = this->m_valueOrNode.m_pDown;

            if (this->m_hasValue)
            {
                pNode->m_valueOrNode.m_value = std::move(this->m_valueOrNode.m_value);
                pNode->m_hasValue = true;

                this->m_hasValue = false; // moved
            }
        }

        this->m_NodeType = NODE_TYPE_INTERIOR;
        this->m_valueOrNode.m_pDown = pNode;

    Cleanup:
        RRETURN(hr);
    }

    // Removes characters from the node.
    _Check_return_ HRESULT RemoveChars(_In_range_(0, NODE_MAX_CHARS - 1) XUINT32 ichStart,
                                      _In_range_(0, NODE_MAX_CHARS - ichStart)  XUINT32 cchCount)
    {
        if ( (XUINT64)ichStart + cchCount > getCount())
            RRETURN(E_UNEXPECTED);

        for (XUINT32 i = ichStart, j = ichStart + cchCount; j < getCount(); i++, j++)
        {
            this->aChar[i] = this->aChar[j];
        }

        ASSERT(cchCount < UINT8_MAX);
        this->m_cch -= (XUINT8)cchCount; // Cast is safe due to check against getCount

        if (m_cch == 0)
            this->aChar[0] = NULL;

        RRETURN(S_OK);
    }
};

template <typename Ty>
class xstringmap_trie_base
{
protected:
    bool m_bIgnoreCase = false;

protected:
    xstringmap_trie_base() = default;

    // Places a named value in the trie
    _Check_return_ HRESULT
    PutValue(
        _Inout_ CNode<Ty> *pHead,
        _In_range_( >, 0)  XUINT32 cName,
        _In_reads_(cName) const WCHAR *pName,
        _In_ const Ty& value
    )
    {
         HRESULT hr = E_FAIL;

         CNode<Ty> *pPrefix = pHead;
         CNode<Ty> *pState = pPrefix->getDown();
         CNode<Ty> *pPrev = NULL;
         CNode<Ty> *pNode = NULL;

         WCHAR chSeek;

         // Ignore trailing nulls.
         while (cName > 0 && !pName[cName-1])
         {
            cName--;
            ASSERT( cName == 0 || pName[cName-1] ); // pName has more than one null termination character.  This is tolerated, but should be fixed upstream.
         }

        //
        // Prefast can't verify the bounds checking in this loop and complains about
        // various "chSeek = *pName & (m_bIgnoreCase &&  ..." statements.  Note that every
        // increment of pName++ is accompanied by a cName--, and immediately before every
        // "chSeek = *pName ..." is a while(cName ...) of some variety.
        //
#pragma warning (push)
#pragma warning (disable : 26014)

         while (cName)
         {

            chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);

            // If this state is packed then we must fail as packing assumes
            // that the trie is no longer going to have insertions.
            if (pState && pState->m_NodeType == NODE_TYPE_PACK)
            {
                return E_FAIL;
            }

            // The general case requires walking the list of nodes
            pPrev = NULL;

            while (pState && (chSeek > pState->getFirstChar()))
            {
                pPrev = pState;
                pState = pState->getRight();
            }

            // If we didn't find a node or it didn't match then insert a new node between
            // the previous and current nodes.
            if (!pState || (chSeek != pState->getFirstChar()))
            {
                IFC(CNode<Ty>::Make(pNode));

                pNode->setRight(pState);

                // Try to link the previous node to this node or make it the new start of
                // this state.
                if (pPrev)
                {
                    pPrev->setRight(pNode);
                }
                else
                {
                    pPrefix->setDown(pNode);
                }

                while (cName)
                {
                    XUINT32 cchAdded = 0;
                    while ( cName && cchAdded < NODE_MAX_CHARS)
                    {
                        chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);
                        pNode->aChar[cchAdded] = chSeek;

                        cchAdded++;
                        cName--;
                        pName++;
                    }

                    pNode->m_NodeType = cName ? NODE_TYPE_INTERIOR : NODE_TYPE_TERMINAL;
                    pNode->m_cch = (XUINT8)cchAdded; // Cast is safe due to NODE_MAX_CHARS bound

                    if (cName)
                    {
                        pPrefix = pNode;

                        IFC(CNode<Ty>::Make(pNode));

                        pPrefix->setDown(pNode);
                    }
                }

                pState = pNode;
            }
            else
            {
                // found a candidate, but let's see how it pans out...
                cName--;
                pName++;

                XUINT32 i = 1;

                while (cName && i < pState->getCount())
                {
                    chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);

                    if (pState->aChar[i] != chSeek)
                    {
                        break;
                    }

                    i++;
                    cName--;
                    pName++;
                }

#pragma warning(pop) //(disable : 26014)

                if (cName)
                {
                    // Haven't found it, but we are at a terminal node
                    // Split the node so that we can continue on with
                    // interior nodes
                    if (pState->m_NodeType == NODE_TYPE_TERMINAL || i < pState->getCount())
                    {
                        IFC(pState->SplitAt(i));
                    }
                }
                else
                {
                    // Found it, but need to check if we are in the middle of a node
                    // or at an interior node. We may need to either find or create an
                    // empty terminal node.

                    // if we didn't reach the end
                    if (i != pState->getCount())
                    {
                        // First call to split at will split the node into
                        // two nodes with pState becoming (or staying) an
                        // interior node.
                        IFC(pState->SplitAt(i));

                        // The second split will cause
                        // an empty terminal node to be created after pState
                        IFC(pState->SplitAt(i));

                        pState = pState->getDown();
                    }
                    else if (pState->m_NodeType == NODE_TYPE_INTERIOR)
                    {
                        if (!(pState->getDown()->m_NodeType == NODE_TYPE_TERMINAL && pState->getDown()->getCount() == 0))
                        {
                            // we did reach the end, but can't set the value
                            // on an interior node.  Create an empty terminal
                            // node.
                            IFC(pState->SplitAt(i));
                        }

                        pState = pState->getDown();
                    }
                }
            }

            if (cName)
            {
                pPrefix = pState;
                pState = pState->getDown();
            }
            else
            {
                hr = pState->hasValue() ? S_FALSE : S_OK;
                pState->setValue(value);
                break;
            }
         }

    Cleanup:
        RRETURN(hr);
    }

    _Ret_maybenull_
    CNode<Ty>*
    FindState(
        _In_opt_ CNode<Ty> *pState,
        _In_range_( >, 0)  XUINT32 cName,
        _In_reads_(cName) const WCHAR *pName
        ) const
    {
        // Ignore trailing nulls.
        while (cName > 0 && !pName[cName-1])
        {
           cName--;
           ASSERT( cName == 0 || pName[cName-1] ); // pName has more than one null termination character.  This is tolerated, but should be fixed upstream.
        }

        //
        // Prefast can't verify the bounds checking in this loop and complains about
        // various "chSeek = *pName ..." statements.  Note that every
        // increment of pName++ is accompanied by a cName--, and immediately before every
        // "chSeek = *pName ..." is either an if(cName) or while(cName ...) of some variety.
        //
#pragma warning (push)
#pragma warning (disable : 26014)

        while (cName)
        {
            // If this state is packed then we can just index the node from the character.
            // All nodes in a packed state are the same size and always have space for a
            // value and a down pointer at each node.

            if (pState && pState->m_NodeType == NODE_TYPE_PACK)
            {
                WCHAR chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);

                if (pState->PackInfo.chFirst > chSeek || pState->PackInfo.chLast < chSeek)
                {
                    return nullptr;
                }

                pState = pState->getPackedNodes()[chSeek - pState->PackInfo.chFirst];

                pName++;
                cName--;
            }

            if (!pState)
            {
                return nullptr;
            }

            if (cName)
            {
                // The general case requires walking the list of nodes
                WCHAR chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);

                while (pState && (chSeek > pState->getFirstChar()))
                {
                    pState = pState->getRight();
                }

                // If we went past the last node in the state or the character we're seeking
                // for isn't in the state return failure

                if (!pState)
                {
                    return nullptr;
                }
                else
                {
                    for (XUINT32 i = 0; i < pState->getCount(); i++)
                    {
                        if (!cName)
                        {
                            return nullptr;
                        }

                        chSeek = *pName & (m_bIgnoreCase && xisalpha(*pName) ? 0x00df : 0xffff);

                        if (pState->aChar[i] != chSeek)
                        {
                            return nullptr;
                        }

                        cName--;
                        pName++;
                    }
                }
            }

#pragma warning (pop) // 26014

            // If we're not at the end of the name try to follow the down pointer.
            if (!pState)
            {
                return nullptr;
            }
            else if (cName)
            {
                // Follow the down pointer or return failure
                pState = pState->getDown();
                if (pState)
                {
                    continue;
                }
                else
                {
                    return nullptr;
                }
            }
            else
            {
                if (pState->m_NodeType == NODE_TYPE_TERMINAL)
                {
                    return pState;
                }
                else
                {
                    pState = pState->getDown();
                    if (pState && pState->m_NodeType == NODE_TYPE_TERMINAL && pState->aChar[0] == NULL)
                    {
                        return pState;
                    }
                    return nullptr;
                }
            }
        }

        // if we managed to get here without having found a success condition,
        // then something is wrong.
        return nullptr;
    }

    // Helper function for DeleteState. Function walks the tree to
    // find the rightmost node having right pointer NULL. Once found its going to
    // add the pInsertNode subtree to that position
    void InsertIntoRightmost(
        _In_ CNode<Ty> **pPreviousRightmost,
        _In_ CNode<Ty> *pInsertNode
        )
    {
        ASSERT(pPreviousRightmost);

        while ((*pPreviousRightmost)->getRight())
        {
            *pPreviousRightmost = (*pPreviousRightmost)->getRight();
        }

        (*pPreviousRightmost)->setRight(pInsertNode);

        // this isn't *really* the rightmost, but we know that we
        // can at least advance by this much.
        if (pInsertNode)
        {
            *pPreviousRightmost = pInsertNode;
        }
    }

    // Deletes all the nodes in a state
    void
    DeleteState(
        _In_opt_ CNode<Ty> *pState
        )
    {
        CNode<Ty> *pRightmost = pState;

        while (pState)
        {
            if (pState && (pState->m_NodeType == NODE_TYPE_PACK))
            {
                CNode<Ty> **ppPackedNodes = pState->getPackedNodes();
                XUINT32 cNode = pState->PackInfo.chLast >= pState->PackInfo.chFirst ?
                                        pState->PackInfo.chLast - pState->PackInfo.chFirst + 1 : 0;

                if (pState->PackInfo.pNodeZero)
                {
                    InsertIntoRightmost(&pRightmost, pState->PackInfo.pNodeZero);
                }

                for (XUINT32 i = 0; i < cNode; i++)
                {
                    if (ppPackedNodes[i])
                    {
                        InsertIntoRightmost(&pRightmost, ppPackedNodes[i]);
                    }
                }
            }
            else
            {
                // check if the pState node has a node in its Down pointer.
                // if yes then pull its down node out and put it into the right
                // pointer of the pState node's rightmost node
                if(pState->getDown())
                {
                     InsertIntoRightmost(&pRightmost, pState->getDown());
                     pState->setDown(nullptr);
                }
            }

            // now pState has NULL in its m_pDown
            // lets delete pState and start the process again.
            auto pTemp = pState;
            pState = pState->getRight();

            // if for some reason the pRightmost is the recently departed
            // then set pRightmost to pState (which will either be pTemp's
            // right pointer, or NULL)
            if (pRightmost == pTemp)
            {
                pRightmost = pState;
            }

            delete pTemp;
        }
    }

    // Attempts to optimize a state by packing it.  A packed state has the smallest
    // number of nodes that will encompas all characters at a certain level.
    // This allows us to directly index a node rather scan for it which is
    // much faster but it takes more memory.
    _Check_return_ HRESULT
    Optimize(
        _In_ CNode<Ty> *pState,
        _Outptr_ CNode<Ty> **ppOptimizedState
        )
    {
        HRESULT hr;
        CNode<Ty> *pTemp;
        CNode<Ty> *pCurrent;
        CNode<Ty> **ppPackedNodes = NULL;
        CNode<Ty> *pPack = NULL;
        XUINT32 cNode;
        XUINT32 i;

        // While counting and validating the nodes in this state we can attempt to
        // optimize the down pointers.

        cNode = 0;

        WCHAR chLow = 0;
        WCHAR chHigh = 0;
        WCHAR chCurrent;
        pCurrent = pState;

        while (pCurrent)
        {
            cNode++;
            chCurrent = pCurrent->getFirstChar();

            // don't include the possible empty
            // terminal node in this so as not to
            // skew the pack density.
            if (chCurrent != NULL)
            {
                if (chCurrent < chLow)
                    chLow = chCurrent;

                if (chCurrent > chHigh)
                    chHigh = chCurrent;
            }

            pCurrent = pCurrent->getRight();
        }

        // If there aren't enough nodes or the nodes arent at a density to make it worth the trouble.
        if ( (cNode < PACK_THRESHOLD) || (( (XDOUBLE)cNode / (chHigh - chLow) ) < PACK_DENSITY_THRESHOLD))
        {
            *ppOptimizedState = NULL;
            return S_OK;
        }

        // Try to allocate memory for the packed state.
        IFC(CNode<Ty>::Make(pPack));

        pPack->setNodeType(vsNodeTypePack);

        ppPackedNodes = new CNode<Ty>* [chHigh - chLow + 1];

        for (i = 0; i < (XUINT32)(chHigh - chLow + 1); i++)
            ppPackedNodes[i] = NULL;

        pPack->setPackedNodes(ppPackedNodes);
        pPack->PackInfo.chFirst = chLow;
        pPack->PackInfo.chLast = chHigh;

        // If we fail to allocate memory we don't need to return failure we just won't
        // pack this state.
        if (NULL == pPack)
            return S_OK;

        // We can now pack the state. As Mr. Burns would say, "Excellent."
        // if the first state is an empty terminal node, put it in the special slot
        pCurrent = pState;
        if (pCurrent->getFirstChar() == NULL)
        {
            pPack->PackInfo.pNodeZero = pCurrent;

            pCurrent = pCurrent->getRight();
        }

        // supress the warning that we would get about the possibility
        // of chCurrent - chLow not being within chHigh - chLow.  PREFAST doesn't
        // realize that ppPackedNodes was allocated using the lowest and highest
        // chCurrents of the same set of nodes as we are iterating over here. (see
        // the while() loop at the begining of the function.
        #pragma warning (push)
        #pragma warning (disable : 26010 26053)

        while (pCurrent)
        {
            chCurrent = pCurrent->getFirstChar();

            ASSERT(chCurrent >= chLow && (chCurrent - chLow) >= 0);

            if (pCurrent->m_NodeType == NODE_TYPE_INTERIOR && pCurrent->m_cch == 1)
            {
                ASSERT(pCurrent->getDown());

                // if we are going to end up with an empty interior node,
                // then we might as well point past it.
                ppPackedNodes[chCurrent - chLow] = pCurrent->getDown();

                pTemp = pCurrent;
                pCurrent = pCurrent->getRight();

                delete pTemp;
            }
            else
            {
                ppPackedNodes[chCurrent - chLow] = pCurrent;
                IFC(pCurrent->RemoveChars(0, 1));

                pTemp = pCurrent;
                pCurrent = pCurrent->getRight();
                pTemp->setRight(NULL);
            }
        }

        #pragma warning (pop) // 26010

       *ppOptimizedState = pPack;
        hr = S_OK;

    Cleanup:
        RRETURN(hr);
    }

    // Walks the entire collection, and calls pCallback on all terminal
    // nodes.  strString refers to the characters before the current node
    // and pExtraData is an extra data value to use with the callback.
    template <typename Fn>
    _Check_return_ HRESULT
    Traverse(
        _In_ CNode<Ty> *pNode,
        _In_ const xstring_ptr& strString,
        _In_ Fn callback,
        _In_ XHANDLE pExtraData)
    {
        HRESULT hr = S_OK;
        xstring_ptr strCurrentString;

        if (!pNode || pNode->m_NodeType == NODE_TYPE_PACK)
        {
            IFC(E_FAIL);
        }

        if (pNode->getCount() > 0)
        {
            IFC(xstring_ptr::Concatenate(
                strString,
                0,
                XSTRING_PTR_EPHEMERAL2(pNode->aChar, pNode->getCount()),
                0,
                &strCurrentString));
        }
        else
        {
            strCurrentString = strString;
        }
        if (pNode->getDown())
        {
            // Head to down node and add current characters to the string.
            IFC(Traverse(pNode->getDown(), strCurrentString, callback, pExtraData));
        }
        if (pNode->m_NodeType == NODE_TYPE_TERMINAL && pNode->hasValue())
        {
            // This is a terminal node, so callback with the given data.
            (callback)(strCurrentString, pNode->getValue(), pExtraData);
        }
        if (pNode->getRight())
        {
            // Head to the right (siblings) with the original passed in string.
            IFC(Traverse(pNode->getRight(), strString, callback, pExtraData));
        }

    Cleanup:
        RRETURN(hr);
    }
};

template <typename Ty>
class xstringmap_base : public xstringmap_trie_base<Ty>
{
protected:
    CNode<Ty> *m_pTrie = nullptr;     // Top of trie
    bool m_bCompiled = false;     // Trie built at compile time

protected:
    xstringmap_base() = default;

    void ClearBase()
    {
        if (!m_bCompiled)
        {
            this->DeleteState(m_pTrie);
        }
        m_pTrie = nullptr;
    }

    _Check_return_ HRESULT PutValue(
        _In_reads_(cName) const WCHAR *pName,
        _In_range_(>, 0)  XUINT32 cName,
        _In_ const Ty& valueIn
        )
    {
        HRESULT hr = S_OK;

        if (!m_pTrie)
        {
            IFC(CNode<Ty>::Make(m_pTrie));
        }

        IFC(xstringmap_trie_base<Ty>::PutValue(m_pTrie, cName, pName, valueIn));

    Cleanup:
        RRETURN(hr);
    }

    bool GetValue(
        _In_reads_(cName) const WCHAR *pName,
        _In_range_(>, 0)  XUINT32 cName,
        _Out_ Ty& valueOut
        ) const
    {
        bool found = false;
        CNode<Ty>* pState = this->FindState(
            m_pTrie ? m_pTrie->getDown() : nullptr,
            cName,
            pName);

        if (pState && pState->hasValue())
        {
            valueOut = pState->getValue();
            found = true;
        }
        else
        {
            valueOut = Ty();
        }

        return found;
    }

    bool HasValue(
        _In_reads_(cName) const WCHAR *pName,
        _In_range_(>, 0)  XUINT32 cName
        ) const
    {
        CNode<Ty>* pState = this->FindState(
            m_pTrie ? m_pTrie->getDown() : nullptr,
            cName,
            pName);

        return nullptr != pState
            && pState->hasValue();
    }

    bool ClearValue(
        _In_reads_(cName) const WCHAR *pName,
        _In_range_( >, 0)  XUINT32 cName
        )
    {
        bool found = false;
        CNode<Ty>* pState = this->FindState(
            m_pTrie ? m_pTrie->getDown() : nullptr,
            cName,
            pName);

        if (pState && pState->hasValue())
        {
            pState->clearValue();
            found = true;
        }

        return found;
    }

    template <typename Fn>
    _Check_return_ HRESULT Traverse(
        _In_ Fn callback,
        _In_ XHANDLE pExtraData
        )
    {
        RRETURN(xstringmap_trie_base<Ty>::Traverse(
            m_pTrie ? m_pTrie->getDown() : nullptr,
            xstring_ptr::EmptyString(),
            callback,
            pExtraData));
    }
};

template <typename Ty>
class xstringmap : public xstringmap_base<Ty>
{
private:
    XUINT32 m_count;
    bool m_emptyKeyWasSet;
    Ty m_emptyKeyValue;
    Ty m_nullKeyValue; // need so that we can return a reference in Peek

private:
    // Poison constructors
    xstringmap<Ty>& operator=(const xstringmap<Ty>& other);
    xstringmap<Ty>(const xstringmap<Ty>& other);

public:
    xstringmap()
        : xstringmap_base<Ty>()
        , m_count(0)
        , m_emptyKeyWasSet(false)
        , m_emptyKeyValue()
        , m_nullKeyValue()
    {
    }

    ~xstringmap()
    {
        Clear();
    }
    void Clear()
    {
        this->ClearBase();

        m_emptyKeyWasSet = false;
        m_emptyKeyValue = Ty();

        m_count = 0;
    }

    HRESULT Add(
        _In_ const xstring_ptr_view& key,
        _In_ const Ty& valueIn
        )
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        RRETURN(Add(keyBuffer, keyCount, valueIn));
    }

    HRESULT Add(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount,
        _In_ const Ty& valueIn
        )
    {
        HRESULT hr = S_OK;

        ASSERT(!this->m_bCompiled);

        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || '\0' == keyBuffer[0])
        {
            m_emptyKeyValue = valueIn;

            if (!m_emptyKeyWasSet)
            {
                m_emptyKeyWasSet = true;
            }
            else
            {
                // Duplicate empty key.
                hr = S_FALSE;
            }
        }
        else
        {
            // Will return S_FALSE for duplicate entries
            hr = this->PutValue(keyBuffer, keyCount, valueIn);
        }

        if (hr == S_OK)
        {
            ++m_count;
        }

        RRETURN(hr);
    }

    bool Find(
        _In_ const xstring_ptr_view& key,
        _Out_ Ty& outData
        ) const
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        return Find(keyBuffer, keyCount, outData);
    }

    bool Find(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount,
        _Out_ Ty& valueOut
        ) const
    {
        bool found = false;

        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || L'\0' == keyBuffer[0])
        {
            if (m_emptyKeyWasSet)
            {
                valueOut = m_emptyKeyValue;
                found = true;
            }
            else
            {
                valueOut = Ty();
            }
        }
        else
        {
            found = this->GetValue(keyBuffer, keyCount, valueOut);
        }

        return found;
    }

    const Ty& Peek(
        _In_ const xstring_ptr_view& key
        )
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        return Peek(keyBuffer, keyCount);
    }

    const Ty& Peek(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount
        ) const
    {
        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || '\0' == keyBuffer[0])
        {
            if (m_emptyKeyWasSet)
            {
                return m_emptyKeyValue;
            }
        }
        else
        {
            CNode<Ty>* pState = FindState(
                m_pTrie ? m_pTrie->getDown() : nullptr,
                keyCount,
                keyBuffer);

            if (nullptr != pState && pState->hasValue())
            {
                return pState->getValue();
            }
        }

        return m_nullKeyValue;
    }

    bool ContainsKey(
        _In_ const xstring_ptr_view& key
        ) const
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        return ContainsKey(keyBuffer, keyCount);
    }

    bool ContainsKey(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount
        ) const
    {
        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || '\0' == keyBuffer[0])
        {
            return m_emptyKeyWasSet;
        }
        else
        {
            return this->HasValue(keyBuffer, keyCount);
        }
    }

    HRESULT Remove(
        _In_ const xstring_ptr_view& key
        )
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        RRETURN(Remove(keyBuffer, keyCount));
    }

    HRESULT Remove(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount
        )
    {
        HRESULT hr = S_FALSE;

        ASSERT(!this->m_bCompiled);

        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || '\0' == keyBuffer[0])
        {
            if (m_emptyKeyWasSet)
            {
                m_emptyKeyValue = Ty();
                m_emptyKeyWasSet = false;

                hr = S_OK;
            }
        }
        else
        {
            hr = this->ClearValue(keyBuffer, keyCount) ? S_OK : S_FALSE;
        }

        if (hr != S_FALSE)
        {
            ASSERT(m_count > 0);

            --m_count;
        }

        RRETURN(hr);
    }

    HRESULT Remove(
        _In_ const xstring_ptr_view& key,
        _Out_ Ty& valueOut
        )
    {
        XUINT32 keyCount;
        const WCHAR* keyBuffer = key.GetBufferAndCount(&keyCount);

        RRETURN(Remove(keyBuffer, keyCount, valueOut));
    }

    HRESULT Remove(
        _In_reads_opt_(keyCount) const WCHAR* keyBuffer,
        _In_ XUINT32 keyCount,
        _Out_ Ty& valueOut
        )
    {
        HRESULT hr = S_OK;

        ASSERT(!m_bCompiled);

        // The value store can't deal with empty keys
        if (   nullptr == keyBuffer
            || 0 == keyCount
            || '\0' == keyBuffer[0])
        {
            if (m_emptyKeyWasSet)
            {
                valueOut = std::move(m_emptyKeyValue);

                m_emptyKeyValue = Ty();
                m_emptyKeyWasSet = false;

                hr = S_OK;

            }
        }
        else
        {
            hr = GetValue(keyBuffer, keyCount, valueOut);

            if (hr != S_FALSE)
            {
                ClearValue(keyBuffer, keyCount);
            }
        }

        if (hr != S_FALSE)
        {
            ASSERT(m_count > 0);

            --m_count;
        }

        RRETURN(hr);
    }

    XUINT32 Count() const
    {
        return m_count;
    }


    //--------------------------------------------------------------------------
    //
    //  Methods exposed for CValueStore
    //
    //--------------------------------------------------------------------------

    template <typename Fn>
    _Check_return_ HRESULT
    Traverse(
        _In_ Fn callback,
        _In_ XHANDLE pExtraData = nullptr
        )
    {
        HRESULT hr = S_OK;

        if (m_emptyKeyWasSet)
        {
            (callback)(xstring_ptr::EmptyString(), m_emptyKeyValue, pExtraData);
        }

        if (m_pTrie)
        {
            IFC(xstringmap_trie_base::Traverse(
                m_pTrie->getDown(),
                xstring_ptr::EmptyString(),
                callback,
                pExtraData));
        }

    Cleanup:
        RRETURN(hr);
    }

    _Check_return_ HRESULT Optimize()
    {
        ASSERT(!m_bCompiled);

        CNode<Ty> *pOptimized = NULL;

        HRESULT hr = xstringmap_trie_base::Optimize(m_pTrie->getDown(), &pOptimized);

        if (SUCCEEDED(hr))
        {
            m_pTrie->setDown(pOptimized);
        }

        RRETURN(hr);
    }

    void putIgnoreCase(bool bIgnoreCase)
    {
        m_bIgnoreCase = bIgnoreCase;
    }

    void assignCompiled(const XNode<Ty>* pRoot)
    {
        m_pTrie =
            const_cast<CNode<Ty>*>(
                reinterpret_cast<const CNode<Ty>*>(
                    pRoot));

        m_bCompiled = true;

        //TODO: what about the count?
    }

    CNode<Ty>* getTrie() const
    {
        return m_pTrie;
    }
};

