// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class DeferredElementCustomRuntimeData : CustomRuntimeData
    {
        public string Name { get; private set; }
        public List<Tuple<XamlProperty, object>> NonDeferredProperties { get; private set; }
        public bool Realize { get; private set; }

        public DeferredElementCustomRuntimeData(CustomWriterRuntimeDataTypeIndex version)
            : base(version)
        {
        }

        internal static DeferredElementCustomRuntimeData CreateAndDeserializeRuntimeData(XbfReader reader, CustomWriterRuntimeDataTypeIndex typeIndex)
        {
            var name = reader.ReadSharedString();
            var nonDeferredProperties = new List<Tuple<XamlProperty, object>>();
            var realize = false;

            if (typeIndex != CustomWriterRuntimeDataTypeIndex.DeferredElement_v1)
            {
                nonDeferredProperties = reader.ReadVector((r) =>
                {
                    var property = r.ReadXamlProperty();
                    var value = r.ReadConstant();

                    return new Tuple<XamlProperty, object>(property, value);
                }, true);

                if (typeIndex != CustomWriterRuntimeDataTypeIndex.DeferredElement_v2)
                {
                    realize = reader.ReadBoolean();
                }
            }

            return new DeferredElementCustomRuntimeData(typeIndex)
            {
                Name = name,
                NonDeferredProperties = nonDeferredProperties,
                Realize = realize
            };
        }
    }
}