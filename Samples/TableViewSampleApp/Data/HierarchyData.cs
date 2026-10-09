using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;

namespace TableViewSampleApp.Data;

// Flat row model: no Level/IsExpanded/children. The tree comes from key selectors (Id plus
// ManagerId or MentorId), so one list can show two relations. Observable for live shaping.
public sealed class Employee : INotifyPropertyChanged
{
    private int? _managerId;
    private int? _mentorId;
    private string _name = string.Empty;
    private string _title = string.Empty;
    private string _dept = string.Empty;

    public int Id { get; set; }
    public int? ManagerId { get => _managerId; set => Set(ref _managerId, value); }
    public int? MentorId { get => _mentorId; set => Set(ref _mentorId, value); }
    public string Name { get => _name; set => Set(ref _name, value); }
    public string Title { get => _title; set => Set(ref _title, value); }
    public string Dept { get => _dept; set => Set(ref _dept, value); }

    public event PropertyChangedEventHandler? PropertyChanged;

    private void Set<T>(ref T field, T value, [CallerMemberName] string? name = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value))
        {
            return;
        }

        field = value;
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
    }
}

internal static class HierarchyData
{
    public const int Branching = 3;

    internal static readonly string[] Depts = { "Engineering", "Design", "Sales", "Support", "Finance", "Legal", "Research", "Ops" };
    private static readonly string[] Titles = { "Director", "Manager", "Lead", "Senior", "Engineer", "Associate" };
    private static readonly string[] First = { "Ada", "Ben", "Cy", "Dan", "Eve", "Fay", "Gus", "Hal", "Ivy", "Jo" };

    // Manager relation: 5 roots, `Branching` reports each. Mentor relation: 8 roots, wider (~2.5
    // mentees each), so the two views differ.
    public static List<Employee> Generate(int count)
    {
        const int managerRoots = 5;
        const int mentorRoots = 8;
        var list = new List<Employee>(count);
        for (int i = 0; i < count; i++)
        {
            int depth = i < managerRoots ? 0 : 1 + (int)Math.Log(1 + (i - managerRoots) / (double)Branching, Branching);
            list.Add(new Employee
            {
                Id = i + 1,
                ManagerId = i < managerRoots ? null : (i - managerRoots) / Branching + 1,
                MentorId = i < mentorRoots ? null : (i - mentorRoots) * 2 / 5 + 1,
                Name = $"{First[i % First.Length]} {(char)('A' + i / First.Length % 26)}{i + 1}",
                Title = Titles[Math.Min(depth, Titles.Length - 1)],
                Dept = Depts[i % Depts.Length],
            });
        }

        return list;
    }
}
