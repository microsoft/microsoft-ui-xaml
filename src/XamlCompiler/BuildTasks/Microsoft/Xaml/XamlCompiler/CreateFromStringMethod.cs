// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    using CodeGen;

    internal class CreateFromStringMethod
    {
        private string fullName;
        private string localName;
        private XamlType declaringType;

        public MethodInfo MethodInfo { get; private set; }
        public XamlType DeclaringType { get { return this.declaringType; } }
        public bool Resolved { get; private set; } = false;

        public CreateFromStringMethod()
        {
            // There's no CreateFromString method on container.
            // This object will be "empty"
            this.fullName = string.Empty;
            this.localName = string.Empty;
        }

        public CreateFromStringMethod(string fullName)
        {
            this.fullName = fullName;
            this.localName = string.Empty;
        }

        public CreateFromStringMethod(XamlType declaringType, string localName)
        {
            this.fullName = string.Empty;
            this.localName = localName;
            this.declaringType = declaringType;
        }

        public bool Exists
        {
            get
            {
                if (string.IsNullOrEmpty(this.fullName) && string.IsNullOrEmpty(this.localName))
                {
                    // Both are null or empty, we don't have a CreateFromString method
                    return false;
                }
                else
                {
                    // At least one of them has something.
                    return true;
                }
            }
        }

        public string UnresolvedName
        {
            get
            {
                if (!string.IsNullOrEmpty(fullName))
                {
                    return fullName;
                }

                if (!string.IsNullOrEmpty(localName))
                {
                    var parts = localName.Split('.');
                    if (parts.Length == 2)
                    {
                        // Local item is a method on a nested type. Parts must have exactly one dot.
                        var nestedTypeName = parts[0];
                        var methodName = parts[1];
                        return $"{declaringType.UnderlyingType.FullName}+{nestedTypeName}.{methodName}";
                    }
                    else
                    {
                        return $"{declaringType.UnderlyingType.FullName}.{localName}";
                    }
                }

                return string.Empty;
            }
        }

        public void SetResolved(XamlType declaringType, string methodName, MethodInfo methodInfo)
        {
            this.declaringType = declaringType;
            this.MethodInfo = methodInfo;
        }

        public LanguageSpecificString ResolvedName
        {
            get {
                return new LanguageSpecificString(
                    () => $"{this.DeclaringType.CppCXName(false)}::{MethodInfo.Name}",
                    () => $"{this.DeclaringType.CppWinRTName()}::{MethodInfo.Name}",
                    () => $"{this.DeclaringType.CSharpName()}.{MethodInfo.Name}",
                    () => $"{this.DeclaringType.VBName()}.{MethodInfo.Name}");
            }
        }
    }
}
