// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.Windows.Design.Metadata;

namespace Microsoft.UI.Xaml.Design
{
    class RegisterMetadata : IProvideAttributeTable
    {
        public AttributeTable AttributeTable
        {
            get
            {
                AttributeTableBuilder attributeTableBuilder = new AttributeTableBuilder();
                return attributeTableBuilder.CreateTable();
            }
        }
    }
}
