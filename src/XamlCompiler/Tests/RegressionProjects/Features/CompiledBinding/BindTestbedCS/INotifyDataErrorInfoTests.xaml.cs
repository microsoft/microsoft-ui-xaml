// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;

// The Blank Page item template is documented at https://go.microsoft.com/fwlink/?LinkId=234238

namespace BindTestbed
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class INotifyDataErrorInfoTests : Page
    {
        /// <summary>
        /// This is a comment for a constructor for a test class. Remove at your own peril.
        /// </summary>
        public INotifyDataErrorInfoTests()
        {
            this.InitializeComponent();
            ErrorModel = new BindTestbedModel.DataErrorModel();
            SCMErrorModel = new LanguageSpecificDataErrorModel();
            DOErrorModel = new BindTestbedModel.DODataErrorModel();
        }

        internal BindTestbedModel.DataErrorModel ErrorModel
        {
            get;
            set;
        }

        internal LanguageSpecificDataErrorModel SCMErrorModel
        {
            get;
            set;
        }

        internal BindTestbedModel.DODataErrorModel DOErrorModel
        {
            get;
            set;
        }
        internal void StringifyBindBack(string value)
        {
            this.ErrorModel.RequiredFunctionString = value;
        }
    }
}
