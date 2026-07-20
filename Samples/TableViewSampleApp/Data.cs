using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;

namespace TableViewSampleApp;

// Row model for the sample. Name lengths vary to exercise Auto column sizing; Bio strings
// vary in length. Notes/Joined are settable so the editable TextBox / DatePicker cells write
// back (TwoWay).
public sealed class Item
{
    public string Name { get; init; } = "";
    public string Role { get; init; } = "";
    public string City { get; init; } = "";
    public int Score { get; init; }
    public string Bio { get; init; } = "";
    public DateTimeOffset Joined { get; set; }
    public string Notes { get; set; } = "";
    public ImageSource? Avatar { get; init; }

    public Item(string name, string role, string city, int score, string bio, DateTimeOffset joined, string notes, ImageSource? avatar)
    {
        Name = name; Role = role; City = city; Score = score;
        Bio = bio; Joined = joined; Notes = notes; Avatar = avatar;
    }
}

internal static class Data
{
    // Name-width tiers. The wide names are ISOLATED (see Make) so the Auto Name column starts
    // narrow and visibly EXPANDS when a wide name scrolls into view (grow-only, matching v1).
    private static readonly string[] Short = { "Al", "Bo", "Cy", "Jo", "Ed", "Li", "Yu", "Sam", "Mia" };
    private static readonly string[] Medium = { "Ada Lovelace", "Grace Hopper", "Alan Turing", "Nina Patel", "Omar Farouk" };
    private static readonly string[] Wide =
    {
        "Maximilian Alexander Fairbanks-Whittington",
        "Anastasia Konstantinova Rozhdestvenskaya",
        "Wolfgang Amadeus von Habsburg-Lothringen",
    };
    private static readonly string[] Roles = { "Dev", "QA", "PM", "Designer", "Architect", "Researcher" };
    private static readonly string[] Cities = { "London", "Oslo", "Kyoto", "New York", "Shenzhen", "Berlin" };

    // Bio strings of varied length so the wrapping Bio column drives variable row height.
    private static readonly string[] Bios =
    {
        "Fixes bugs.",
        "Runs the manual and automated test passes ahead of every milestone release, triaging incoming regressions across all supported platforms and keeping the sign-off checklist current for each build.",
        "Coordinates sprint planning and delivery.",
        "Designs the visual language and reviews every control spec for accessibility and theming consistency across light, dark, and high-contrast.",
        "Investigates performance and memory footprint on low-end hardware.",
    };
    private static readonly string[] NotesSeed = { "Fix bugs", "Test pass", "Plan", "Review", "Profile" };

    // 3 dummy avatars (Assets\avatar1..3.png), cycled per row.
    private static ImageSource Avatar(int i)
        => new BitmapImage(new Uri($"ms-appx:///Assets/avatar{(i % 3) + 1}.png"));

    // ~150 rows so the body virtualizes and scrolls. A single wide name appears in isolation
    // every 50 rows (at 35, 85, 135), so the first screen has only short/medium names and the
    // Auto Name column starts narrow, then grows when you scroll a wide name into view.
    public static List<Item> Make(int n = 150)
    {
        var list = new List<Item>(n);
        var rnd = new Random(42);
        var baseDate = new DateTimeOffset(2024, 1, 1, 0, 0, 0, TimeSpan.Zero);
        for (int i = 0; i < n; i++)
        {
            string name =
                (i % 50 == 35) ? Wide[(i / 50) % Wide.Length] :
                (i % 3 == 0) ? Short[i % Short.Length] :
                               Medium[i % Medium.Length];
            list.Add(new Item(
                name,
                Roles[i % Roles.Length],
                Cities[i % Cities.Length],
                rnd.Next(0, 101),
                Bios[i % Bios.Length],
                baseDate.AddDays(rnd.Next(0, 500)),
                NotesSeed[i % NotesSeed.Length],
                Avatar(i)));
        }
        return list;
    }
}
