// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

/***************************************************************************\
*
*
*  Class for Xaml markup extension for a Type reference
*
*
\***************************************************************************/
using System;
using System.Windows;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Xaml;

namespace System.Windows.Markup
{
    /// <summary>
    ///  Class for Xaml markup extension for a Type reference.  
    /// </summary>
    [TypeForwardedFrom("PresentationFramework, Version=3.5.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35")]
    [TypeConverter(typeof(TypeExtensionConverter))]
    [MarkupExtensionReturnType(typeof(Type))]
    public class TypeExtension : MarkupExtension 
    {
        /// <summary>
        ///  Default Constructor 
        /// </summary>
        public TypeExtension()
        {
        }
        
        /// <summary>
        ///  Constructor that takes a type name
        /// </summary>
        public TypeExtension(string typeName)
        {
            if (typeName == null)
            {
                throw new ArgumentNullException("typeName");
            }
            
            _typeName = typeName;
        }

        /// <summary>
        /// Constructor that takes a type
        /// </summary>
        public TypeExtension(Type type)
        {
            if (type == null)
            {
                throw new ArgumentNullException("type");
            }

            _type = type;
            // we would like to set TypeName here, but we can't because we can't resolve its namespace
        }

        /// <summary>
        ///  The typename represented by this markup extension.  The name has the format
        ///  Prefix:Typename, where Prefix is the xml namespace prefix for this type.
        /// </summary>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public string TypeName 
        {
            get { return _typeName; }
            set 
            {
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                _typeName = value;
                _type = null; // so that ProvideValue does not use the existing type
            }
        }

        /// <summary>
        ///  The type represented by this markup extension.  
        /// </summary>
        [DefaultValue(null)]
        [ConstructorArgument("type")]
        public Type Type
        {
            get { return _type; }
            set 
            { 
                if (value == null)
                {
                    throw new ArgumentNullException("value");
                }

                _type = value; 
                _typeName = null; // so that ProvideValue does not use the existing typeName
            }
        }

        private string _typeName;
        private Type _type;
    }
}

