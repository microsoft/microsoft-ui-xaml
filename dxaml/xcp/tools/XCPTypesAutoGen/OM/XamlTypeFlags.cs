// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace OM
{
    public class XamlTypeFlags
    {
        /// <summary>
        /// Specifies whether the type allows to be associated with multiple objects.
        /// </summary>
        public bool AllowsMultipleAssociations { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether to generate the [muse] attribute in IDL.
        /// </summary>
        public bool GenerateMuseAttribute { get; set; }

        /// <summary>
        /// DateTime, Point and TimeSpan, despite being structs, have special provisions in the WinRT type system
        /// to be boxed as value types instead of references.
        /// </summary>
        public bool HasSpecialBoxer { get; set; }

        /// <summary>
        /// Generates type flag TYPE_NOT_CREATEABLE_FROM_XAML in XcpTypes.g.h when set to False.
        /// Example: INDEX_NOTIFY_EVENT_ARGS
        /// </summary>
        public bool IsCreateableFromXAML { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether this type should be webhosthidden
        /// </summary>
        public bool IsWebHostHidden { get; set; }

        public XamlTypeFlags()
        {
            IsCreateableFromXAML = true;
        }
    }
}
