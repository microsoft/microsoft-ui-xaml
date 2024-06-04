// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class ApiInformation
    {
        private static IReadOnlyDictionary<string, ApiInformationMethod> SupportedApiInformation =
            new Dictionary<string, ApiInformationMethod>()
        {
            { "IsApiContractPresent",       new ApiInformationMethod("IsApiContractPresent", true) },
            { "IsApiContractNotPresent",    new ApiInformationMethod("IsApiContractPresent", false) },
            { "IsPropertyPresent",          new ApiInformationMethod("IsPropertyPresent", true) },
            { "IsPropertyNotPresent",       new ApiInformationMethod("IsPropertyPresent", false) },
            { "IsTypePresent",              new ApiInformationMethod("IsTypePresent", true) },
            { "IsTypeNotPresent",           new ApiInformationMethod("IsTypePresent", false) },
        };

        private static IReadOnlyDictionary<string, List<ApiInformationParameter>> SupportedApiInformationParameters =
            new Dictionary<string, List<ApiInformationParameter>>()
        {
            { "IsApiContractPresent2",  new List<ApiInformationParameter>() { new ApiInformationParameter(typeof(string)), new ApiInformationParameter(typeof(ushort)) }},
            { "IsApiContractPresent3",  new List<ApiInformationParameter>() { new ApiInformationParameter(typeof(string)), new ApiInformationParameter(typeof(ushort)), new ApiInformationParameter(typeof(ushort)) }},
            { "IsTypePresent1",         new List<ApiInformationParameter>() { new ApiInformationParameter(typeof(string)) }},
            { "IsPropertyPresent2",     new List<ApiInformationParameter>() { new ApiInformationParameter(typeof(string)), new ApiInformationParameter(typeof(string)) }},
        };

        public ApiInformationMethod Method { get; }
        public IEnumerable<ApiInformationParameter> Parameters { get; private set; }

        public ApiInformation(string methodName)
        {
            if (!SupportedApiInformation.ContainsKey(methodName))
            {
                throw new ArgumentException("methodName");
            }
            Method = SupportedApiInformation[methodName];
        }

        internal void SetParameters(List<ApiInformationParameter> parameters)
        {
            if (!SupportedApiInformationParameters.ContainsKey(Method.MethodName + parameters.Count))
            {
                throw new ArgumentException("parameters");
            }
            List<ApiInformationParameter> supportedParameters = SupportedApiInformationParameters[Method.MethodName + parameters.Count];
            if (supportedParameters.Count != parameters.Count)
            {
                throw new ArgumentException("parameters");
            }

            for (int i = 0; i < parameters.Count; i++)
            {
                parameters[i].ParameterType = supportedParameters[i].ParameterType;
            }
            Parameters = parameters;
        }

        public string MemberFriendlyName => UniqueName.GetMemberFriendlyName();

        public string UniqueName => string.Format("{0}_{1}", Method.UniqueName, string.Join("_", Parameters.Select(p => p.UniqueName)));

        public override bool Equals(object obj)
        {
            ApiInformation other = obj as ApiInformation;
            return (other != null && other.UniqueName == UniqueName);
        }

        public override int GetHashCode()
        {
            return UniqueName.GetHashCode();
        }
    }
}
