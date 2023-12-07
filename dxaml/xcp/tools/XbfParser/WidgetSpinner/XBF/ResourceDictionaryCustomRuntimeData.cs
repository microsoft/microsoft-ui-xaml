// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Xaml.WidgetSpinner.Model;
using Microsoft.Xaml.WidgetSpinner.Reader;
using System;
using System.Collections.Generic;
using System.IO;

namespace Microsoft.Xaml.WidgetSpinner.XBF
{
    public class ResourceDictionaryCustomRuntimeData : CustomRuntimeData
    {
        public List<Tuple<string, StreamOffsetToken>> ExplicitKeyResources { get; }
        public List<Tuple<string, StreamOffsetToken>> ImplicitKeyResources { get; }
        public List<string> ResourcesWithXNames { get; }
        public Dictionary<StreamOffsetToken, List<XamlPredicateAndArgs>> ConditionallyDeclaredObjects { get; }

        internal ResourceDictionaryCustomRuntimeData(CustomWriterRuntimeDataTypeIndex version, List<Tuple<string, StreamOffsetToken>> explicitKeyResources,
            List<Tuple<string, StreamOffsetToken>> implicitKeyResources, List<string> resourcesWithXNames, Dictionary<StreamOffsetToken, List<XamlPredicateAndArgs>> conditionallyDeclaredObjects)
            : base(version)
        {
            ImplicitKeyResources = implicitKeyResources;
            ExplicitKeyResources = explicitKeyResources;
            ResourcesWithXNames = resourcesWithXNames;
            ConditionallyDeclaredObjects = conditionallyDeclaredObjects;
        }

        internal static ResourceDictionaryCustomRuntimeData CreateAndDeserializeRuntimeData(XbfReader reader,
            CustomWriterRuntimeDataTypeIndex typeIndex)
        {
            // A switch/case statement, despite being more verbose, makes it more obvious what the actual format is for any given
            // version of this data structure given that changes introduced in subsequent version tended to be complex rather
            // than simply additive.
            switch (typeIndex)
            {
                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v1:
                    {
                        var explicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);
                        var resourcesWithXNames = reader.ReadVector((r) => r.ReadSharedString(), true);
                        var implicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);
                        // In ResourceDictionary_v1, we stored a vector containing the keys for all implicit data templates.
                        // However, XAML doesn't truly support implicit data templates (unlike SL5 and WPF) so for ResourceDictionary_v2, 
                        // we removed all references to implicit data templates, but the stored data still needs to be deserialized from
                        // ResourceDictionary_v1.
                        reader.ReadVector((r) => r.ReadSharedString(), true);
                        // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys. This is actually
                        // redundant given the presence of m_implicitKeyResources (plus we only actually need the number of keys, not the
                        // keys themselves), so we were basically just wasting space. Starting with ResourceDictionary_v3, this information
                        // is no longer stored, but we still need to deserialize the data from v1 and v2.
                        reader.ReadVector((r) => r.ReadSharedString(), true);

                        return new ResourceDictionaryCustomRuntimeData(typeIndex, explicitKeyResources, implicitKeyResources, resourcesWithXNames, new Dictionary<StreamOffsetToken, List<XamlPredicateAndArgs>>());
                    }

                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v2:
                    {
                        var explicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);
                        var resourcesWithXNames = reader.ReadVector((r) => r.ReadSharedString(), true);
                        var implicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);
                        // Prior to ResourceDictionary_v3, we stored a vector containing the list of implicit style keys. This is actually
                        // redundant given the presence of m_implicitKeyResources (plus we only actually need the number of keys, not the
                        // keys themselves), so we were basically just wasting space. Starting with ResourceDictionary_v3, this information
                        // is no longer stored, but we still need to deserialize the data from v1 and v2.
                        reader.ReadVector((r) => r.ReadSharedString(), true);

                        return new ResourceDictionaryCustomRuntimeData(typeIndex, explicitKeyResources, implicitKeyResources, resourcesWithXNames, new Dictionary<StreamOffsetToken, List<XamlPredicateAndArgs>>());
                    }

                case CustomWriterRuntimeDataTypeIndex.ResourceDictionary_v3:
                    {
                        var explicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);
                        var resourcesWithXNames = reader.ReadVector((r) => r.ReadSharedString(), true);
                        var implicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var token = r.ReadStreamOffsetToken();

                            return new Tuple<string, StreamOffsetToken>(key, token);
                        }, true);

                        // TODO: do something with conditional keys
                        var conditionalExplicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var tokens = r.ReadVector((r2) => new StreamOffsetToken(r2.Read7BitEncodedInt()), true);

                            return new Tuple<string, List<StreamOffsetToken>>(key, tokens);
                        }, true);
                        var conditionalImplicitKeyResources = reader.ReadVector((r) =>
                        {
                            var key = r.ReadSharedString();
                            var tokens = r.ReadVector((r2) => new StreamOffsetToken(r2.Read7BitEncodedInt()), true);

                            return new Tuple<string, List<StreamOffsetToken>>(key, tokens);
                        }, true);
                        var conditionallyDeclaredObjectsAsList = reader.ReadVector((r) =>
                        {
                            var token = r.ReadStreamOffsetToken();
                            var xamlPredicatesAndArgsList = r.ReadVector((r2) => r2.ReadXamlPredicateAndArgs(), true);

                            return new Tuple<StreamOffsetToken, List<XamlPredicateAndArgs>>(token, xamlPredicatesAndArgsList);
                        }, true);
                        var conditionallyDeclaredObjects = new Dictionary<StreamOffsetToken, List<XamlPredicateAndArgs>>();
                        foreach (var kvp in conditionallyDeclaredObjectsAsList)
                        {
                            conditionallyDeclaredObjects.Add(kvp.Item1, kvp.Item2);
                        }

                        return new ResourceDictionaryCustomRuntimeData(typeIndex, explicitKeyResources, implicitKeyResources, resourcesWithXNames, conditionallyDeclaredObjects);
                    }
                default:
                    {
                        throw new InvalidDataException(string.Format("Not a known version of ResourceDictionaryCustomWriterRuntimeData: {0}", typeIndex));
                    }
            }

        }
    }
}