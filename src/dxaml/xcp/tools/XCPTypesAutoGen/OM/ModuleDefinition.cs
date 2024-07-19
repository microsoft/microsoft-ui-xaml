// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#if SILVERLIGHTXASD_BUILD
namespace SilverlightXasd
#else
namespace OM
#endif
{
    public sealed class ModuleDefinition
    {
        private readonly string name;
        private readonly string idlGroupFileName;
        private readonly string manifestId;
        private readonly string dllName;

        public ModuleDefinition(string name, string idlGroupFileName, string manifestId, string dllName)
        {
            this.name = name;
            this.idlGroupFileName = idlGroupFileName;
            this.manifestId = manifestId;
            this.dllName = dllName;
        }

        public string IdlGroupFileName
        {
            get
            {
                return this.idlGroupFileName;
            }
        }

        public string ManifestId
        {
            get
            {
                return manifestId;
            }
        }

        public string DllName
        {
            get
            {
                return dllName;
            }
        }
    }
}
