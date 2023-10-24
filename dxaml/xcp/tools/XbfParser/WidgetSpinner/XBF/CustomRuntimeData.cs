// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Metadata;
using System;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public enum CustomWriterRuntimeDataTypeIndex
    {
        Unknown = 0,
        VisualStateGroupCollection_v2 = 1,
        Style_v1 = 2,
        VisualStateGroupCollection_v3 = 3,
        VisualStateGroupCollection_v4 = 4,
        VisualStateGroupCollection_v5 = 5,
        DeferredElement_v2 = 6,
        ResourceDictionary_v2 = 7,
        Style_v2 = 8,
        DeferredElement_v3 = 9,
        ResourceDictionary_v3 = 10,

        // Legacy values derived from StableXbfTypeIndex
        DeferredElement_v1 = StableXbfTypeIndex.DeferredElement,                        // 745
        ResourceDictionary_v1 = StableXbfTypeIndex.ResourceDictionary,                  // 371
        VisualStateGroupCollection_v1 = StableXbfTypeIndex.VisualStateGroupCollection,  // 420
    }

    public struct StreamOffsetToken
    {
        public static StreamOffsetToken Default { get; } = new StreamOffsetToken(-1);

        public long Offset { get; }

        internal StreamOffsetToken(long offset)
        {
            if (offset < -1)
            {
                throw new ArgumentException(nameof(offset));
            }

            Offset = offset;
        }

        public override bool Equals(object obj)
        {
            if (!(obj is StreamOffsetToken))
            {
                return false;
            }

            var token = (StreamOffsetToken)obj;
            return Offset == token.Offset;
        }

        public override int GetHashCode()
        {
            return Offset.GetHashCode();
        }

        public override string ToString()
        {
            return string.Format("Nodestream offset: {0}", Offset);
        }
    }

    public abstract class CustomRuntimeData
    {
        public CustomWriterRuntimeDataTypeIndex Version { get; }

        protected CustomRuntimeData(CustomWriterRuntimeDataTypeIndex version)
        {
            Version = version;
        }

        public override string ToString()
        {
            return string.Format("Type: {0}", Version);
        }
    }
}
