// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.UI.Xaml.Design.ControlProvider;
using Microsoft.Windows.Design.Metadata;

namespace Microsoft.UI.Xaml.Design
{
    class RegisterMetadata : IProvideAttributeTable
    {

        public AttributeTable AttributeTable
        {
            get
            {
                AttributeTableBuilder builder = new AttributeTableBuilder();

                new NumberBoxPropertyProvider(builder).AddProperties();

                return builder.CreateTable();
            }
        }
    }
}
