// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace CollectionHelper
{
    template<class C, class T>
    inline bool contains(const C& c, const T& v)
    {
        return end(c) != std::find(begin(c), end(c), v);
    }

    template<class C, class T>
    inline void remove(C& c, const T& v)
    {
        c.erase(std::remove(begin(c), end(c), v), end(c));
    }

    template<class C, class T>
    inline void unique_push_back(C& c, T& v)
    {
        if (end(c) == std::find(begin(c), end(c), v))
        {
            c.push_back(v);
        }
    }
}  // namespace CollectionHelper