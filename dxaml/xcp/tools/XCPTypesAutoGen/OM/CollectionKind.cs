// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public enum CollectionKind
    {
        Concrete = 0, // Reference the class by its name
        Enumerable = 1,
        Indexable = 2,
        Observable = 3,
        Vector = 4
    }
}
