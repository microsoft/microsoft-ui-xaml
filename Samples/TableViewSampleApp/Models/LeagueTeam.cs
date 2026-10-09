// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.ComponentModel;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Models;

/// <summary>
/// Group-stage standings row for a fictional Champions-League-style football tournament:
/// 8 groups (A–H) of 4 teams each, 6 games per team. Used by the Sorting page, where the
/// textbook standings order (group, then points, then goal difference) is reached with
/// grouped shaping plus a single-column sort.
///
/// Besides the results it carries draw and fixture data of the types the gallery's template
/// columns edit: a <see cref="Country"/> chip, a <see cref="IsSeeded"/> CheckBox, the
/// <see cref="NextMatchDate"/> date picker and the <see cref="Kickoff"/> time picker. Every
/// property raises PropertyChanged, so an edit re-renders and the page can re-sort.
/// </summary>
public sealed class LeagueTeam : INotifyPropertyChanged
{
    private string _group = string.Empty;
    private string _team = string.Empty;
    private string _country = string.Empty;
    private bool _isSeeded;
    private DateTimeOffset _nextMatchDate;
    private TimeSpan _kickoff;
    private int _played;
    private int _wins;
    private int _draws;
    private int _losses;
    private int _goalsFor;
    private int _goalsAgainst;

    public string Group
    {
        get => _group;
        set => Set(ref _group, value);
    }

    public string Team
    {
        get => _team;
        set => Set(ref _team, value);
    }

    /// <summary>Football association the club belongs to (England, Spain, …).</summary>
    public string Country
    {
        get => _country;
        set => Set(ref _country, value);
    }

    /// <summary>Pot-1 seed in the draw. Independent of results, so editing it cannot
    /// contradict Points or Standing.</summary>
    public bool IsSeeded
    {
        get => _isSeeded;
        set => Set(ref _isSeeded, value);
    }

    /// <summary>Date of the next domestic fixture. Sorting uses this value, so "2 Jan" orders
    /// after "28 Dec" even though it sorts before it as text.</summary>
    public DateTimeOffset NextMatchDate
    {
        get => _nextMatchDate;
        set
        {
            if (Set(ref _nextMatchDate, value))
            {
                Raise(nameof(NextMatchDateOrNull));
                Raise(nameof(NextMatchText));
            }
        }
    }

    /// <summary>CalendarDatePicker shim: its Date is nullable. Clearing the picker keeps the
    /// current date rather than storing an empty fixture.</summary>
    public DateTimeOffset? NextMatchDateOrNull
    {
        get => _nextMatchDate;
        set
        {
            if (value is { } date)
            {
                NextMatchDate = date;
            }
        }
    }

    /// <summary>The fixture date as display text ("2 Jan"). The text-date column sorts on this
    /// string, which is the alphabetical-versus-chronological contrast.</summary>
    public string NextMatchText => _nextMatchDate.ToString("d MMM", CultureInfo.CurrentCulture);

    /// <summary>Local kickoff time of the next fixture, 24-hour clock.</summary>
    public TimeSpan Kickoff
    {
        get => _kickoff;
        set => Set(ref _kickoff, value);
    }

    public int Played
    {
        get => _played;
        set => Set(ref _played, value);
    }

    public int Wins
    {
        get => _wins;
        set
        {
            if (Set(ref _wins, value))
            {
                Raise(nameof(Points));
            }
        }
    }

    public int Draws
    {
        get => _draws;
        set
        {
            if (Set(ref _draws, value))
            {
                Raise(nameof(Points));
            }
        }
    }

    public int Losses
    {
        get => _losses;
        set => Set(ref _losses, value);
    }

    public int GoalsFor
    {
        get => _goalsFor;
        set
        {
            if (Set(ref _goalsFor, value))
            {
                Raise(nameof(GoalDifference));
            }
        }
    }

    public int GoalsAgainst
    {
        get => _goalsAgainst;
        set
        {
            if (Set(ref _goalsAgainst, value))
            {
                Raise(nameof(GoalDifference));
            }
        }
    }

    /// <summary>Goal difference: <c>GoalsFor − GoalsAgainst</c>. Common
    /// first-tier tiebreaker after points in football tournaments.</summary>
    public int GoalDifference => _goalsFor - _goalsAgainst;

    /// <summary>Standard football points: <c>3·Wins + 1·Draws</c>.</summary>
    public int Points => (_wins * 3) + _draws;

    public event PropertyChangedEventHandler? PropertyChanged;

    private bool Set<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (Equals(field, value))
        {
            return false;
        }

        field = value;
        Raise(propertyName);
        return true;
    }

    private void Raise(string? propertyName) =>
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
}
