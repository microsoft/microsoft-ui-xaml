// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if CSWINRT3_0
namespace MUXControls.Test
{
    internal static class CsWinRT3ModuleInitializer
    {
        public static void Main()
        {
        }

        // Set the TypeMap entry assembly when TAEF loads this DLL.
        [System.Runtime.CompilerServices.ModuleInitializer]
        public static void SetEntryAssemblyForCsWinRT()
        {
            global::System.Reflection.Assembly.SetEntryAssembly(typeof(CsWinRT3ModuleInitializer).Assembly);
        }
    }
}
#endif
