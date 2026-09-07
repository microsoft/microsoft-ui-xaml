// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// Hand-rolled deterministic data faker for the Person model. Hand-rolled
/// (not Bogus) so the gallery has zero NuGet dependencies and produces the
/// same names/values across runs (deterministic = screenshot-stable, easy
/// to reason about in interaction tests). Fixed seed = 8163 keeps the same
/// dataset every launch.
/// </summary>
public static class PersonData
{
    private const int Seed = 8163;

    private static readonly string[] s_firstNames =
    {
        "Aiden", "Alex", "Aria", "Asher", "Avery", "Bella", "Caleb", "Camila",
        "Carter", "Charlotte", "Chloe", "Daniel", "Eleanor", "Eli", "Elijah",
        "Ellie", "Emma", "Ethan", "Evelyn", "Ezra", "Felix", "Finn", "Gabriel",
        "Grace", "Grayson", "Hailey", "Hannah", "Harper", "Henry", "Hudson",
        "Ian", "Ibrahim", "Isaac", "Isabella", "Jack", "Jackson", "James",
        "Jasmine", "Jayden", "John", "Julia", "Kai", "Kayla", "Kennedy",
        "Lara", "Leah", "Levi", "Liam", "Lily", "Lincoln", "Logan", "Lucas",
        "Madison", "Mason", "Mateo", "Maya", "Mia", "Michael", "Mila", "Naomi",
        "Natalie", "Nathan", "Nicholas", "Noah", "Nora", "Olivia", "Oscar",
        "Owen", "Paige", "Penelope", "Quinn", "Rachel", "Riley", "Roman",
        "Ruby", "Ryan", "Samuel", "Sarah", "Scarlett", "Sebastian", "Sofia",
        "Sophia", "Stella", "Theo", "Theodore", "Thomas", "Tyler", "Victoria",
        "Violet", "William", "Willow", "Wyatt", "Xavier", "Yara", "Zara", "Zoe"
    };

    private static readonly string[] s_lastNames =
    {
        "Anderson", "Bennett", "Brooks", "Carter", "Chen", "Cooper", "Davis",
        "Edwards", "Foster", "Garcia", "Hall", "Hayes", "Hernandez", "Jackson",
        "James", "Johnson", "Jones", "Khan", "Kim", "King", "Lee", "Lewis",
        "Lopez", "Martinez", "Miller", "Mitchell", "Moore", "Morgan", "Murphy",
        "Nelson", "Nguyen", "Owen", "Parker", "Patel", "Perez", "Phillips",
        "Powell", "Price", "Reed", "Reyes", "Richardson", "Rivera", "Roberts",
        "Robinson", "Rodriguez", "Rogers", "Ross", "Russell", "Sanchez", "Scott",
        "Singh", "Smith", "Stewart", "Taylor", "Thomas", "Thompson", "Torres",
        "Walker", "Ward", "Watson", "White", "Williams", "Wilson", "Wood",
        "Wright", "Yamamoto", "Young", "Zhang"
    };

    private static readonly (string Department, string[] Roles)[] s_departments =
    {
        ("Engineering", new[] { "Software Engineer", "Senior Engineer", "Staff Engineer", "Engineering Manager", "QA Engineer" }),
        ("Design", new[] { "Designer", "Senior Designer", "Design Lead", "Researcher", "Content Designer" }),
        ("Product", new[] { "Product Manager", "Senior PM", "Group PM", "Program Manager", "Product Lead" }),
        ("Sales", new[] { "Sales Rep", "Account Executive", "Sales Manager", "Solutions Engineer", "Customer Success" }),
        ("Marketing", new[] { "Marketing Specialist", "Content Strategist", "Brand Manager", "Marketing Manager", "Demand Gen" }),
        ("Operations", new[] { "Operations Analyst", "Ops Manager", "Logistics Lead", "Vendor Manager", "Workplace" }),
        ("Finance", new[] { "Financial Analyst", "Accountant", "Finance Manager", "Controller", "Treasury" }),
        ("HR", new[] { "Recruiter", "HR Business Partner", "People Operations", "Talent Lead", "DEI Program Manager" })
    };

    private static readonly string[] s_bios =
    {
        "Keeps the weekly plan moving and is quick to unblock teammates.",
        "Enjoys turning ambiguous requests into a crisp plan. Often pairs with design and support to validate details before implementation.",
        "Owns a handful of customer-facing workflows and keeps a close eye on telemetry, accessibility feedback, and release notes. Volunteers for incident retrospectives because they like finding small process fixes that prevent repeats.",
        "Specializes in onboarding and documentation.",
        "Balances roadmap work with mentoring. Known for concise design reviews, careful handoffs, and a practical sense of when a prototype is ready to become product code.",
        "Works across time zones with partner teams, translating schedule risks into concrete choices. Their notes tend to include screenshots, test data, and the one decision still needed before launch.",
        "Focuses on quality-of-life improvements that make busy days easier.",
        "Leads complex customer pilots from first meeting through rollout. They collect real usage stories, compare them with product assumptions, and summarize the gaps in a way that helps engineering, sales, and operations act together.",
        "Keeps dashboards tidy and follow-up lists short.",
        "Builds trust by closing the loop. When a customer raises an issue, they track the owner, explain the workaround, and come back after the fix ships with a clear summary of what changed."
    };

    /// <summary>
    /// Public list of department names — used by the MixedControlsPage's
    /// in-cell ComboBox binding so the picker stays in sync with the
    /// data-generator's department vocabulary.
    /// </summary>
    public static IReadOnlyList<string> Departments { get; } =
        s_departments.Select(d => d.Department).ToArray();

    private static List<Person>? s_cache;

    /// <summary>
    /// Returns the cached canonical dataset (1000 deterministic entries).
    /// Pages then take a slice of any size from this list.
    /// </summary>
    public static IReadOnlyList<Person> All => s_cache ??= Generate(1000);

    /// <summary>
    /// Returns a fresh ObservableCollection of <paramref name="count"/> people
    /// taken from the cached dataset. Callers get their own collection so they
    /// can mutate it without affecting other pages.
    /// </summary>
    public static ObservableCollection<Person> Take(int count)
    {
        var list = new ObservableCollection<Person>();
        var source = All;
        for (int i = 0; i < count && i < source.Count; i++)
        {
            var p = source[i];
            list.Add(new Person
            {
                FirstName = p.FirstName,
                LastName = p.LastName,
                Email = p.Email,
                Department = p.Department,
                Role = p.Role,
                Bio = p.Bio,
                JoinDate = p.JoinDate,
                ShiftStart = p.ShiftStart,
                Salary = p.Salary,
                IsActive = p.IsActive,
            });
        }
        return list;
    }

    private static List<Person> Generate(int count)
    {
        var random = new Random(Seed);
        var list = new List<Person>(count);
        var today = DateTimeOffset.Now.Date;
        for (int i = 0; i < count; i++)
        {
            var first = s_firstNames[random.Next(s_firstNames.Length)];
            var last = s_lastNames[random.Next(s_lastNames.Length)];
            var dept = s_departments[random.Next(s_departments.Length)];
            var role = dept.Roles[random.Next(dept.Roles.Length)];

            // Deterministic email: lowercased and disambiguated by index so 1000
            // entries don't repeat (collisions among first/last names are common).
            var email = $"{first}.{last}.{i + 1:0000}@contoso.com".ToLowerInvariant();

            // Join date in the last ~10 years, biased slightly toward recent hires.
            var daysAgo = (int)Math.Round(Math.Pow(random.NextDouble(), 1.6) * 10 * 365);
            var joinDate = today.AddDays(-daysAgo);

            // Salary band loosely correlated with role seniority text.
            double baseSalary = role.Contains("Manager", StringComparison.Ordinal) || role.Contains("Lead", StringComparison.Ordinal) ? 165_000
                : role.Contains("Senior", StringComparison.Ordinal) || role.Contains("Staff", StringComparison.Ordinal) ? 195_000
                : role.Contains("Group", StringComparison.Ordinal) || role.Contains("Controller", StringComparison.Ordinal) ? 220_000
                : 115_000;
            // Round to the nearest hundred. Math.Round(value, -2) is invalid
            // (digits arg must be 0..15) so do the divide/multiply trick.
            double salary = Math.Round((baseSalary + (random.NextDouble() - 0.5) * 30_000) / 100.0) * 100.0;

            list.Add(new Person
            {
                FirstName = first,
                LastName = last,
                Email = email,
                Department = dept.Department,
                Role = role,
                Bio = s_bios[(i + random.Next(s_bios.Length)) % s_bios.Length],
                JoinDate = new DateTimeOffset(joinDate, TimeSpan.Zero),
                ShiftStart = new TimeSpan(7 + random.Next(0, 4), random.Next(0, 4) * 15, 0),
                Salary = salary,
                IsActive = random.NextDouble() > 0.07,
            });
        }
        return list;
    }
}
