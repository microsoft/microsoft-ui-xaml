// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Xaml;

namespace System.Windows.Markup
{
    [ContentProperty("Name")]
    public class Reference : MarkupExtension
    {
        public Reference()
        {
        }

        public Reference(string name)
        {
            Name = name;
        }

        [ConstructorArgument("name")]
        public string Name { get; set; }

    }
}
