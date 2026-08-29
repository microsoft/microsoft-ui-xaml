// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xvector.h"

template <typename TData>
class xstack 
    : private xvector<TData>
{
private:
    // Poison constructors
    xstack& operator=(_In_ const xstack& other);
    xstack(_In_ const xstack& other);
public: 
    typedef _xvector_reverse_iterator<TData> iterator;
    typedef _xvector_const_reverse_iterator<TData> const_iterator;
    typedef _xvector_iterator<TData> reverse_iterator;
    typedef _xvector_const_iterator<TData> const_reverse_iterator;
    xstack() {}
    virtual ~xstack() {}

    HRESULT front(TData& val)       { return xvector<TData>::back(val);      }
    HRESULT back(TData& val)        { return xvector<TData>::front(val);       }
    HRESULT top(TData& val)         { return xvector<TData>::back(val);      }
    HRESULT bottom(TData& val)      { return xvector<TData>::front(val);       }
    HRESULT reserve(_In_ XUINT32 requestedSize) { return xvector<TData>::reserve(requestedSize); }
    HRESULT push(_In_ const TData& val)  { return xvector<TData>::push_back(val); }
    HRESULT pop()                   { return xvector<TData>::pop_back();     }
    void clear()                    { xvector<TData>::clear();                }
    XUINT32 size() const            { return xvector<TData>::size();          }
    bool empty() const             { return xvector<TData>::empty();         }

    // NOTE: The mac requires us to expand these typedefs.
    // Even though this is a reverse-iterator on the underlying vector, it's
    // still a forward iterator. In calling code you can use the type defs above.
    _xvector_reverse_iterator<TData> begin() { return xvector<TData>::rbegin();         }
    _xvector_reverse_iterator<TData> end()   { return xvector<TData>::rend();           }
    _xvector_const_reverse_iterator<TData> begin() const    { return xvector<TData>::rbegin();         }
    _xvector_const_reverse_iterator<TData> end() const      { return xvector<TData>::rend();           }

    _xvector_iterator<TData> rbegin() { return xvector<TData>::begin();         }
    _xvector_iterator<TData> rend()   { return xvector<TData>::end();           }
    _xvector_const_iterator<TData> rbegin() const    { return xvector<TData>::begin();         }
    _xvector_const_iterator<TData> rend() const      { return xvector<TData>::end();           }
};

