// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// Hand-built deterministic standings for a fictional Champions-League-style
/// football tournament: 8 groups (A–H) × 4 teams = 32 entries, each team
/// having played 6 round-robin group games (home + away vs. each of the
/// other 3 teams in their group).
///
/// Designed for the multi-column sort sample: the dataset deliberately
/// contains within-group point ties so Goal Difference becomes a meaningful
/// tiebreaker, demonstrating why a 3-level sort (Group asc → Points desc →
/// GoalDifference desc) — the textbook UEFA ordering — is more useful than
/// any single-column sort.
///
/// Stats are internally consistent per group: ΣW = ΣL across the 4 teams,
/// ΣD is even, total team-games = 4×6 = 24 = 2×12 group games, and ΣGF =
/// ΣGA so goal difference nets to zero within each group.
///
/// The list is ordered alphabetically by team name in source so the table
/// renders in a "shuffled" state — the user gets a clear before/after when
/// they click a header.
/// </summary>
public static class LeagueData
{
    /// <summary>Returns a fresh ObservableCollection so each consumer can
    /// mutate its own copy (e.g. reorder via Move) without disturbing
    /// shared state.</summary>
    public static ObservableCollection<LeagueTeam> All() =>
        new(s_rows.Select(Clone));

    private static LeagueTeam Clone(LeagueTeam src) => new()
    {
        Group        = src.Group,
        Team         = src.Team,
        Played       = src.Played,
        Wins         = src.Wins,
        Draws        = src.Draws,
        Losses       = src.Losses,
        GoalsFor     = src.GoalsFor,
        GoalsAgainst = src.GoalsAgainst,
    };

    private static readonly IReadOnlyList<LeagueTeam> s_rows = new[]
    {
        // Source order = alphabetical by Team so the default view is
        // visibly unsorted by Group/Points/GD. Within each group:
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
    };

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
}
