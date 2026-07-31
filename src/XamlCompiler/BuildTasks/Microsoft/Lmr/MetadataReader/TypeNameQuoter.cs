// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Text;
using Debug=Microsoft.UI.Xaml.Markup.Compiler.Lmr.Internal.Debug;

namespace Microsoft.UI.Xaml.Markup.Compiler.Lmr
{
    /// <summary>
    /// The class is used to quote type names (including the namespace name).
    /// The type names in the metadata are all unquoted but reflection returns the quoted name. 
    /// For example, if a type name is "[Foo]", reflection returns "\[Foo\]" for the Name property of the type.
    /// </summary>
    static internal class TypeNameQuoter
    {
        //The spec of the special characters can be found at http://msdn.microsoft.com/en-us/library/w3f99sx1.aspx
        //In these special characters, '.' cannot be used as part of the type name, and reflection doesn't escape '`'.
        //More tests show that reflection also escapes '&' an '*'

        static private char[] specialCharacters = { '\\', '[', ']', ',', '+', '&', '*' };

        static internal string GetQuotedTypeName(string name)
        {
            if (name.IndexOfAny(specialCharacters) == -1)
            {
                //No special character in the name
                return name;
            }

            //escape the special characters in name
            StringBuilder sb = StringBuilderPool.Get();
            for (int i = 0; i < name.Length; i++)
            {
                if (Contains(specialCharacters, name[i]))
                {
                    //escape the special character by inserting a \
                    sb.Append('\\');
                }
                sb.Append(name[i]);
            }

            string result = sb.ToString();
            StringBuilderPool.Release(ref sb);
            return result;
        }


        // The Array.Contains extension method doesn't exist in until .NET 3.5, so 
        // we define it here.  
        // Can't define an extension method without ref to System.Core.dll.
        static bool Contains(char[] This, char ch)
        {
            foreach (char c in This)
            {
                if (c == ch)
                {
                    return true;
                }
            }
            return false;
        }
    }
}
