// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls.Tabular;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;

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
