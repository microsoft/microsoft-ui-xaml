// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template <typename TData>
class _xlist_node
{
public:
    _xlist_node(void)
        : m_next(NULL)
        , m_data(TData())
    {
    }

    _xlist_node(_xlist_node<TData>* next, const TData& data)
        : m_next(next)
        , m_data(data)
    {
    }

    ~_xlist_node(void)
    {
    }

    _xlist_node<TData>* m_next;
    TData m_data;
};

// Forward declaration of list.
template <typename TData>
class xlist;

template <typename TKey, typename TData>
class xpairlist 
    : public xlist< std::pair<TKey, TData> >
{
public:
    typedef std::pair<TKey, TData> TPair;
    HRESULT Get(const TKey& key, TData& outData) const
    {
        const _xlist_node<TPair>* node = this->m_head;
        
        while (node != NULL)
        {
            if (DataStructureFunctionProvider<TKey>::AreEqual(key, node->m_data.first))
            {
                outData = node->m_data.second;
                RRETURN(S_OK);
            }

            node = node->m_next;
        }  

        RRETURN(S_FALSE);
    }

    bool ContainsKey(const TKey& key) const
    {
        TData outData;
        return this->Get(key, outData) == S_OK;
    }

    HRESULT RemoveByKey(const TKey& key, TPair& outPair)
    {
        HRESULT hr = S_FALSE;
        _xlist_node<TPair>* node = this->m_head;
        _xlist_node<TPair>* prev = NULL;
        
        while (node != NULL)
        {
            if (DataStructureFunctionProvider<TKey>::AreEqual(key, node->m_data.first))
            {
                hr = S_OK;
                IFC(this->RemoveNextNode(prev, outPair)); 
                break;
            }

            prev = node;
            node = node->m_next;
        }

Cleanup:
        RRETURN(hr);
    }

    HRESULT RemoveByKey(const TKey& key, TData& outData)
    {
        HRESULT hr = S_FALSE;
        TPair removedPair;
        IFC(this->RemoveByKey(key, removedPair));
        outData = removedPair.second;
Cleanup:
        RRETURN(hr);
    }
};

template <typename TData>
class _xlist_const_iterator
{
public:
    _xlist_const_iterator()
        : m_Upto(NULL)
        , m_Container(NULL)
    {
        XCP_WEAK(&m_Container);
    }

    _xlist_const_iterator(_In_ _xlist_node<TData>* uptoNode, _In_ xlist<TData>* container)
        : m_Upto(uptoNode)
        , m_Container(container)
    {
        XCP_WEAK(&m_Container);
    }

    const TData& operator*() const        
    { 
        ASSERT(this->m_Upto != NULL); 
        return this->m_Upto->m_data;    
    }
    
    const TData* operator->() const       
    { 
        ASSERT(this->m_Upto != NULL); 
        return &**this;           
    }

    _xlist_const_iterator<TData>& operator++()   
    { 
        // ++preincrement
        ASSERT(this->m_Upto != NULL);
        this->m_Upto = this->m_Upto->m_next;
        return *this;
    }

    _xlist_const_iterator<TData> operator++(int)
    {
        // postincrement++
        _xlist_const_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }

    bool operator==(const _xlist_const_iterator<TData>& rhs) const
    {
        ASSERT(this->m_Container == rhs.m_Container);
        ////ASSERT(m_Container != NULL);
        return this->m_Upto == rhs.m_Upto;
    }

    bool operator!=(const _xlist_const_iterator<TData>& rhs) const
    {
        return !(*this == rhs);
    }

    _xlist_const_iterator<TData>& operator=(const _xlist_const_iterator<TData>& rhs)
    {    
        if (this != &rhs)
        {
            this->m_Upto = rhs.m_Upto;
            this->m_Container = rhs.m_Container;
        }

        return *this;
    }

    bool IsEnd()
    {
        return this->m_Upto == NULL;    
    }

protected:
    _xlist_node<TData>* m_Upto;
    xlist<TData>* m_Container;
};

template <typename TData>
class _xlist_iterator
    : public _xlist_const_iterator<TData>
{
public:
    _xlist_iterator()
    {
    }

    _xlist_iterator(_In_ _xlist_node<TData>* uptoNode, _In_ xlist<TData>* container)
        : _xlist_const_iterator<TData>(uptoNode, container)
    {
    }

    TData& operator*() const        
    { 
        ASSERT(this->m_Upto != NULL); 
        return this->m_Upto->m_data;    
    }
    
    TData* operator->() const       
    { 
        ASSERT(this->m_Upto != NULL); 
        return &**this;           
    }

    _xlist_iterator<TData>& operator++()
    {
        // ++preincrement (the cast to base const class is intentional)
        ++(*static_cast<_xlist_const_iterator<TData>*>(this));
        return *this;
    }

    _xlist_iterator<TData> operator++(int)
    {
        // postincrement++
        _xlist_iterator<TData> ret = *this;
        ++*this;
        return ret;
    }
};

//
// List structure.
//
// 0 items - head == tail, head == NULL, tail == NULL
// 1 item  - head == tail, tail != NULL, head != NULL
// 2+ items- head != tail, tail != NULL, head != NULL
//
template <typename TData>
class xlist
{
private:
    typedef void (xlist<TData>::*fAttachToEndDelegate)(_xlist_node<TData>* node);

    // Poison constructors
    xlist& operator=(const xlist& other);
     xlist(const xlist& other);
public:
    typedef _xlist_iterator<TData> iterator;
    typedef _xlist_const_iterator<TData> const_iterator;

    xlist(void) 
    {
        ResetMembers();
    }

    ~xlist(void)                    { clear();                              }
    HRESULT front(TData& val)       { return GetNodeData(this->m_head, val);      }
    HRESULT back(TData& val)        { return GetNodeData(this->m_tail, val);      }
    XUINT32 size()                  { return this->m_size;                        }
    bool empty()                   { return this->m_size == 0;                   }
    _xlist_iterator<TData> begin()                { return _xlist_iterator<TData>(this->m_head, this);        }
    _xlist_iterator<TData> end()                  { return _xlist_iterator<TData>(NULL, this);          }
    _xlist_const_iterator<TData> begin() const    { return _xlist_const_iterator<TData>(this->m_head, this);  }
    _xlist_const_iterator<TData> end() const      { return _xlist_const_iterator<TData>(NULL, this);    }

    void clear()
    {
        _xlist_node<TData>* current = this->m_head;
        _xlist_node<TData>* remove = NULL;
        
        while (current != NULL)
        {
            remove = current;
            current = current->m_next;
            delete remove;
        }

        this->ResetMembers();
    }

    HRESULT push_back(const TData& val)
    {
        return this->PushToEnd(&xlist<TData>::AppendToTail, this->m_tail, val);
    }

    HRESULT push_front(const TData& val)
    {
        return this->PushToEnd(&xlist<TData>::PrependToHead, this->m_head, val);
    }

    HRESULT pop_front()
    {
        HRESULT hr = S_OK;

        if (this->m_head == NULL)
        {
            // Can't pop empty queue.
            IFC(E_FAIL);
        }

        if (this->m_head == m_tail)
        {
            // Single item queue.
            delete this->m_head;
            this->SetBothEnds(NULL);
        }
        else
        {
            // More than one item in the queue and this is not the tail.
            delete this->InternalRemoveNode(&this->m_head);
        }

        DecrementSize();
Cleanup:
        RRETURN(hr);
    }

protected:
    HRESULT RemoveNextNode(_In_opt_ _xlist_node<TData>* node, TData& outData)
    {
        _xlist_node<TData>* nodeToRemove = InternalRemoveNode((node == NULL) ? &this->m_head : &(node->m_next));
        this->m_tail = (nodeToRemove->m_next == NULL) ? node : this->m_tail;
        outData = std::move(nodeToRemove->m_data);
        delete nodeToRemove;
        this->DecrementSize();
        RRETURN(S_OK);
    }

    HRESULT GetNodeData(_In_opt_ _xlist_node<TData>* node, TData& val)
    {
        if (node == NULL)
        {
            RRETURN(S_FALSE);
        }

        val = node->m_data;
        RRETURN(S_OK);
    }

    _xlist_node<TData>* InternalRemoveNode(_Outptr_ _xlist_node<TData>** ppNodeToRemove)
    {
        _xlist_node<TData>* nodeToRemove = *ppNodeToRemove;
        *ppNodeToRemove = nodeToRemove->m_next;
        return nodeToRemove;
    }

private:
    HRESULT PushToEnd(fAttachToEndDelegate attachmentFunction, _In_opt_ _xlist_node<TData>* endNode, const TData& val)
    {
        HRESULT hr = S_OK;
        _xlist_node<TData>* node = NULL;

        node = new _xlist_node<TData>(NULL, val);
        if (endNode == NULL)
        {
            attachmentFunction = &xlist<TData>::SetBothEnds;
        }

        ((*this).*(attachmentFunction))(node);
        this->IncrementSize();
        RRETURN(hr);//RRETURN_REMOVAL
    }

    void DecrementSize()
    {
        ASSERT(this->m_size != 0);
        ASSERT((this->m_size > 1) || ((this->m_head == NULL) && (this->m_tail == NULL)));
        ASSERT((this->m_size > 2) || (this->m_head == this->m_tail));
        this->m_size--;
    }

    void ZeroSize()
    {
        ASSERT(this->m_tail == NULL);
        ASSERT(this->m_head == NULL);
        this->m_size = 0;
    }

    void IncrementSize()
    {
        ASSERT(this->m_tail != NULL);
        ASSERT(this->m_head != NULL); 
        this->m_size++;
    }

    void ResetMembers()
    {
        this->SetBothEnds(NULL);
        this->ZeroSize();
    }

    void SetBothEnds(_In_opt_ _xlist_node<TData>* node)
    {
        ASSERT((node == NULL) || (node->m_next == NULL));
        this->m_head = node;
        this->m_tail = node;        
    }

    void AppendToTail(_In_ _xlist_node<TData>* node)
    {
        ASSERT(this->m_tail != NULL);
        ASSERT(node != NULL);
        this->m_tail->m_next = node;
        this->m_tail = node;
    }

    void PrependToHead(_In_ _xlist_node<TData>* node)
    {
        ASSERT(this->m_head != NULL);
        ASSERT(node != NULL);
        node->m_next = this->m_head;
        this->m_head = node;
    }

protected:
    _xlist_node<TData>* m_head;
    _xlist_node<TData>* m_tail;
    XUINT32 m_size;
};


