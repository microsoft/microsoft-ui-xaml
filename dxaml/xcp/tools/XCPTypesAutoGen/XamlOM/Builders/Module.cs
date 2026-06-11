// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;

namespace XamlOM
{
    [AttributeUsage(AttributeTargets.Assembly)]
    public class ModuleAttribute : Attribute
    {
        public string Name { get; set; }
        public string IdlGroupFileName { get; set; }
        public string ManifestId { get; set; }
        public string DllName { get; set; }

        private OM.ModuleDefinition omModule;

        public OM.ModuleDefinition OMModule
        {
            get
            {
                if ( this.omModule == null)
                {
                    this.omModule = new OM.ModuleDefinition(this.Name, this.IdlGroupFileName, this.ManifestId, this.DllName);
                }

                return this.omModule;
            }
        }
    }
}
