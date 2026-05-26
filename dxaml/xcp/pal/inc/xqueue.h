// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef _XQUEUE_H_
#define _XQUEUE_H_

template <typename TData>
class xqueue 
    : private xdevector<TData>
{
private:
    // Poison constructors
    xqueue& operator=(const xqueue& other);
    xqueue(const xqueue& other);

public: 
    xqueue(void) {}
    ~xqueue(void) {}

    typedef _xvector_iterator<TData> iterator;
    typedef _xvector_const_iterator<TData> const_iterator;
    typedef _xvector_reverse_iterator<TData> reverse_iterator;
    typedef _xvector_const_reverse_iterator<TData> const_reverse_iterator;

    HRESULT front(TData& val)       { return xdevector<TData>::front(val);          }
    HRESULT back(TData& val)        { return xdevector<TData>::back(val);           }
    HRESULT push(const TData& val)  { return xdevector<TData>::push_back(val);      }
    HRESULT pop()                   { return xdevector<TData>::pop_front();         }
    void clear()                    { xdevector<TData>::clear();                    }
    XUINT32 size()                  { return xdevector<TData>::size();              }
    bool empty()                   { return xdevector<TData>::empty();             }

    _xvector_reverse_iterator<TData> rbegin()               { return xdevector<TData>::rbegin();         }
    _xvector_reverse_iterator<TData> rend()                 { return xdevector<TData>::rend();           }
    _xvector_const_reverse_iterator<TData> rbegin() const   { return xdevector<TData>::rbegin();         }
    _xvector_const_reverse_iterator<TData> rend() const     { return xdevector<TData>::rend();           }

    _xvector_iterator<TData> begin()               { return xdevector<TData>::begin();         }
    _xvector_iterator<TData> end()                 { return xdevector<TData>::end();           }
    _xvector_const_iterator<TData> begin() const   { return xdevector<TData>::begin();         }
    _xvector_const_iterator<TData> end() const     { return xdevector<TData>::end();           }
};

#endif
