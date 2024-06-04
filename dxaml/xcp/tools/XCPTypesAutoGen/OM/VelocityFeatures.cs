// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using System.Collections.Generic;
using System.Reflection;

namespace OM
{
    /// <summary>
    /// Creates a map between Velocity Feature names and versions.
    /// </summary>
    public static class VelocityFeatures
    {
        public class ExtraData
        {
            public bool SkipRuntimeChecks = false;
        }

        private static int _nextFeatureVersion = OSVersions.VelocityFeatureVersionFirst;
        private static Dictionary<string, int> _features = new Dictionary<string, int>();
        private static List<string> _versions = new List<string>();
        private static Dictionary<int, ExtraData> _extraData = new Dictionary<int, ExtraData>();
        private static ExtraData _defaultExtraData = new ExtraData();

        public static int GetVersion(string featureName)
        {
            if (string.IsNullOrWhiteSpace(featureName))
            {
                throw new InvalidOperationException("Velocity feature must be a non-blank string");
            }

            if (_features.ContainsKey(featureName)) return _features[featureName];
            if (_nextFeatureVersion > OSVersions.VelocityFeatureVersionLast)
            {
                throw new InvalidOperationException(string.Format("Too many unique Velocity Features defined (max={0})", OSVersions.VelocityFeatureVersionLast - OSVersions.VelocityFeatureVersionFirst + 1));
            }


            if (_nextFeatureVersion > OSVersions.VelocityFeatureVersionLast)
            {
                throw new InvalidOperationException(string.Format("Too many unique Velocity Features defined (max={0}", OSVersions.VelocityFeatureVersionLast - OSVersions.VelocityFeatureVersionFirst + 1));

            }


            _features.Add(featureName, _nextFeatureVersion);
            _versions.Add(featureName);
            return _nextFeatureVersion++;
        }

        public static string GetFeatureName(int version)
        {
            if (!IsVelocityVersion(version))
            {
                throw new InvalidOperationException(string.Format("Velocity feaure name requested for invalid versions version {0} (valid range {1} to {2}",version, OSVersions.VelocityFeatureVersionFirst, OSVersions.VelocityFeatureVersionLast - 1));
            }

            return _versions[version - OSVersions.VelocityFeatureVersionFirst];
        }

        public static bool IsVelocityVersion(int version)
        {
            return version >= OSVersions.VelocityFeatureVersionFirst && version < _nextFeatureVersion;

        }

        public static bool ContextHasVelocityFeature(OMContextView view)
        {
            foreach (NamespaceDefinition ns in view.Namespaces)
            {
                foreach (ClassDefinition definition in ns.Classes)
                {
                    if (definition.VelocityVersion != 0) return true;
                    foreach (ClassVersion version in definition.Versions)
                    {
                        if (VelocityFeatures.IsVelocityVersion(version.Version))
                        {
                            return true;
                        }
                    }
                }
                foreach (EnumDefinition definition in ns.Enums)
                {
                    if (definition.VelocityVersion != 0)
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        private static ExtraData EnsureExtraData(int id)
        {
            if (!_extraData.ContainsKey(id))
            {
                _extraData[id] = new ExtraData();
            }
            return _extraData[id];
        }

        private static ExtraData GetExtraData(int id)
        {
            if (!_extraData.ContainsKey(id))
            {
                return _defaultExtraData;
            }
            return _extraData[id];
        }

        public static void SetSkipRuntimeChecks(int id, bool val)
        {
            ExtraData data = EnsureExtraData(id);
            data.SkipRuntimeChecks = val;
        }

        public static bool ShouldSkipRuntimeChecks(int id)
        {
            return GetExtraData(id).SkipRuntimeChecks;
        }

        public static List<string> GetClassVelocityFeatures(ClassDefinition definition)
        {
            List<string> features = new List<string>();

            if (definition.VelocityVersion != 0)
            {
                // Our whole class is protected so there can be no individual features on versions
                features.Add(VelocityFeatures.GetFeatureName(definition.VelocityVersion));
            }
            else
            {
                // See if any of our interfaces are protected
                foreach (ClassVersion version in definition.Versions)
                {
                    if (VelocityFeatures.IsVelocityVersion(version.Version))
                    {
                        features.Add(VelocityFeatures.GetFeatureName(version.Version));
                    }
                }
            }
            return features;
        }

        public static string GetOverrideString(int version, int velocityVersion = 0)
        {
            if (!VelocityFeatures.IsVelocityVersion(version) && velocityVersion == 0)
            {
                // Not a velocity feature version so return standard override string
                return "override";
            }

            // return override macro for the velocity feature
            return VelocityFeatures.GetFeatureName(velocityVersion != 0 ? velocityVersion : version).ToUpper() + "_OVERRIDE";
        }

        public static string GetQueryInterfaceClause(int version)
        {
            if (!VelocityFeatures.IsVelocityVersion(version))
            {
                // Not a velocity feature version so return standard override string
                return string.Empty;
            }

            if (ShouldSkipRuntimeChecks(version))
            {
                return string.Empty;
            }

            // return override macro for the velocity feature
            return " && " + VelocityFeatures.GetFeatureName(version) + "::IsEnabled()";

        }
    }
}
