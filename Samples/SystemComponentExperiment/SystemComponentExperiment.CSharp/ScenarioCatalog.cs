namespace SystemComponentExperiment.CSharp;

public static class ScenarioCatalog
{
    public static IReadOnlyList<ScenarioDefinition> All { get; } =
    [
        new(
            "environment.startup",
            "Environment and module inventory",
            "Runtime",
            "Windows 10 1809",
            typeof(StartupScenarioPage)),
        new(
            "composition.basics",
            "Compositor and visual creation",
            "Composition",
            "Windows 10 1809",
            typeof(CompositionScenarioPage)),
        new(
            "dispatching.basics",
            "DispatcherQueue enqueue",
            "Dispatching",
            "Windows 10 1809",
            typeof(DispatcherQueueScenarioPage))
    ];
}
