// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

// Description:
//   This attribute is placed on a class to identify the property that will
//   function as an Name for the given class

using System;
using System.Globalization;
using SRCS = System.Runtime.CompilerServices;

namespace System.Windows.Markup
{
    /// <summary>
    /// This attribute is placed on a class to identify the property that will
    /// function as an Name for the given class
    /// </summary>
    [AttributeUsage(AttributeTargets.Class)]
    [SRCS.TypeForwardedFrom("WindowsBase, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35")]
    public sealed class RuntimeNamePropertyAttribute: Attribute
    {
        /// <summary/>
        public RuntimeNamePropertyAttribute(string name)
        {
            _name = name;
        }

        /// <summary>
        /// The Name of the property in the class that will contain the ID of
        /// the class, this property needs to be of type string and have
        /// both get and set access
        /// </summary>
        public string Name
        {
            get
            {
                return _name;
            }
        }

        private string _name = null;
    }

}
