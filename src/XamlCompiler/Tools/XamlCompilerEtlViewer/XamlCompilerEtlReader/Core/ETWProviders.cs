// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XamlCompilerEtlReader.Core
{
    /// <summary>
    /// ETWProviders
    /// </summary>
    public static class ETWProviders
    {
        public static Guid ClrRundownProviderGuid = new Guid("A669021C-C450-4609-A035-5AF59AF4DF18");

        public static Guid ClrRuntimeProviderGuid = new Guid("e13c0d23-ccbc-4e12-931b-d9cc2eee27e4");

        public static Guid CodeMarkersProviderGuid = new Guid("641d7f6c-481c-42e8-ab7e-d18dc5e5cb9e");

        public static Guid XamlCompilerGuid = new Guid("7002328e-3a2c-50b9-f436-c98c8017683e");
    }
}
