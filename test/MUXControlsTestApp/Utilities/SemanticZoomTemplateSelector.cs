// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Media;

namespace MUXControlsTestApp
{
    public class SemanticZoomTemplateSelector : DataTemplateSelector
    {
        public SemanticZoomTemplateSelector()
        {
            Templates = new List<DataTemplate>();
        }

        public List<DataTemplate> Templates { get;set; }
        
        protected override DataTemplate SelectTemplateCore(object item, DependencyObject container)
        {
            return base.SelectTemplateCore(item, container);
        }

        protected override DataTemplate SelectTemplateCore(object item)
        {
            if (Templates == null || Templates.Count < 1)
                return null;

            var itemTemplate = item as String;

            if (string.IsNullOrEmpty(itemTemplate))
            {
                return null;
            }


            DataTemplate defaultTemplate = Templates[0];

            var match = from m in Templates
                        where m.GetValue(FrameworkElement.NameProperty).ToString() == itemTemplate
                        select m;
            if (match != null && match.Any())
                return match.First();

            return defaultTemplate;
        }
    }
}
