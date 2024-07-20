// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class StyleSetterEssence
    {
        [Flags]
        public enum StyleSetterEssenceFlags
        {
            HasThemeResourceValue = 0x1,
            HasStaticResourceValue = 1 << 1,
            HasStringValue = 1 << 2,
            HasObjectValue = 1 << 3,
            IsPropertyResolved = 1 << 4,
            HasContainerValue = 1 << 5,
            HasTokenForSelf = 1 << 6,
            IsMutable = 1 << 7,
        }

        public StyleSetterEssenceFlags Flags { get; }

        public XamlProperty Property { get; }

        public object Value { get; }

        public StreamOffsetToken Token { get; }

        public bool HasThemeResourceValue => (Flags & StyleSetterEssenceFlags.HasThemeResourceValue) ==
                                             StyleSetterEssenceFlags.HasThemeResourceValue;

        public bool HasStaticResourceValue => (Flags & StyleSetterEssenceFlags.HasStaticResourceValue) ==
                                             StyleSetterEssenceFlags.HasStaticResourceValue;

        public bool HasStringValue => (Flags & StyleSetterEssenceFlags.HasStringValue) ==
                                             StyleSetterEssenceFlags.HasStringValue;

        public bool HasObjectValue => (Flags & StyleSetterEssenceFlags.HasObjectValue) ==
                                             StyleSetterEssenceFlags.HasObjectValue;

        public bool IsPropertyResolved => (Flags & StyleSetterEssenceFlags.IsPropertyResolved) ==
                                             StyleSetterEssenceFlags.IsPropertyResolved;

        public bool HasContainerValue => (Flags & StyleSetterEssenceFlags.HasContainerValue) ==
                                             StyleSetterEssenceFlags.HasContainerValue;

        public bool HasTokenForSelf => (Flags & StyleSetterEssenceFlags.HasTokenForSelf) ==
                                             StyleSetterEssenceFlags.HasTokenForSelf;

        public bool IsMutable => (Flags & StyleSetterEssenceFlags.IsMutable) ==
                                             StyleSetterEssenceFlags.IsMutable;

        internal StyleSetterEssence(StyleSetterEssenceFlags flags, XamlProperty property, object value, StreamOffsetToken token)
        {
            Flags = flags;
            Property = property;
            Value = value;
            Token = token;
        }

        internal static StyleSetterEssence Deserialize(XbfReader reader)
        {
            XamlProperty property;
            var token = StreamOffsetToken.Default;
            object value = null;

            var flags = (StyleSetterEssenceFlags)reader.Read7BitEncodedInt();

            if ((flags & StyleSetterEssenceFlags.IsPropertyResolved) == StyleSetterEssenceFlags.IsPropertyResolved)
            {
                property = reader.ReadXamlProperty();
            }
            else
            {
                var propertyName = reader.ReadSharedString();
                var declaringType = reader.ReadXamlType();
                property = XamlPropertyRegistry.Instance.GetPropertyByName(declaringType, propertyName);
                flags |= StyleSetterEssenceFlags.IsPropertyResolved;
            }

            if ((flags & StyleSetterEssenceFlags.HasStringValue) == StyleSetterEssenceFlags.HasStringValue)
            {
                value = reader.ReadSharedString();
            }
            else if ((flags & StyleSetterEssenceFlags.HasContainerValue) == StyleSetterEssenceFlags.HasContainerValue)
            {
                value = reader.ReadConstant().Item2;
            }
            else if ((flags & StyleSetterEssenceFlags.HasStaticResourceValue) == StyleSetterEssenceFlags.HasStaticResourceValue)
            {
                token = reader.ReadStreamOffsetToken();
            }
            else if ((flags & StyleSetterEssenceFlags.HasThemeResourceValue) == StyleSetterEssenceFlags.HasThemeResourceValue)
            {
                token = reader.ReadStreamOffsetToken();
            }
            else if ((flags & StyleSetterEssenceFlags.HasObjectValue) == StyleSetterEssenceFlags.HasObjectValue)
            {
                token = reader.ReadStreamOffsetToken();
            }
            else if ((flags & StyleSetterEssenceFlags.HasTokenForSelf) == StyleSetterEssenceFlags.HasTokenForSelf)
            {
                token = reader.ReadStreamOffsetToken();
            }

            return new StyleSetterEssence(flags, property, value, token);
        }
    }

    public class StyleCustomRuntimeData : CustomRuntimeData
    {
        public List<StyleSetterEssence> Setters { get; }

        public StyleCustomRuntimeData(CustomWriterRuntimeDataTypeIndex version, List<StyleSetterEssence> setters)
            : base(version)
        {
            Setters = setters;
        }

        internal static StyleCustomRuntimeData CreateAndDeserializeRuntimeData(XbfReader reader, CustomWriterRuntimeDataTypeIndex typeIndex)
        {
            var setters = reader.ReadVector(StyleSetterEssence.Deserialize, true);

            return new StyleCustomRuntimeData(typeIndex, setters);
        }
    }
}