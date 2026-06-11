// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.ComponentModel;

namespace XamlOM
{
    /// <summary>
    /// Specifies low-level flags that control the behavior of this type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Delegate | AttributeTargets.Struct | AttributeTargets.Interface | AttributeTargets.Enum, Inherited = false)]
    public class TypeFlagsAttribute : Attribute, NewBuilders.ITypeBuilder
    {
        /// <summary>
        /// Gets or sets a value indicating whether to generate the [muse] attribute in IDL.
        /// </summary>
        public bool GenerateMuseAttribute { get; set; }

        /// <summary>
        /// DateTime, Point and TimeSpan, despite being structs, have special provisions in the WinRT type system
        /// to be boxed as value types instead of references.
        /// </summary>
        public bool HasSpecialBoxer { get; set; }

        /// <summary>
        /// Excludes type from the DXaml IDL.
        /// </summary>
        public bool IsExcludedFromDXamlInterface { get; set; }

        /// <summary>
        /// Excludes type in DirectUI type table.
        /// </summary>
        [DefaultValue(true)]
        public bool IsCreateableFromDXaml { get; set; }

        /// <summary>
        /// Generates type flag TYPE_NOT_CREATEABLE_FROM_XAML in XcpTypes.g.h when set to False.
        /// Example: INDEX_NOTIFY_EVENT_ARGS
        /// </summary>
        [DefaultValue(true)]
        public bool IsCreateableFromXAML { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this type is a system type from DXaml's perspective.
        /// Example: INDEX_RECT
        /// </summary>
        public bool IsDXamlSystemType { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this type should be webhosthidden
        /// </summary>
        [DefaultValue(true)]
        public bool IsWebHostHidden { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this type is re-mapping a core type.
        /// Example: FontWeight.
        /// </summary>
        public bool IsRemappingOfCoreType { get; set; }

        public TypeFlagsAttribute()
        {
            IsCreateableFromDXaml = true;
            IsCreateableFromXAML = true;
            IsWebHostHidden = true;
        }

        public void BuildNewType(OM.TypeDefinition definition, Type source)
        {
            definition.IsWebHostHidden = IsWebHostHidden;
            NewBuilders.Helper.ApplyFlagProperties(this, definition.XamlTypeFlags);
        }
    }
}
