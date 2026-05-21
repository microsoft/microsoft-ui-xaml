// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace XmlValidation
{
    public class ResourceFile: AbstractFile
    {
        private readonly byte[] resData;

        public ResourceFile(string fileName)
            : base(fileName)
        {
            var dirName = Path.GetDirectoryName(fileName);
            var isMaster = dirName.EndsWith(@"\test\resources\masters", StringComparison.OrdinalIgnoreCase);
            if (isMaster)
            {
                var resName = Path.GetFileName(fileName);
                this.resData = Interop.ReadTestResource(resName);
            }
        }

        public override void ValidateFileExists()
        {
            if (this.resData != null)
            {
                return;
            }
            base.ValidateFileExists();
        }

        public override string GetContent()
        {
            if (this.resData != null)
            {
                string result = System.Text.Encoding.UTF8.GetString(this.resData);
                return result;
            }
            return base.GetContent();
        }
    }
}
