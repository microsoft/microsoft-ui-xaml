// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.ComponentModel;
using System.ComponentModel.Design.Serialization;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Xaml;

namespace System.Windows.Markup
{
    /// <summary>
    ///  Class for Xaml markup extension for static field and property references.
    /// </summary>
    [TypeForwardedFrom("PresentationFramework, Version=3.5.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35")]
    [TypeConverter(typeof(StaticExtensionConverter))]
    [MarkupExtensionReturnType(typeof(object))]
    public class StaticExtension : MarkupExtension 
    {
        /// <summary>
        ///  Constructor that takes no parameters
        /// </summary>
        public StaticExtension()
        {
        }
        
        /// <summary>
        ///  Constructor that takes the member that this is a static reference to.  
        ///  This string is of the format 
        ///     Prefix:ClassName.FieldOrPropertyName.  The Prefix is 
        ///  optional, and refers to the XML prefix in a Xaml file.
        /// </summary>
        public StaticExtension(
            string   member)
        {
            if (member == null)
            {
                throw new ArgumentNullException("member");
            }
            _member = member;
        }

        /// <summary>
        ///  The static field or property represented by a string.  This string is
        ///  of the format Prefix:ClassName.FieldOrPropertyName.  The Prefix is 
        ///  optional, and refers to the XML prefix in a Xaml file.
        /// </summary>
        [ConstructorArgument("member")]
        public string Member
        {
            get { return _member; }
            set 
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                _member = value;
            }
        }

        [DefaultValue(null)]
        public Type MemberType
        {
            get { return _memberType; }
            set
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }
                _memberType = value;
            }
        }

        private string _member;
        private Type _memberType;
    }
}

