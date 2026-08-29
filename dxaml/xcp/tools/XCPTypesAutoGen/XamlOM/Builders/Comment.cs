// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies the comment to generate.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Interface | AttributeTargets.Enum | AttributeTargets.Property | AttributeTargets.Method, Inherited = false)]
    public class CommentAttribute : 
        Attribute,
        NewBuilders.ITypeBuilder, NewBuilders.IMemberBuilder
    {
        public string Comment
        {
            get;
            private set;
        }

        public CommentAttribute(string comment)
        {
            Comment = comment;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.Comment = Comment;
        }

        public void BuildNewMember(OM.MemberDefinition definition, MemberInfo source)
        {
            definition.Comment = Comment;
        }
    }
}
