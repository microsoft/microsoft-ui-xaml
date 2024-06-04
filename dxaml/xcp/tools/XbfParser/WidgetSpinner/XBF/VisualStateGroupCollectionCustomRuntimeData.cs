// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class VisualStateEssence
    {
        public string Name { get; private set; }
        public StreamOffsetToken DeferredStoryBoardToken { get; private set; }
        public bool HasStoryboard { get; private set; }
        public List<StreamOffsetToken> DeferredPropertySetterTokens { get; private set; }
        public List<List<int>> StateTriggerValues { get; private set; }
        public List<StreamOffsetToken> ExtensibleStateTriggerTokens { get; private set; }
        public List<StreamOffsetToken> StateTriggerCollectionTokens { get; private set; }
        public List<StreamOffsetToken> StaticResourceTriggerTokens { get; private set; }

        internal VisualStateEssence()
        {
        }

        internal static VisualStateEssence Deserialize(XbfReader reader, CustomWriterRuntimeDataTypeIndex typeIndex)
        {
            var name = reader.ReadSharedString();
            var deferredStoryBoardToken = reader.ReadStreamOffsetToken();
            var hasStoryboard = reader.ReadBoolean();

            var essence = new VisualStateEssence()
            {
                Name = name,
                DeferredStoryBoardToken = deferredStoryBoardToken,
                HasStoryboard = hasStoryboard
            };

            if (typeIndex != CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v1)
            {
                essence.DeferredPropertySetterTokens = reader.ReadVector((r) => r.ReadStreamOffsetToken(), true);
                essence.StateTriggerValues =
                    reader.ReadVector((r) => r.ReadVector((r2) => r2.Read7BitEncodedInt(), true), true);

                if (typeIndex != CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v2)
                {
                    essence.ExtensibleStateTriggerTokens = reader.ReadVector((r) => r.ReadStreamOffsetToken(), true);
                    essence.StateTriggerCollectionTokens = reader.ReadVector((r) => r.ReadStreamOffsetToken(), true);

                    if (typeIndex != CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v3 &&
                        typeIndex != CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v4)
                    {
                        essence.StaticResourceTriggerTokens = reader.ReadVector((r) => r.ReadStreamOffsetToken(), true);
                    }
                }
            }

            return essence;
        }
    }

    public class VisualStateGroupEssence
    {
        public string Name { get; private set; }
        public bool HasDynamicTimelines { get; private set; }
        public StreamOffsetToken DeferredSelf { get; private set; }

        internal VisualStateGroupEssence()
        {
        }

        internal static VisualStateGroupEssence Deserialize(XbfReader reader)
        {
            var name = reader.ReadSharedString();
            var hasDynamicTimelines = reader.ReadBoolean();
            var deferredSelf = reader.ReadStreamOffsetToken();

            return new VisualStateGroupEssence()
            {
                Name = name,
                HasDynamicTimelines = hasDynamicTimelines,
                DeferredSelf = deferredSelf
            };
        }
    }

    public class VisualTransitionEssence
    {
        public string ToState { get; private set; }
        public string FromState { get; private set; }
        public StreamOffsetToken DeferredSelf { get; private set; }

        internal VisualTransitionEssence()
        {
        }

        internal static VisualTransitionEssence Deserialize(XbfReader reader)
        {
            var toState = reader.ReadSharedString();
            var fromState = reader.ReadSharedString();
            var deferredSelf = reader.ReadStreamOffsetToken();

            return new VisualTransitionEssence()
            {
                ToState = toState,
                FromState = fromState,
                DeferredSelf = deferredSelf
            };
        }
    }

    public class VisualStateIndices
    {
        public int FromIndex { get; private set; }
        public int ToIndex { get; private set; }

        internal VisualStateIndices()
        {
        }

        internal static VisualStateIndices Deserialize(XbfReader reader)
        {
            var fromIndex = reader.Read7BitEncodedInt();
            var toIndex = reader.Read7BitEncodedInt();

            return new VisualStateIndices() { FromIndex = fromIndex, ToIndex = toIndex };
        }
    }

    public class VisualTransitionTableOptimizedLookup
    {
        public Dictionary<VisualStateIndices, int> VisualStateToTransitionMap { get; private set; }
        public List<int> GroupToDefaultTransitionMap { get; private set; }

        internal VisualTransitionTableOptimizedLookup() { }

        internal static VisualTransitionTableOptimizedLookup Deserialize(XbfReader reader)
        {
            var table = new VisualTransitionTableOptimizedLookup();
            var tempList = reader.ReadVector((r) =>
            {
                var indices = VisualStateIndices.Deserialize(r);
                var transition = r.Read7BitEncodedInt();

                return new Tuple<VisualStateIndices, int>(indices, transition);
            }, true);
            table.GroupToDefaultTransitionMap = reader.ReadVector((r) => r.Read7BitEncodedInt(), true);

            table.VisualStateToTransitionMap = new Dictionary<VisualStateIndices, int>();
            foreach (var kvp in tempList)
            {
                table.VisualStateToTransitionMap.Add(kvp.Item1, kvp.Item2);
            }

            return table;
        }
    }

    public class VisualStateGroupCollectionCustomRuntimeData : CustomRuntimeData
    {
        public List<int> VisualStateToGroupMap { get; private set; }
        public List<VisualStateEssence> VisualStates { get; private set; }
        public List<VisualStateGroupEssence> VisualStateGroups { get; private set; }
        public List<VisualTransitionEssence> VisualTransitions { get; private set; }
        public bool UnexpectedTokensDetected { get; private set; }
        public VisualTransitionTableOptimizedLookup VisualTransitionLookup { get; private set; }
        public StreamOffsetToken EntireCollectionToken { get; private set; }
        public List<string> SeenNameDirectives { get; private set; }

        internal VisualStateGroupCollectionCustomRuntimeData(CustomWriterRuntimeDataTypeIndex version)
            : base(version)
        {

        }

        internal static VisualStateGroupCollectionCustomRuntimeData CreateAndDeserializeRuntimeData(XbfReader reader, CustomWriterRuntimeDataTypeIndex typeIndex)
        {
            // The full data structure and how it is used is fairly complex, but luckily
            // the StreamOffsetToken for the entire VisualStateGroupCollection is recorded. 
            // The various unused members can be ignored, but they must still be read.
            var visualStateToGroupMap = reader.ReadVector((r) => r.Read7BitEncodedInt(), true);
            var visualStates = reader.ReadVector((r) => VisualStateEssence.Deserialize(r, typeIndex), true);
            var visualStateGroups = reader.ReadVector(VisualStateGroupEssence.Deserialize, true);
            var visualTransitions = reader.ReadVector(VisualTransitionEssence.Deserialize, true);
            var unexpectedTokensDetected = reader.ReadBoolean();
            var visualTransitionLookup = VisualTransitionTableOptimizedLookup.Deserialize(reader);
            var entireCollectionToken = reader.ReadStreamOffsetToken();
            var seenNameDirectives = new List<string>();
            if (typeIndex == CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v4 ||
                typeIndex == CustomWriterRuntimeDataTypeIndex.VisualStateGroupCollection_v5)
            {
                seenNameDirectives = reader.ReadVector((r) => r.ReadSharedString(), true);
            }

            return new VisualStateGroupCollectionCustomRuntimeData(typeIndex)
            {
                VisualStateToGroupMap = visualStateToGroupMap,
                VisualStates = visualStates,
                VisualStateGroups = visualStateGroups,
                VisualTransitions = visualTransitions,
                UnexpectedTokensDetected = unexpectedTokensDetected,
                VisualTransitionLookup = visualTransitionLookup,
                EntireCollectionToken = entireCollectionToken,
                SeenNameDirectives = seenNameDirectives
            };
        }
    }
}