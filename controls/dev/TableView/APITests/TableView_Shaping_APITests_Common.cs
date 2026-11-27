// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls.Tabular;
using Microsoft.UI.Xaml.Data;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Runtime.CompilerServices;

using static Microsoft.UI.Xaml.Tests.MUXControls.ApiTests.TableViewTestHelpers;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    internal static class TableViewShapingTestHelpers
    {
        // Six people over three roles and three departments, in an order that is sorted by none of
        // them, so an accidental default shape shows up immediately.
        internal static List<ShapedPerson> MakeShapedPeople() => new List<ShapedPerson>
        {
            new ShapedPerson("Asha", "Designer", "Studio"),
            new ShapedPerson("Diego", "Engineer", "Platform"),
            new ShapedPerson("Mei", "Architect", "Research"),
            new ShapedPerson("Rafa", "Engineer", "Platform"),
            new ShapedPerson("Ines", "Designer", "Studio"),
            new ShapedPerson("Owen", "Engineer", "Platform"),
        };

        internal static ShapedPerson Person(object item) => (ShapedPerson)item;

        // Tall enough that every row in these fixtures realizes, so TryGetElement never legitimately
        // returns null.
        internal static TableView CreateShapedTable(object source)
            => CreateTableView(source, headers: new[] { "Name", "Role" }, width: 500, height: 700);

        internal static int GroupHeaderCount(TableView tableView)
            => GetProjectedLabels(tableView, includeCount: true).Count(label => label.StartsWith("#", StringComparison.Ordinal));

        // The group keys in projected order, without their counts.
        internal static List<string> GroupKeys(TableView tableView)
            => GetProjectedLabels(tableView, includeCount: true)
                .Where(label => label.StartsWith("#", StringComparison.Ordinal))
                .Select(label => label.Substring(1, label.IndexOf('(') - 1))
                .ToList();

        internal static void VerifyProjection(TableView tableView, IList<string> expected, string context)
            => VerifySequence(expected, GetProjectedLabels(tableView, includeCount: true), context);

        // A CollectionViewSource over items, optionally grouped by groupPath and sorted by sortPath.
        // Bind its View: the view owns the shaping and holds the CollectionViewSource only weakly.
        internal static CollectionViewSource MakeShapingView(object items, string groupPath = null, string sortPath = null,
            CollectionViewSortDirection direction = CollectionViewSortDirection.Ascending)
        {
            var cvs = new CollectionViewSource { Source = items };
            if (groupPath != null)
            {
                cvs.GroupDescriptions.Add(new PropertyGroupDescription(groupPath));
            }

            if (sortPath != null)
            {
                cvs.SortDescriptions.Add(new Microsoft.UI.Xaml.Data.SortDescription(sortPath, direction));
            }

            return cvs;
        }

        // Flat live fixture: source order A B C D, scores ascending, teams alternating X/Y.
        internal static ObservableCollection<LiveRow> MakeLiveFlat() => new ObservableCollection<LiveRow>(new[]
        {
            new LiveRow { Id = 1, Name = "A", Score = 10, Team = "X" },
            new LiveRow { Id = 2, Name = "B", Score = 20, Team = "Y" },
            new LiveRow { Id = 3, Name = "C", Score = 30, Team = "X" },
            new LiveRow { Id = 4, Name = "D", Score = 40, Team = "Y" },
        });

        internal static T Named<T>(IEnumerable<T> items, string name) where T : class
            => items.First(i => NameOf(i) == name);

        internal static string NameOf(object item)
        {
            switch (item)
            {
                case LiveRow l: return l.Name;
                case ShapedPerson s: return s.Name;
                default: return "?";
            }
        }

        // A single Name text template column over the source.
        internal static TableView CreateNameTable(object source)
        {
            var tableView = new TableView { Width = 400, Height = 600 };
            tableView.Columns.Add(MakeTextTemplateColumn("Name", width: new GridLength(1, GridUnitType.Star)));
            tableView.ItemsSource = source;
            return tableView;
        }

        // Projected rows as one space-separated string: a data row's Name, a group header's [key].
        internal static string RowLabels(TableView tableView)
            => string.Join(" ", GetProjectedElements(tableView).Select(element => element switch
            {
                TableViewGroupHeader header => header.Content is TableViewGroupInfo info ? $"[{info.Key}]" : "[H]",
                TableViewRow row => NameOf(row.DataContext),
                _ => "?",
            }));

        internal static string ThrownTypeName(Action action)
        {
            try
            {
                action();
                return "no exception";
            }
            catch (Exception ex)
            {
                return ex.GetType().Name;
            }
        }
    }

    // Observable row for live shaping; PropertyChanged is the only signal live shaping listens to.
    internal sealed class LiveRow : INotifyPropertyChanged
    {
        private string _name = "";
        private int _score;
        private string _team = "";

        public int Id { get; set; }
        public string Name { get => _name; set => Set(ref _name, value); }
        public int Score { get => _score; set => Set(ref _score, value); }
        public string Team { get => _team; set => Set(ref _team, value); }

        public event PropertyChangedEventHandler PropertyChanged;

        private void Set<T>(ref T field, T value, [CallerMemberName] string name = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value))
            {
                return;
            }

            field = value;
            PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
        }
    }

    // A reference-type value, so "Department.Name" exercises the dotted-path evaluator.
    internal sealed class Department
    {
        public string Name { get; set; }

        public override string ToString() => Name;
    }

    internal sealed class ShapedPerson : INotifyPropertyChanged
    {
        private string m_name;
        private string m_role;
        private string m_departmentName;

        public ShapedPerson(string name, string role, string departmentName)
        {
            m_name = name;
            m_role = role;
            m_departmentName = departmentName;
            Department = new Department { Name = departmentName };
        }

        public event PropertyChangedEventHandler PropertyChanged;

        public string Name
        {
            get => m_name;
            set
            {
                if (m_name != value)
                {
                    m_name = value;
                    Raise(nameof(Name));
                }
            }
        }

        public string Role
        {
            get => m_role;
            set
            {
                if (m_role != value)
                {
                    m_role = value;
                    Raise(nameof(Role));
                }
            }
        }

        public string DepartmentName
        {
            get => m_departmentName;
            set
            {
                if (m_departmentName != value)
                {
                    m_departmentName = value;
                    Department = new Department { Name = value };
                    Raise(nameof(DepartmentName));
                    Raise(nameof(Department));
                }
            }
        }

        // An int property, for an int group key.
        public int NameLength => m_name.Length;

        // Nested object, so "Department.Name" exercises the dotted-path evaluator.
        public Department Department { get; private set; }

        // Makes a row stale without announcing it.
        internal void SetNameWithoutNotification(string name) => m_name = name;

        private void Raise(string propertyName)
            => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    internal sealed class PlainShapedPerson
    {
        public string Name { get; set; }
        public string Role { get; set; }
    }
}
