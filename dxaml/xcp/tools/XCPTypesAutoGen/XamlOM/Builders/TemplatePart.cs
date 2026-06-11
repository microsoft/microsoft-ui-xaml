// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;

namespace Microsoft.UI.Xaml
{
    /// <summary>
    /// Represents an attribute that is applied to the class definition to identify the types of the named parts that are used for templating.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = true, Inherited = false)]
    public class TemplatePartAttribute : Attribute, XamlOM.NewBuilders.IClassBuilder
    {
        /// <summary>
        /// Gets or sets the pre-defined name of the part.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        /// <summary>
        /// Gets or sets the type of the named part this attribute is identifying. 
        /// </summary>
        public Type Type
        {
            get;
            set;
        }

        public TemplatePartAttribute(string name, Type type)
        {
            Name = name;
            Type = type;
        }

        public void BuildNewClass(OM.ClassDefinition definition, Type source)
        {
            OM.TypeReference templatePartTypeRef = XamlOM.NewBuilders.Helper.GetTypeRef(Type);

            // Generally describe the attribute.
            OM.AttributeInstantiation attribute = new OM.AttributeInstantiation()
            {
                ArgumentType = XamlOM.NewBuilders.ModelFactory.GetOrCreateAttribute(this.GetType())
            };
            attribute.Arguments.Add(Name);
            attribute.Arguments.Add(templatePartTypeRef);
            definition.Attributes.Add(attribute);

            // Add first-class description for a template part for easier processing during code-gen.
            definition.TemplateParts.Add(new OM.TemplatePartDefinition()
            {
                Name = Name,
                Type = templatePartTypeRef
            });
        }
    }
}
