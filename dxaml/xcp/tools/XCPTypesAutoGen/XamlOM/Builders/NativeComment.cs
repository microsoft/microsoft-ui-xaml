// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Comment is generated in EnumDefs.g.h, UIAEnums.g.h, etc…
    /// Example: Block all hosting page interaction with the application
    /// </summary>
    [AttributeUsage(AttributeTargets.Enum | AttributeTargets.Field, Inherited = false)]
    public class NativeCommentAttribute : Attribute, NewBuilders.IEnumBuilder
    {
        public string Comment
        {
            get;
            private set;
        }

        public NativeCommentAttribute(string comment)
        {
            Comment = comment;
        }

        public void BuildNewEnum(OM.EnumDefinition definition, Type source)
        {
            definition.UIAComment = Comment;
        }
    }
}
