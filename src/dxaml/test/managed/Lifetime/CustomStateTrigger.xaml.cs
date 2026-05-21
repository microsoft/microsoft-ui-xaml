// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace CustomTypes
{
    public sealed partial class CustomStateTrigger : StateTriggerBase
    {
        public CustomStateTrigger()
        {
        }

        private string _MemberStringQueried;
        public string MemberString
        {
            get
            {
                return _MemberStringQueried;
            }

            set
            {
                _MemberStringQueried = value;
                SetActive(_MemberStringQueried == MemberStringGlobal);
            }
        }

        public static string MemberStringGlobal { get; set; }
    }
}
