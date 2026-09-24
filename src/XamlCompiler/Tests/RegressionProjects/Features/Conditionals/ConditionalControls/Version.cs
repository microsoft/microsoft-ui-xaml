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

    public static class Test
    {
        public static void EnsureVersion(Type type)
        {
            if (!Windows.Foundation.Metadata.ApiInformation.IsTypePresent(type.FullName))
            {
                throw new System.InvalidOperationException("API not supposed to run in this version");
            }
        }
    }        
}
