// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using System;
using System.Collections.Generic;
using System.Linq;
using Microsoft.UI.Xaml.Data;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Collections.ObjectModel;

using Windows.Foundation.Collections;
using System.Collections;
using System.Collections.Specialized;
using Microsoft.UI.Xaml;
using Windows.UI.Popups;
using Microsoft.UI;

namespace BindTestbedModel
{
    public interface IManager : IEmployee
    {
        IEnumerable<IEmployee> ReportsEnum { get; }
        IList<IEmployee> ReportsList { get; }
        EmployeeCollection ReportsOC { get; }
        Windows.UI.Color FavoriteColor { get; set; }
    }

    internal class Manager : Employee, IManager
    {
        private EmployeeCollection _reports;
        private Windows.UI.Color _favoriteColor = Colors.AliceBlue;

        public Manager()
        {
            _reports = new EmployeeCollection();
        }

        public IEnumerable<IEmployee> ReportsEnum { get { return _reports; } }
        public IList<IEmployee> ReportsList { get { return _reports; } }
        public EmployeeCollection ReportsOC { get { return _reports; } }

        public Windows.UI.Color FavoriteColor
        {
            get { return _favoriteColor; }
            set { if (_favoriteColor != value) { _favoriteColor = value; NotifyPropertyChanged(); } }
        }

        protected override bool GetIsManager()
        {
            return true;
        }

        protected override string GetTitle()
        {
            return "Developer";
        }
    }
}
