// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    /// <summary>
    /// String hash used to build identifiers in generated code. It must be deterministic across
    /// processes and identical regardless of the runtime the compiler runs on.
    /// .NET randomizes string.GetHashCode() per process, so this is the string.GetHashCode() algorithm of the
    /// 64-bit .NET Framework, which the compiler used to run on: the generated output stays unchanged.
    /// </summary>
    internal static class StableStringHash
    {
        public static int Compute(string s)
        {
            unchecked
            {
                int hash1 = 5381;
                int hash2 = hash1;
                int length = s.Length;
                for (int i = 0; i < length; i += 2)
                {
                    int c = s[i];
                    if (c == 0)
                    {
                        break;
                    }
                    hash1 = ((hash1 << 5) + hash1) ^ c;

                    if (i + 1 >= length)
                    {
                        break;
                    }
                    c = s[i + 1];
                    if (c == 0)
                    {
                        break;
                    }
                    hash2 = ((hash2 << 5) + hash2) ^ c;
                }
                return hash1 + (hash2 * 1566083941);
            }
        }
    }
}
