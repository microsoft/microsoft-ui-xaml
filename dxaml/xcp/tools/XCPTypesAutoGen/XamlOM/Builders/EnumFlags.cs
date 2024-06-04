// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.ComponentModel;

namespace XamlOM
{
    /// <summary>
    /// Specifies low-level flags that control the behavior of this type.
    /// </summary>
    [AttributeUsage(AttributeTargets.Enum, Inherited = false)]
    public class EnumFlagsAttribute : Attribute, NewBuilders.IEnumBuilder
    {
        /// <summary>
        /// Generates [FlagsAttribute] in JoltClasses.g.cs.
        /// </summary>
        public bool AreValuesFlags { get; set; }

        /// <summary>
        /// Causes enumeration values to be assigned numerical values on the native side, when set to True.
        /// Example: INDEX_RENDEROPTIONS_TEXTHINTINGMODE
        /// </summary>
        [DefaultValue(true)]
        public bool NativeUsesNumericValues { get; set; }

        /// <summary>
        /// Causes the enum values to be generated as hex instead of decimal
        /// Example: 0x20 vs 32
        /// </summary>
        public bool GenerateHexValues { get; set; }

        /// <summary>
        /// Generates type flag TYPE_HAS_TYPE_CONVERTER in XcpTypes.g.h when set to True.
        /// Example: INDEX_RUN
        /// </summary>
        public bool HasTypeConverter { get; set; }

        /// <summary>
        /// Prevents enumeration autogeneration in EnumDefs.g.h (UIAEnums.g.h, etc...) when set to True.
        /// Example: INDEX_FILL_RULE
        /// </summary>
        public bool IsExcludedFromNative { get; set; }

        /// <summary>
        /// Causes '?' prefix in generated 'struct KnownType.m_pwsz' name, in XcpTypes.g.h.
        /// Example: INDEX_ENUMERATED
        /// </summary>
        public bool IsTypeConverter { get; set; }

        /// <summary>
        /// Causes the enum to be a typedef on the native side when set to True.
        /// Example: INDEX_LOG_SOURCE
        /// </summary>
        public bool IsNativeTypeDef { get; set; }

        /// <summary>
        /// Causes the stable type index enum values to be added automatically when set to True.
        /// </summary>
        public bool IsStableTypeIndex { get; set; }

        /// <summary>
        /// Causes the stable property index enum values to be added automatically when set to True.
        /// </summary>
        public bool IsStablePropertyIndex { get; set; }

        /// <summary>
        /// Causes the stable event index enum values to be added automatically when set to True.
        /// </summary>
        public bool IsStableEventIndex { get; set; }

        /// <summary>
        /// If this attribute is set then in addition to generating the enum, a companion enum with the same name+"Consecutive" will be generated
        /// With all the same values except the indices will start at 0 and go up consecutively from there. A conversion function will be generated too.
        /// </summary>
        public bool GenerateConsecutiveEnum { get; set; }

        public EnumFlagsAttribute()
        {
            NativeUsesNumericValues = true;
        }

        public void BuildNewEnum(OM.EnumDefinition definition, Type source)
        {
            NewBuilders.Helper.ApplyFlagProperties(this, definition.XamlEnumFlags);
        }
    }
}
