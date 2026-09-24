// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;

namespace LibManagedWinmd
{
    public sealed class SchemaTypeHolder : Control
    {
        MyIDictionary _dict = new MyIDictionary();
        public MyIDictionary Dictionary
        {
            get { return _dict; }
        }

        MyIDictionaryWith2Adds _dict2 = new MyIDictionaryWith2Adds();
        public MyIDictionaryWith2Adds DictionaryWith2Adds
        {
            get { return _dict2; }
        }
    }
}
