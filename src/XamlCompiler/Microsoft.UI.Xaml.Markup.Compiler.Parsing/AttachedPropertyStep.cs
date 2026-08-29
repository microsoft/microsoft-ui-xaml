// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class AttachedPropertyStep : DependencyPropertyStep
    {
        public AttachedPropertyStep(string propertyName, XamlType valueType, XamlType ownerType,
            BindPathStep parent, ApiInformation apiInformation)
            : base(propertyName, valueType, ownerType, parent, apiInformation)
        {
        }

        public override string UniqueName => string.Format("A_{0}_{1}", OwnerType.UnderlyingType.FullName.GetMemberFriendlyName(), PropertyName);

        public override bool IsValueRequired
        {
            get
            {
                if (Parent.ImplementsINDEI)
                {
                    var memberInfo = this.OwnerType.UnderlyingType.GetMember(PropertyName + "Property");
                    if (memberInfo != null && memberInfo.Length > 0)
                    {
                        return DirectUI.ReflectionHelper.FindAttributeByShortTypeName(memberInfo[0], KnownTypes.RequiredAttribute) != null;
                    }
                }
                return false;
            }
        }
    }
}
