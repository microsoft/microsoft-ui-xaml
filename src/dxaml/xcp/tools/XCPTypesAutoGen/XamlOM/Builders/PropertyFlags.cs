// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.ComponentModel;
using System.Reflection;

namespace XamlOM
{
    /// <summary>
    /// Specifies low-level flags that control the behavior of this property.
    /// </summary>
    [AttributeUsage(AttributeTargets.Property, Inherited = false)]
    public class PropertyFlagsAttribute : Attribute, NewBuilders.IPropertyBuilder
    {
        /// <summary>
        /// Generates property flag PROP_AFFECT_ARRANGE in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_BITMAPCACHE_RENDER_AT_SCALE
        /// </summary>
        public bool AffectsArrange { get; set; }

        /// <summary>
        /// Generates property flag PROP_AFFECT_MEASURE in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_UIELEMENT_VISIBILITY
        /// </summary>
        public bool AffectsMeasure { get; set; }

        /// <summary>
        /// Generates property flag  PROP_DO_NOT_ENTER_LEAVE_VALUE in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_ITEMS_CONTROL_ITEMS_HOST, PROPERTYINDEX_RICH_TEXT_BOX_OVERFLOW_CONTENT_TARGET.
        /// </summary>
        public bool DoNotEnterOrLeaveValue { get; set; }

        /// <summary>
        /// Generates property flag  PROP_NEEDS_INVOKE in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_GLYPHS_FONTURI, PROPERTYINDEX_MEDIA_ELEMENT_SOURCE.
        /// </summary>
        public bool NeedsInvoke { get; set; }

        /// <summary>
        /// In Windows 8.1 (Blue), this property was stored in a field, but it's now stored in sparse storage,
        /// which we need to know to determine when to un-associate multi-associate objects compatibly.
        /// </summary>
        public bool HadFieldInBlue { get; set; }

        /// <summary>
        /// Gets or sets whether the property is conditionally independently animatable.
        /// </summary>
        public bool IsConditionallyIndependentlyAnimatable { get; set; }

        /// <summary>
        /// Gets or sets whether this property is NOT a visual tree property.
        /// Example: Page.Frame.
        /// </summary>
        public bool IsExcludedFromVisualTree { get; set; }

        /// <summary>
        /// Forces a property to be part of the visual tree
        /// </summary>
        public bool IsForcedIntoVisualTree { get; set; }

        /// <summary>
        /// Generates property flag PROP_STORAGE_GROUP in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_DROP_SHADOW_EFFECT_COLOR
        /// </summary>
        public bool IsInStorageGroup { get; set; }

        /// <summary>
        /// Gets or sets whether the property value is required. Default is False.
        /// </summary>
        public bool IsRequired { get; set; }

        /// <summary>
        /// Generates property flag PROP_INHERITED in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_TEXTELEMENT_FONTSIZE
        /// </summary>
        public bool IsValueInherited { get; set; }

        /// <summary>
        /// Generates property flag PROP_ON_DEMAND in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_RESOURCE_DICTIONARY_MERGEDDICTIONARIES
        /// </summary>
        public bool IsValueCreatedOnDemand { get; set; }

        /// <summary>
        /// Generates property flag PROP_PARSERONLY_WRITE in XcpTypes.g.h when set to True.
        /// Example: PROPERTYINDEX_COLOR_ANIMATION_USING_KEYFRAMES_KEYFRAMES
        /// </summary>
        public bool IsReadOnlyExceptForParser { get; set; }

        /// <summary>
        /// Gets or sets whether the property is independently animatable.
        /// </summary>
        public bool IsIndependentlyAnimatable { get; set; }

        /// <summary>
        /// Gets or sets whether this property is a helper property on a struct.
        /// </summary>
        public bool IsHelper { get; set; }

        /// <summary>
        /// Gets or sets whether this property's field should be a ComPtr instead of a TrackerPtr.
        /// </summary>
        public bool UseComPtr { get; set; }

        /// <summary>
        /// Gets or sets whether this property's getter should have a virtual implementation
        /// </summary>
        public bool IsGetterImplVirtual { get; set; }

        /// <summary>
        /// Gets or sets whether this property's setter should have a virtual implementation
        /// </summary>
        public bool IsSetterImplVirtual { get; set; }

        public PropertyFlagsAttribute()
        {
        }

        public void BuildNewProperty(OM.PropertyDefinition definition, PropertyInfo source)
        {
            definition.IsStructHelper = IsHelper;
            definition.PropertyType.IsOptional = !IsRequired;
            if (IsReadOnlyExceptForParser)
            {
                definition.IdlPropertyInfo.IsReadOnly = true;
                definition.IsReadOnly = true;
            }
            NewBuilders.Helper.ApplyFlagProperties(this, definition.XamlPropertyFlags);
        }
    }
}
