// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using TableViewSampleApp.Models;

namespace TableViewSampleApp.Data;

/// <summary>
/// People with Arabic and Hebrew names and notes for the Right-to-left page, kept
/// out of the page so it holds only the RTL behaviour. <see cref="Mixed"/> mixes the first <see cref="MixedInCount"/>
/// into <see cref="PersonData"/>'s rows; "Add a person" takes the rest in turn through <see cref="Create"/>.
/// </summary>
public static class RtlPersonData
{
    /// <summary>How many right-to-left people <see cref="Mixed"/> puts into the top rows.</summary>
    public const int MixedInCount = 5;

    /// <summary>The employee id of the first right-to-left person.</summary>
    public const int FirstEmployeeId = 1041;

    private static readonly (string First, string Last, string Department, string Role, string Office, string Notes)[] s_people =
    {
        ("ليلى", "حداد", "Design", "Design Lead", "Dublin", "تقود فريق التصميم وتراجع ملاحظات المستخدمين كل أسبوع."),
        ("נועה", "כהן", "Engineering", "QA Engineer", "London", "מובילה את צוות הבדיקות ואחראית על נגישות המוצר."),
        ("يوسف", "الخطيب", "Engineering", "Senior Engineer", "Singapore", "يعمل على تحسين أداء تطبيق Contoso على الأجهزة المحمولة."),
        ("אורי", "לוי", "Operations", "Ops Manager", "Toronto", "מתחזק את מערכת הניטור ומתאם את סבב הכוננויות."),
        ("نور", "منصور", "Marketing", "Brand Manager", "London", "تنسق مواعيد الإطلاق مع فرق التسويق والمبيعات."),
        ("תמר", "אברהם", "Product", "Product Manager", "Seattle", "מתאמת בין צוותי העיצוב והפיתוח לקראת כל גרסה."),
        ("عمر", "صالح", "Finance", "Financial Analyst", "Dublin", "يعد تقارير الميزانية الربعية ويتابع النفقات."),
        ("מיכל", "פרץ", "HR", "Recruiter", "Redmond", "אחראית על גיוס מהנדסים ועל תהליך הקליטה."),
    };

    /// <summary>First names the "Rename selected person" action cycles through.</summary>
    public static IReadOnlyList<string> Renames { get; } = ["سارة", "רוני", "كريم", "דניאל"];

    /// <summary>PersonData.Take(40) with the first five Arabic / Hebrew people mixed into the top rows.</summary>
    public static ObservableCollection<Person> Mixed()
    {
        var people = PersonData.Take(40);
        for (var i = 0; i < MixedInCount; i++)
        {
            people.Insert(i * 2, Create(i, FirstEmployeeId + i));
        }

        return people;
    }

    /// <summary>The <paramref name="index"/>-th right-to-left person (cycling), with <paramref name="employeeId"/>.</summary>
    public static Person Create(int index, int employeeId)
    {
        var seed = s_people[index % s_people.Length];
        return new Person
        {
            FirstName = seed.First,
            LastName = seed.Last,
            Email = string.Format(CultureInfo.CurrentCulture, "employee{0}@contoso.com", employeeId),
            Department = seed.Department,
            Role = seed.Role,
            Office = seed.Office,
            Bio = seed.Notes,
            EmployeeId = employeeId,
            IsActive = employeeId % 5 != 0,
            JoinDate = new DateTimeOffset(DateTimeOffset.Now.Date.AddDays(-97 * (employeeId % 13 + 1)), TimeSpan.Zero),
            ShiftStart = new TimeSpan(8 + employeeId % 3, 0, 0),
            Salary = 120_000 + 2_500 * (employeeId % 9),
        };
    }
}
