// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Xaml;

namespace Microsoft.UI.Xaml.Markup.Compiler
{
    public class MapIndexStep : BindPathStep
    {
        private readonly string keyHashCode;

        public string Key { get; }

        public MapIndexStep(string key, XamlType valueType, BindPathStep parent, ApiInformation apiInformation)
            : base(valueType, parent, apiInformation)
        {
            Key = key;
            keyHashCode = ((uint)key.GetHashCode()).ToString();
        }

        public override string UniqueName => string.Format("K{0}", keyHashCode);
    }
}
