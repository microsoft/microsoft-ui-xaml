// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class TemplatePartDefinition
    {
        public string FieldName
        {
            get
            {
                return string.Format("m_tp{0}Part", Name);
            }
        }

        /// <summary>
        /// Gets or sets the pre-defined name of the part.
        /// </summary>
        public string Name
        {
            get;
            set;
        }

        public string VarName
        {
            get
            {
                return string.Format("p{0}Part", Name);
            }
        }

        /// <summary>
        /// Gets or sets the type of the named part this attribute is identifying.
        /// </summary>
        public TypeReference Type
        {
            get;
            set;
        }
    }
}
