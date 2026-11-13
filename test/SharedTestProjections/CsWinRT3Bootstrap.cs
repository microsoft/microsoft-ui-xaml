// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#if CSWINRT3_0
using System;
using System.IO;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.Loader;

internal static class CsWinRT3Bootstrap
{
    [ModuleInitializer]
    internal static void Initialize()
    {
        // Native activation must share TAEF's runtime and type-map assembly identities.
        AppContext.SetSwitch("CSWINRT_LOAD_COMPONENTS_IN_DEFAULT_ALC", true);
        if (Environment.Version.Major >= 11)
        {
            AppContext.SetData("System.Runtime.InteropServices.TypeMappingEntryAssembly", "SharedTestProjections");
        }
        else
        {
            Assembly module = typeof(CsWinRT3Bootstrap).Assembly;
            string path = Path.Combine(Path.GetDirectoryName(module.Location), "SharedTestProjections.dll");
            Assembly root = AssemblyLoadContext.GetLoadContext(module).LoadFromAssemblyPath(path);
            Assembly.SetEntryAssembly(root);
        }
    }
}
#endif
