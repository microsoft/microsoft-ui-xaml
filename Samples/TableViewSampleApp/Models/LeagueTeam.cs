// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Models;

/// <summary>
/// Group-stage standings row for a fictional Champions-League-style football
/// tournament: 8 groups (A–H) of 4 teams each, 6 games per team. Used by the
/// multi-column sort sample because the canonical sport-standings ordering
/// — group, then points desc, then goal difference desc as tiebreaker — is
/// the textbook scenario for multi-key sort.
///
/// Implements INotifyPropertyChanged so future sample additions (e.g.,
/// programmatic result updates) can mutate a row and have the table react.
/// </summary>
public sealed class LeagueTeam : INotifyPropertyChanged
{
    private string _group = string.Empty;
    private string _team = string.Empty;
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
                RaiseDerived();
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
                RaiseDerived();
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
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(GoalDifference)));
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
                PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(GoalDifference)));
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
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
        return true;
    }

    private void RaiseDerived()
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(nameof(Points)));
    }
}
