// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;
using Windows.Foundation;

namespace LibManagedWinmd
{
    public struct NestedGeometry
    {
        public Size Size;
        public int Unchanged;
    }

    public sealed class SchemaTypeHolder : Control
    {
        public Size MySize { get; set; }
        public Rect MyRect { get; set; }
        public NestedGeometry Geometry { get; set; }
        public IList<Size> Sizes { get; set; }
        public IDictionary<string, Size> SizeMap { get; set; }

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
