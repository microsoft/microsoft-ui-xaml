// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Navigation magic numbers.

#pragma once

// Work around disruptive max/min macros
#undef max
#undef min

namespace DirectUI { namespace Components { namespace Moco {

    const int s_cNavigationIndexFirst = 0;
    const int s_cNavigationIndexLast = std::numeric_limits<int>::max();
    const int s_cNavigationIndexNone = -1;
    
} } }