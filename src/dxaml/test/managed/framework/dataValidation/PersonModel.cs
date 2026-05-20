// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using MockDataAnnotations;

namespace Microsoft.UI.Xaml.Tests.Framework.DataValidation
{
    public class PersonModel : ModelBase
    {
        [Required]
        public string Name
        {
            get { return GetValue() as string; }
            set { SetValue(value); }
        }

        [Required]
        [PhoneNumber]
        public string PhoneNumber
        {
            get { return GetValue() as string; }
            set { SetValue(value); }
        }

        [Required]
        [Min(13, "Users must be at least 13 years old")]
        public string Age
        {
            get { return GetValue() as string; }
            set { SetValue(value); }
        }
    }
}
