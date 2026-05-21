# Direct Manipulation Integration

This stub tracks missing documentation around Xaml's integration with DirectManipulation (DManip).

## Table of Contents

- [Cross-slide Viewports](#cross-slide-viewports)
- [Wish list](#wish-list)

## Cross-slide Viewports

Cross-slide viewports are used for scenarios like swipe to select, where we don't send input to DManip until we can disambiguate the input. In general, once we send input to DManip we're not getting it back. In these scenarios Xaml still needs the input to figure out whether it's a slide to select or whether it's an actual pan/zoom, at which point we send the input to DManip.

Cross-slide viewports are possibly also involved in initiating a drag.

## Wish list
* Overview - integration with ScrollViewer, two-step registration, when viewports are created
* Overview of DManip API - IDirectManipulationManager, IDirectManipulationContent, etc
* Xaml Class details - CDirectManipulationService, CInputServices, CUIDMContainer, etc
* Cross-slide viewport details
