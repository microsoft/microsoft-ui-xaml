using Microsoft.UI.Xaml.Controls;

namespace SystemComponentExperiment.CSharp;

public sealed record ScenarioDefinition(
    string Id,
    string Title,
    string Component,
    string MinimumOs,
    Type PageType);
