// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class ApiInformationMethod
    {

        public bool Condition { get; }
        public string MethodName { get; }

        public ApiInformationMethod(string methodName, bool condition)
        {
            Condition = condition;
            MethodName = methodName;
        }

        public string UniqueName => string.Format("{0}{1}", MethodName, Condition ? "" : "Not");
    }
}
