// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace Microsoft.UI.Xaml.Markup
{
    /// <summary>
    /// Specifies the content property of a type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct, Inherited = false)]
    public class ContentPropertyAttribute : Attribute, XamlOM.NewBuilders.IClassBuilder
    {
        // Forces inclusion of the [ContentProperty] attribute in IDL. This only exists right now 
        // for compat reasons, because we shipped Windows Blue with a [ContentProperty("Template")] attribute 
        // on FrameworkTemplate, while Template is an internal property.
        private bool _forceIncludeInIdl
        {
            get;
            set;
        }

        public string Name
        {
            get;
            set;
        }

        public ContentPropertyAttribute(string name)
        {
            Name = name;
        }

        public ContentPropertyAttribute(string name, bool forceIncludeInIdl)
        {
            Name = name;
            _forceIncludeInIdl = forceIncludeInIdl;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            OM.AttributeInstantiation attribute = new OM.AttributeInstantiation()
            {
                ArgumentType = XamlOM.NewBuilders.ModelFactory.GetOrCreateAttribute(this.GetType())
            };
            attribute.Arguments.Add(Name);
            definition.Attributes.Add(attribute);

            definition.IdlClassInfo.ForceIncludeContentProperty = _forceIncludeInIdl;
        }
    }
}
