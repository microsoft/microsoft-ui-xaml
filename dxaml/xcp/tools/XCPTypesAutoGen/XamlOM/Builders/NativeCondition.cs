// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the build condition to generate around this type or member.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class NativeConditionAttribute : Attribute
    {
        public string Condition
        {
            get;
            private set;
        }

        public NativeConditionAttribute(string condition)
        {
            Condition = condition;
        }
    }
}
