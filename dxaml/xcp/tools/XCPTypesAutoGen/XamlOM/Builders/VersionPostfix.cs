// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace XamlOM
{
    /// <summary>
    /// Specifies the postfix for a version. By default, versioned interfaces are named I[TypeName][VersionNumber].
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Delegate | AttributeTargets.Enum, Inherited = false, AllowMultiple = true)]
    public class VersionPostfixAttribute : Attribute
    {
        public string Postfix
        {
            get;
            private set;
        }

        public int Version
        {
            get;
            private set;
        }

        public VersionPostfixAttribute(string postfix)
        {
            Version = 1;
            Postfix = postfix;
        }

        public VersionPostfixAttribute(int version, string postfix)
        {
            Version = version;
            Postfix = postfix;
        }
    }
}
