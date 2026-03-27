// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System;
using System.Collections.Generic;
using System.Threading.Tasks;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.Serialization
{
    partial class CustomPage
        : Page
    {
        private partial class GroupHeader
            : List<object>
            , ICustomPropertyProvider
        {
            private string m_Header;
            private Type m_Type;

            public string Header
            {
                get
                {
                    return m_Header;
                }

                set
                {
                    m_Header = value;
                }
            }

            public GroupHeader(string header)
            {
                m_Header = header;
                m_Type = typeof(Microsoft.UI.Xaml.Controls.ListViewHeaderItem);
            }

            public Type Type
            {
                get
                {
                    return m_Type;
                }
            }

            public ICustomProperty GetCustomProperty(string name)
            {
                return null;
            }

            public ICustomProperty GetIndexedProperty(string name, Type type)
            {
                throw new NotImplementedException();
            }

            public string GetStringRepresentation()
            {
                return m_Header;
            }
        }

        public int FirstVisibleIndex
        {
            get
            {
                return (InnerListView.ItemsPanelRoot as ItemsStackPanel).FirstVisibleIndex;
            }
        }

        public CustomPage(bool isGrouped, bool scrollInLoaded, bool header = false, bool footer = false)
        {
            InnerListView = (ListView)XamlReader.Load(
                   @"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'
                            Width='200' Height='200'>
                        <ListView.Resources>
                          <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>
                        </ListView.Resources>
                        <ListView.ItemTemplate>
                            <DataTemplate>
                                <TextBlock Text='{Binding}' FontSize='20'/>
                            </DataTemplate>
                        </ListView.ItemTemplate>
                        <ListView.GroupStyle>
                            <GroupStyle>
                                <GroupStyle.HeaderTemplate>
                                    <DataTemplate>
                                        <Grid Background='Red'>
                                            <TextBlock Text='{Binding}' Foreground='White' FontSize='30'/>
                                        </Grid>
                                    </DataTemplate>
                                </GroupStyle.HeaderTemplate>
                            </GroupStyle>
                        </ListView.GroupStyle>
                    </ListView>");

            CollectionViewSource cvs = new CollectionViewSource();
            cvs.Source = GetData(isGrouped);
            cvs.IsSourceGrouped = isGrouped;

            InnerListView.ItemsSource = cvs.View;

            StackPanel stackPanel = new StackPanel();
            stackPanel.Children.Add(new Button() { Content = "ButtonBeforeListViewToAvoidFocusIssue" });
            stackPanel.Children.Add(InnerListView);
            Content = stackPanel;

            isHeader = header;
            isFooter = footer;
            if (header)
            {
                InnerListView.Header = new Button() { Content = "Header", Width = 200, Height = 600 };
            }

            if (footer)
            {
                InnerListView.Footer = new Button() { Content = "Footer", Width = 200, Height = 600 };
            }

            if (scrollInLoaded)
            {
                this.Loaded += (s, e) =>
                {
                    ScrollListWithListViewPersistenceHelper();
                };
            }
            else
            {
                ScrollListWithListViewPersistenceHelper();
            }
        }

        private async void ScrollListWithListViewPersistenceHelper()
        {
            var persistedValue = string.Empty;

            if (isHeader)
            {
                // scroll a litte bit in the header part
                persistedValue = "AAAAAQAAAAFAbgAAAAAAAAAAAAA=";
            }
            else if (isFooter)
            {
                // scroll past the items into the footer range
                persistedValue = "AAAAAQAAAAJAaQAAAAAAAAAAAAA=";
            }
            else
            {
                // scroll to the position using the provided string
                // this string scrolls to item index 3 when the data is grouped
                // this string scrolls to item index 4 when the data is NOT grouped
                persistedValue = "AAAAAQAAAAMAAAAAAAAAAAAAAAZJdGVtIDU=";
            }

            await ListViewPersistenceHelper.SetRelativeScrollPositionAsync(InnerListView, persistedValue, (s) => Task.FromResult((object)s).AsAsyncOperation());
        }

        private List<object> GetData(bool isGrouped)
        {
            List<object> data = new List<object>();

            if (isGrouped)
            {
                // 5 groups
                // 10 items each
                for (int i = 0; i < 5; ++i)
                {
                    GroupHeader gh = new GroupHeader("Group " + i.ToString());

                    for (int j = 0; j < 10; ++j)
                    {
                        gh.Add("Item " + j.ToString());
                    }

                    data.Add(gh);
                }
            }
            else
            {
                // 10 items
                for (int i = 0; i < 10; ++i)
                {
                    data.Add("Item " + i.ToString());
                }
            }

            return data;
        }

        public ListView InnerListView { get; set; }

        private bool isHeader = false;
        private bool isFooter = false;
    }
}
