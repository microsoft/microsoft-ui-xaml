// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    internal class ClassName
    {
        public String Namespace { get; }
        public String ShortName { get; }

        public ClassName(string fullName)
        {
            if (!string.IsNullOrEmpty(fullName))
            {
                int lastDotIndex = fullName.LastIndexOf('.');
                if (lastDotIndex == -1)
                {
                    throw new ArgumentException($"Class full name is invalid: {fullName}");
                }
                Namespace = fullName.Substring(0, lastDotIndex);
                ShortName = fullName.Substring(lastDotIndex + 1);
            }
        }

        public string FullName
        {
            get
            {
                return string.IsNullOrEmpty(ShortName) ?
                    string.Empty :
                    string.Format("{0}.{1}", Namespace, ShortName);
            }
        }
    }
}
