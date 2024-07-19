// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace MS.Internal
{
    internal partial class TypeProxy
    {
        private static Type[] __types = 
        {
            null, 
            //#comment  Generation tool will insert control types
            //#comment  typeof(<namespace.controlname>),
            //#insert CONTROL_TYPES
        };

        private static CreateObjectDelegate[] __create = 
        { 
            null, 
            //#comment  Generation tool will insert a creation delegate for each type
            //#comment  () => { return new <namespace.controlname>(); },
            //#insert CREATION_DELEGATES
        };

        private static UInt32[] __coreIds = 
        { 
            0, 
            //#comment  Generation tool will insert the core ID for each type
            //#comment  CoreTypes.<namespace>_<controlname>ID,
            //#insert CONTROL_IDS
        };

        private static Dictionary<Type, UInt32> _typeMap = new Dictionary<Type, UInt32>(64);

        private static void InitializeStaticData()
        {
            for (int index = 1; index < __types.Length; ++index)
            {
                _typeMap[__types[index]] = (UInt32)index;
            }
        }
    }
}
