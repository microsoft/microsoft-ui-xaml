// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;

namespace ConditionalControls
{
    public sealed class V1Type
    {
    }

    public sealed class V2Type
    {
    }

    internal sealed class V3Type
    {
    }

    internal static class Test
    {
        public static void EnsureVersion<T>()
        {
            if (!Windows.Foundation.Metadata.ApiInformation.IsTypePresent(typeof(T).FullName))
            {
                throw new System.InvalidOperationException("API not supposed to run in this version");
            }
        }
    }        
}
