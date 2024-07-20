// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// This attribute is intended for ITextRangeProvider.GetBoundingRectangles only. The return type was supposed to be 
    /// a real return type, but originally in the SLOM the return type was marked as "out", causing the IDL to represent it 
    /// as an out parameter rather than a return type. This attribute was invented to preserve that signature.
    /// </summary>
    [AttributeUsage(AttributeTargets.Method, Inherited = false)]
    public class ReturnTypeIsOutAttribute : Attribute, NewBuilders.IMethodBuilder
    {
        public ReturnTypeIsOutAttribute()
        {
        }

        public void BuildNewMethod(OM.MethodDefinition definition, MethodInfo source)
        {
            definition.ReturnType.IsOut = true;
        }
    }
}
