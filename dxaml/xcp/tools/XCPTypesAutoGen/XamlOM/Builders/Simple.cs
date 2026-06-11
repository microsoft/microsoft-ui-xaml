// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Reflection;
using System.Text.RegularExpressions;

namespace XamlOM
{
    /// <summary>
    /// Declares event is associated with simple property change notification.
    /// </summary>
    [AttributeUsage(AttributeTargets.Event, Inherited = false)]
    public class SimplePropertyEventAttribute : Attribute, NewBuilders.IEventBuilder
    {
        /// <summary>
        /// Name of simple property triggering event.  Name is resolved within the scope of the declaring type.
        /// </summary>
        public string SourceMemberName
        {
            get;
            private set;
        }

        public SimplePropertyEventAttribute(string sourceMemberName)
        {
            SourceMemberName = sourceMemberName;
        }

        public void BuildNewEvent(OM.EventDefinition definition, EventInfo source)
        {
            definition.SimplePropertyEventSourceMemberName = SourceMemberName;
        }
    }

    /// <summary>
    /// Declares type can be used for simple properties.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Enum | AttributeTargets.Struct, Inherited = false)]
    public class SimpleTypeAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        /// <summary>
        /// How will this type appear in the interface.  See SimpleProperty.h for more info.
        /// </summary>
        public SimpleTypeKind Kind
        {
            get;
            private set;
        }

        /// <summary>
        /// Value passed for initialization of type's default value.
        /// </summary>
        public string DefaultValue
        {
            get;
            set;
        }

        /// <summary>
        /// A hint for using constexpr initialization for type's default.  To use constexpr initialization,
        /// type needs to have constexpr constructor (or be trivially constructible) and have no user-defined destructor.
        /// </summary>
        public bool IsConstexprConstructible
        {
            get;
            set;
        } = true;

        public SimpleTypeAttribute(SimpleTypeKind kind)
        {
            Kind = kind;
        }

        public void BuildNewType(TypeDefinition definition, Type source)
        {
            definition.SimpleTypeKind = Kind;

            if (!string.IsNullOrEmpty(DefaultValue))
            {
                definition.SimpleDefaultValue = DefaultValue;
            }

            definition.IsConstexprConstructible = IsConstexprConstructible;
        }
    }

    /// <summary>
    /// Declares simple property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class SimplePropertyAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        /// <summary>
        /// Declares how the property will be stored (e.g. sparse, field, group sparse).  See SimpleProperty.h for more info.
        /// </summary>
        public SimplePropertyStorage Storage
        {
            get;
            private set;
        }

        /// <summary>
        /// Value passed for initialization of property default value.  It overrides per type default.
        /// </summary>
        public string DefaultValue
        {
            get;
            set;
        }

        /// <summary>
        /// Name of type used for storing grouped properties when Storage is GroupSparse.
        /// </summary>
        public string GroupStorageClass
        {
            get;
            set;
        }

        public SimplePropertyAttribute(SimplePropertyStorage storage)
        {
            Storage = storage;
        }

        public void BuildNewProperty(PropertyDefinition definition, PropertyInfo source)
        {
            DependencyPropertyDefinition asDpd = definition as DependencyPropertyDefinition;

            if (asDpd != null)
            {
                asDpd.SimpleStorage = Storage;

                if (!string.IsNullOrEmpty(DefaultValue))
                {
                    if (Helper.ShouldPrefixWithABI())
                    {
                        // (?<!::) -- non-capturing, negative look-behind assertion for names already qualified to root namespace or by other namespace (e.g. ABI), \b - word boundary
                        // ABI::Windows::, ::Windows::, MyWindows:: - will not replace
                        // ::Windows:: - will replace
                        asDpd.SimpleDefaultValue = Regex.Replace(DefaultValue, @"(?<!::)\bWindows::", @"ABI::Windows::");
                    }
                    else
                    {
                        asDpd.SimpleDefaultValue = DefaultValue;
                    }

                    asDpd.SimpleDefaultValue = asDpd.SimpleDefaultValue;
                }

                asDpd.SimpleGroupStorageClass = GroupStorageClass;
            }
            else
            {
                throw new Exception("SimpleProperty can only attribute internal DP definitions.");
            }
        }
    }
}
