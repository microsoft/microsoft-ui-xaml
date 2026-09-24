using System;
using System.Linq;
using CommunityToolkit.Mvvm.ComponentModel;
using CommunityToolkit.Mvvm.Input;
using XamlTelemtryViewerWInui3.Models;

namespace XamlTelemtryViewerWInui3.ViewModels
{
    public sealed partial class FilterWindowViewModel : ObservableObject
    {
        [ObservableProperty]
        public partial FilterGroup? SelectedGroup { get; set; }

        [ObservableProperty]
        public partial FilterCondition? SelectedCondition { get; set; }

        [ObservableProperty]
        public partial string QueryText { get; set; } = string.Empty;

        [ObservableProperty]
        public partial string ParseError { get; set; } = string.Empty;

        public FilterWindowViewModel(EventFilter filter)
        {
            Filter = filter;
            SyncQueryFromFilter();
        }

        public EventFilter Filter { get; }

        /// <summary>
        /// Raised when the user clicks Apply to signal the main window
        /// to re-run filtering.
        /// </summary>
        public event Action? FilterApplied;

        [RelayCommand]
        private void AddGroup()
        {
            var group = new FilterGroup();
            group.Conditions.Add(new FilterCondition());
            
            if (Filter.Groups.Count > 0)
            {
                group.JoinWithPrevious = FilterJoin.And;
            }

            Filter.Groups.Add(group);
            SelectedGroup = group;
            SyncQueryFromFilter();
        }

        [RelayCommand]
        private void RemoveGroup()
        {
            if (SelectedGroup == null)
            {
                return;
            }

            Filter.Groups.Remove(SelectedGroup);
            SelectedGroup = Filter.Groups.LastOrDefault();
            SyncQueryFromFilter();
        }

        [RelayCommand]
        private void AddCondition()
        {
            if (SelectedGroup == null)
            {
                if (Filter.Groups.Count == 0)
                {
                    AddGroup();
                    return;
                }

                SelectedGroup = Filter.Groups.Last();
            }

            var condition = new FilterCondition();
            SelectedGroup.Conditions.Add(condition);
            SelectedCondition = condition;
            SyncQueryFromFilter();
        }

        [RelayCommand]
        private void RemoveCondition()
        {
            if (SelectedCondition == null || SelectedGroup == null)
            {
                return;
            }

            SelectedGroup.Conditions.Remove(SelectedCondition);
            SelectedCondition = null;

            // Remove empty groups automatically
            if (SelectedGroup.Conditions.Count == 0)
            {
                Filter.Groups.Remove(SelectedGroup);
                SelectedGroup = Filter.Groups.LastOrDefault();
            }

            SyncQueryFromFilter();
        }

        [RelayCommand]
        private void ClearAll()
        {
            Filter.Groups.Clear();
            SelectedGroup = null;
            SelectedCondition = null;
            QueryText = string.Empty;
            ParseError = string.Empty;
        }

        [RelayCommand]
        private void ParseQueryText()
        {
            try
            {
                if (string.IsNullOrWhiteSpace(QueryText))
                {
                    Filter.Groups.Clear();
                    ParseError = string.Empty;
                    return;
                }

                Filter.FromQueryText(QueryText);
                ParseError = string.Empty;
            }
            catch (Exception ex)
            {
                ParseError = $"Parse error: {ex.Message}";
            }
        }

        [RelayCommand]
        private void RefreshText()
        {
            SyncQueryFromFilter();
        }

        [RelayCommand]
        public void Apply()
        {
            FilterApplied?.Invoke();
        }

        private void SyncQueryFromFilter()
        {
            QueryText = Filter.ToQueryText();
            ParseError = string.Empty;
        }
    }
}
