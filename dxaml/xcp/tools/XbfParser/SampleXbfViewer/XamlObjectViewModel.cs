// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.Xaml.WidgetSpinner.Model;

namespace SampleXbfViewer
{
    public class XamlObjectViewModel
    {
        private XamlObject m_xamlObject;

        public XamlObjectViewModel(XamlObject xamlObject)
        {
            this.m_xamlObject = xamlObject;
        }
    }
}