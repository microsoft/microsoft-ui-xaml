// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class MethodStep : BindPathStep
    {
        private MemberInfo[] methodInfos;
        private readonly IList<Parameter> parameters;
        public override bool IsIncludedInUpdate => false;
        public override string UniqueName { get; }
        public override XamlType ValueType { get; }
        public bool IsOverloaded { get; }
        public bool IsStatic { get; }
        public string MethodName { get; }
        public XamlType OwnerType { get; }

        public MethodStep(MemberInfo[] memberInfos, XamlType ownerType, BindPathStep parent, ApiInformation apiInformation)
            : this(memberInfos, 0, ownerType, parent, apiInformation)
        { }

        private MethodStep(MemberInfo[] memberInfos, uint selectedOverload, XamlType ownerType, BindPathStep parent, ApiInformation apiInformation)
            : base(null, parent, apiInformation)
        {
            if (memberInfos.Length < 1)
            {
                throw new ArgumentException("methodInfos must not be empty");
            }
            MethodInfo selectedMethod = memberInfos[selectedOverload] as MethodInfo;
            methodInfos = memberInfos;
            MethodName = selectedMethod.Name;
            IsOverloaded = memberInfos.Length > 1;
            IsStatic = selectedMethod.IsStatic;
            OwnerType = ownerType;
            ValueType = OwnerType.SchemaContext.GetXamlType(selectedMethod.ReturnType);
            UniqueName = string.Format("M_{0}{1}", MethodName, selectedOverload == 0 ? "" : selectedOverload.ToString());
            parameters = selectedMethod.GetParameters().Select(p => new Parameter(p)).ToList();
        }

        public IReadOnlyList<Parameter> Parameters => parameters as IReadOnlyList<Parameter>;

        public MethodStep GetOverload(int parameterCount)
        {
            for (uint i = 0; i < methodInfos.Length; i++)
            {
                MethodInfo method = methodInfos[i] as MethodInfo;
                if (method.GetParameters().Length == parameterCount)
                {
                    return new MethodStep(methodInfos, i, OwnerType, Parent, ApiInformation);
                }
            }
            throw new ArgumentException();
        }
    }
}
