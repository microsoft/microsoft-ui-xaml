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
    public abstract class AbstractFile
    {
        private readonly string fileName;

        protected AbstractFile(string fileName)
        {
            this.fileName = fileName;
        }

        public virtual void ValidateFileExists()
        {
            if (!File.Exists(this.fileName))
            {
                throw new UserArgumentException(string.Format("{0} does not exist", this.GetType().Name));
            }
        }

        public void ValidateFileSpecified()
        {
            if (string.IsNullOrWhiteSpace(this.fileName))
            {
                throw new UserArgumentException(string.Format("{0} file is not specified", this.GetType().Name));
            }
        }

        public virtual string GetContent()
        {
            return File.ReadAllText(this.fileName);
        }

        public override string ToString()
        {
            return $"{this.GetType().Name}:{this.fileName}";
        }
    }
}
