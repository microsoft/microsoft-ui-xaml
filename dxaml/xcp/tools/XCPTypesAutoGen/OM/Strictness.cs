// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    // The Strictness enum defines how an API will be enforced against strict mode requirements.
    // The enforcement of strict mode is a function of StrictType (type strictness), DO.UseStrict (per-object strictness)
    // and Strictness (API strictness), as follows:
    // 
    // 1) If the API has no Strictness attribute set (default):
    // if (StrictType) : banned
    // else allowed
    //
    // 2) If the API has Strictness = StrictOnly:
    // if (StrictType) : allowed
    // else if (DO.UseStrict) : allowed
    // else banned
    //
    // 3) If the API has Strictness = NonStrictOnly:
    // if (StrictType) : banned
    // else if (DO.UseStrict) : banned
    // else allowed
    //
    // 4) If the API has Strictness = Agnostic:
    // always allowed
    public enum Strictness
    {
        Agnostic,       // API can be accessed in strict or non strict mode.
        StrictOnly,     // API can only be accessed in strict mode.
        NonStrictOnly,  // API can only be accessed in non-strict mode
    }
}
