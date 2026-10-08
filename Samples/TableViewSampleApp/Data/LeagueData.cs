// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using TableViewSampleApp.Helpers;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// Hand-built deterministic standings for a fictional Champions-League-style
/// football tournament: 8 groups (A–H) × 4 teams = 32 entries, each team
/// having played 6 round-robin group games (home + away vs. each of the
/// other 3 teams in their group).
///
/// Designed for the Sorting page (grouped + single-column sort): the dataset
/// deliberately contains within-group point ties, so grouping by Group and
/// sorting by Points leaves Goal Difference as the visible tiebreaker.
///
/// Stats are internally consistent per group: ΣW = ΣL across the 4 teams,
/// ΣD is even, total team-games = 4×6 = 24 = 2×12 group games, and ΣGF =
/// ΣGA so goal difference nets to zero within each group.
///
/// Source order is by group, starting with Group F and alphabetical by team
/// within each group, so the default view is visibly unsorted by Points and
/// Goal Difference: the user gets a clear before/after when they click a header.
///
/// Draw and fixture data (Country, IsSeeded, NextMatchDate, Kickoff) are derived
/// from the row index, never from a random draw, so they are identical on every run.
/// Each group has exactly one seed and four different countries.
/// </summary>
public static class LeagueData
{
    /// <summary>Associations that appear in the Country chip.</summary>
    public static IReadOnlyList<string> Countries { get; } = new[]
    {
        "England", "Spain", "Italy", "Germany", "France", "Portugal", "Netherlands", "Belgium",
    };

    /// <summary>Festive-period fixture dates. Sorted as text "2 Jan" lands before "28 Dec";
    /// sorted by value it does not.</summary>
    private static readonly DateTimeOffset[] s_fixtureDates =
    {
        Fixture(2026, 12, 16), Fixture(2026, 12, 17), Fixture(2026, 12, 20),
        Fixture(2026, 12, 23), Fixture(2026, 12, 26), Fixture(2026, 12, 28),
        Fixture(2026, 12, 30), Fixture(2027, 1, 2), Fixture(2027, 1, 3),
    };

    /// <summary>Realistic kickoff times on a 24-hour clock.</summary>
    private static readonly TimeSpan[] s_kickoffs =
    {
        new(12, 30, 0), new(15, 0, 0), new(17, 30, 0), new(20, 0, 0), new(20, 45, 0), new(21, 0, 0),
    };

    /// <summary>Clubs that can join late (the Sorting page's "Add a late entrant" action),
    /// in the order they are handed out. Each starts with no games played.</summary>
    private static readonly string[] s_lateEntrants =
    {
        "Copper Herons", "Jade Lynxes", "Opal Badgers", "Bronze Kestrels", "Silver Ibis", "Umber Jackals",
    };

    /// <summary>Returns a fresh ObservableCollection so each consumer can
    /// mutate its own copy without disturbing shared state.</summary>
    public static ObservableCollection<LeagueTeam> All() =>
        new(s_rows.Select(Clone));

    /// <summary>Qualification band for a points total: Qualified (12+), Playoff (7–11) or
    /// Eliminated. Never returns the empty string, so it is also safe as a GroupBy key.</summary>
    public static string Band(object? value) => value switch
    {
        int points when points >= 12 => "Qualified",
        int points when points >= 7 => "Playoff",
        int => "Eliminated",
        _ => SampleShaping.NoneKey,
    };

    /// <summary>GroupBy key for a standings row and a group-key Tag (Group, Standing, Country,
    /// IsSeeded). Never blank: an empty group identity fails fast.</summary>
    public static object GroupKeyOf(LeagueTeam? team, string key)
    {
        if (team is null)
        {
            return SampleShaping.NoneKey;
        }

        var value = key switch
        {
            "Standing" => Band(team.Points),
            nameof(LeagueTeam.Country) => team.Country,
            nameof(LeagueTeam.IsSeeded) => team.IsSeeded ? "Seeded" : "Unseeded",
            _ => string.IsNullOrWhiteSpace(team.Group) ? string.Empty : string.Format(CultureInfo.CurrentCulture, "Group {0}", team.Group),
        };

        return string.IsNullOrWhiteSpace(value) ? SampleShaping.NoneKey : value;
    }

    /// <summary>A realistic late entrant for <paramref name="group"/>: no games played yet, so
    /// adding it keeps every group invariant. <paramref name="index"/> cycles through the list.</summary>
    public static LeagueTeam LateEntrant(int index, string group)
    {
        var i = s_rows.Count + index;
        var team = new LeagueTeam
        {
            Group = group,
            Team = s_lateEntrants[index % s_lateEntrants.Length],
            Country = Countries[(i * 3) % Countries.Count],
            NextMatchDate = s_fixtureDates[(i * 5) % s_fixtureDates.Length],
            Kickoff = s_kickoffs[(i * 5) % s_kickoffs.Length],
        };

        return team;
    }

    private static LeagueTeam Clone(LeagueTeam src) => new()
    {
        Group         = src.Group,
        Team          = src.Team,
        Country       = src.Country,
        IsSeeded      = src.IsSeeded,
        NextMatchDate = src.NextMatchDate,
        Kickoff       = src.Kickoff,
        Played        = src.Played,
        Wins          = src.Wins,
        Draws         = src.Draws,
        Losses        = src.Losses,
        GoalsFor      = src.GoalsFor,
        GoalsAgainst  = src.GoalsAgainst,
    };

    private static readonly IReadOnlyList<LeagueTeam> s_rows = WithFixtures(new[]
    {
        // Source order = by group (F first), alphabetical by Team within a group, so the
        // default view is visibly unsorted by Points/GD. Within each group:
        //   ΣW = ΣL  •  ΣD even  •  ΣGF = ΣGA (goal difference nets to 0)

        // Group F  ── 3-way tie at 11 pts; only GoalDifference separates them
        Row("F", "Cyan Mustangs",     0, 0, 6,  1,  7),  //  0 pts, -6 GD
        Row("F", "Lemon Raptors",     3, 2, 1,  6,  6),  // 11 pts,  0 GD
        Row("F", "Scarlet Owls",      3, 2, 1,  8,  4),  // 11 pts, +4 GD
        Row("F", "Slate Cobras",      3, 2, 1,  7,  5),  // 11 pts, +2 GD

        // Group A  ── dominant leader + collapse at the bottom
        Row("A", "Azure Lions",       3, 1, 2,  8,  6),  // 10 pts, +2
        Row("A", "Crimson Hawks",     5, 1, 0, 14,  3),  // 16 pts, +11
        Row("A", "Golden Wolves",     2, 2, 2,  7,  7),  //  8 pts,  0
        Row("A", "Silver Bears",      0, 0, 6,  2, 15),  //  0 pts, -13

        // Group B  ── two teams tied at 13 pts, GD breaks them
        Row("B", "Bronze Sharks",     1, 2, 3,  5,  8),  //  5 pts, -3
        Row("B", "Emerald Falcons",   4, 1, 1, 11,  5),  // 13 pts, +6
        Row("B", "Ivory Stallions",   0, 2, 4,  3,  9),  //  2 pts, -6
        Row("B", "Violet Tigers",     4, 1, 1, 10,  7),  // 13 pts, +3

        // Group C  ── clean 1-2-3-4 separation
        Row("C", "Cobalt Pumas",      2, 1, 3,  6,  9),  //  7 pts, -3
        Row("C", "Onyx Vipers",       3, 0, 3,  9,  9),  //  9 pts,  0
        Row("C", "Pearl Stags",       1, 1, 4,  4,  9),  //  4 pts, -5
        Row("C", "Ruby Eagles",       5, 0, 1, 12,  4),  // 15 pts, +8

        // Group D  ── runaway perfect-ish leader
        Row("D", "Amber Foxes",       3, 1, 2,  8,  6),  // 10 pts, +2
        Row("D", "Coral Otters",      0, 1, 5,  4, 15),  //  1 pt, -11
        Row("D", "Maroon Bulls",      2, 1, 3,  5,  8),  //  7 pts, -3
        Row("D", "Sable Panthers",    5, 1, 0, 15,  3),  // 16 pts, +12

        // Group E  ── close race, no ties
        Row("E", "Indigo Knights",    4, 1, 1, 10,  5),  // 13 pts, +5
        Row("E", "Magenta Cougars",   2, 2, 2,  6,  8),  //  8 pts, -2
        Row("E", "Saffron Pirates",   3, 2, 1,  9,  6),  // 11 pts, +3
        Row("E", "Teal Marlins",      0, 1, 5,  2,  8),  //  1 pt, -6

        // Group G  ── even spread top to bottom
        Row("G", "Lilac Stingrays",   1, 2, 3,  4,  8),  //  5 pts, -4
        Row("G", "Mint Rhinos",       3, 1, 2,  8,  7),  // 10 pts, +1
        Row("G", "Plum Ravens",       4, 0, 2, 11,  6),  // 12 pts, +5
        Row("G", "Russet Wolves",     2, 1, 3,  5,  7),  //  7 pts, -2

        // Group H  ── another clear pyramid + GD outlier at the bottom
        Row("H", "Aqua Tortoises",    1, 2, 3,  4,  6),  //  5 pts, -2
        Row("H", "Garnet Bison",      3, 0, 3,  7,  6),  //  9 pts, +1
        Row("H", "Mauve Penguins",    1, 1, 4,  2, 11),  //  4 pts, -9
        Row("H", "Quartz Dragons",    5, 1, 0, 13,  3),  // 16 pts, +10
    });

    private static LeagueTeam Row(string group, string team,
                                   int w, int d, int l,
                                   int gf, int ga) => new()
    {
        Group        = group,
        Team         = team,
        Played       = w + d + l,
        Wins         = w,
        Draws        = d,
        Losses       = l,
        GoalsFor     = gf,
        GoalsAgainst = ga,
    };

    // Index-derived draw and fixture data. Within each block of four rows (one group) the
    // country indexes (3i mod 8) are all different, and the seed position (i/4 mod 4) moves
    // around so the seed is not always the first or the strongest team.
    private static IReadOnlyList<LeagueTeam> WithFixtures(LeagueTeam[] rows)
    {
        for (var i = 0; i < rows.Length; i++)
        {
            rows[i].Country = Countries[(i * 3) % Countries.Count];
            rows[i].IsSeeded = i % 4 == (i / 4) % 4;
            rows[i].NextMatchDate = s_fixtureDates[(i * 5) % s_fixtureDates.Length];
            rows[i].Kickoff = s_kickoffs[(i * 5) % s_kickoffs.Length];
        }

        return rows;
    }

    // Local noon, so the calendar day is the same in every time zone the picker converts to.
    private static DateTimeOffset Fixture(int year, int month, int day)
    {
        var local = new DateTime(year, month, day, 12, 0, 0, DateTimeKind.Unspecified);
        return new DateTimeOffset(local, TimeZoneInfo.Local.GetUtcOffset(local));
    }
}
