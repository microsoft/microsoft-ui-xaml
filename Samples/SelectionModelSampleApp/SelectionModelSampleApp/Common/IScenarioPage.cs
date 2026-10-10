// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
namespace SelectionModelSampleApp.Common
{
    /// <summary>
    /// Implemented by sample pages that can put themselves into a named, deterministic state.
    /// The app passes a "&lt;PageTag&gt;:&lt;scenario&gt;" command line argument, which lets the
    /// documentation screenshots be captured reproducibly instead of by driving the UI by hand.
    /// </summary>
    public interface IScenarioPage
    {
        void ApplyScenario(string scenario);
    }
}
