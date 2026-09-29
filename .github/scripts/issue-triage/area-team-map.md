# Area-to-team routing evidence

This evidence supports normal pull-request review of `area-team-map.json`.
The initial mapping is inferred only from public issue labels as of
September 29, 2026. Historical co-labeling is evidence, not a declaration of
current ownership. Once merged, the workflow consumes the checked-in mapping
without any additional approval or activation step.

## Method

For each existing `area-*` label, sample up to thirty most recently created issues
(open or closed) using this GitHub search:

```text
repo:microsoft/microsoft-ui-xaml is:issue label:"<area>" sort:created-desc
```

Count only issues with exactly one area and one team. When a specific
`team-Markup`, `team-Reach`, or `team-Rendering` accompanies its parent `team-Core`,
count the specific team, not both. Discard other multiple-team cases.

Propose a mapping only when the leading team has at least three supporting
issues and at least 80% of the qualified observations. Otherwise leave the JSON
value `null`: no automatic team assignment. This yields 65 proposals and 58
unmapped areas. All 123 existing area spellings are preserved, including
`area-SystemBackdropEement`.

The counts below are from this bounded sample, not the repository's complete
history or a statistical confidence estimate. Examples are public issues
supporting the leading observed team, even for rows intentionally left unmapped.
Sparse, old, or conflicting history needs an explicit maintainer decision.

## Review table

| Area | Proposed team | Qualified observations | Leading-team examples |
| --- | --- | --- | --- |
| `area-AOT` | Unmapped | Markup 5; Core 2; Controls 1; Reach 1 | #9980, #9955, #9876 |
| `area-AccessKeys` | Unmapped | Rendering 4; Controls 3 | #8648, #8447, #6978 |
| `area-Accessibility` | `team-Controls` | Controls 9; CompInput 1 | #11226, #8720, #8296 |
| `area-AnimatedIcon` | `team-Controls` | Controls 13; CompInput 1 | #10243, #9962, #7672 |
| `area-AnimatedVisualPlayer` | Unmapped | Controls 5; CompInput 4 | #7158, #5965, #2785 |
| `area-Animations` | Unmapped | Controls 11; Core 5; CompInput 3; Rendering 1 | #9482, #9131, #8879 |
| `area-App activation` | Unmapped | Reach 1 | #7343 |
| `area-AppWindow` | Unmapped | CompInput 5; Core 5 | #11941, #10849, #9982 |
| `area-Application` | Unmapped | Reach 5; Markup 3; Controls 1; Core 1 | #10513, #9843, #9838 |
| `area-AutoSuggestBox` | `team-Controls` | Controls 21 | #11750, #10590, #9560 |
| `area-Binding` | Unmapped | Markup 8; Core 4; Controls 1 | #11794, #11533, #10078 |
| `area-Breadcrumb` | `team-Controls` | Controls 13 | #7283, #7213, #7075 |
| `area-Button` | `team-Controls` | Controls 9; Reach 1 | #9083, #8870, #7358 |
| `area-C#/WinRT` | Unmapped | Markup 12; Reach 2; Controls 1; Core 1 | #10250, #9763, #6313 |
| `area-C++/WinRT` | Unmapped | Controls 3; Core 3; Markup 2 | #8748, #5041, #4847 |
| `area-ColorPicker` | `team-Controls` | Controls 21; Rendering 1 | #11243, #10798, #8883 |
| `area-ComboBox` | `team-Controls` | Controls 17 | #11266, #10633, #10023 |
| `area-CommandBarFlyout` | `team-Controls` | Controls 18 | #11498, #10170, #9834 |
| `area-Commanding` | `team-Controls` | Controls 17; Markup 1 | #9618, #9403, #9379 |
| `area-CoreFramework` | Unmapped | Controls 8; Rendering 5; Markup 4; Reach 2; CompInput 1; Core 1 | #9667, #9147, #8342 |
| `area-DataGrid` | `team-Controls` | Controls 7 | #6408, #4616, #4522 |
| `area-DateTimePickers` | `team-Controls` | Controls 20 | #11236, #10673, #10671 |
| `area-Density` | `team-Controls` | Controls 6 | #6464, #6458, #4841 |
| `area-Design` | Unmapped | No unambiguous observations | - |
| `area-DesignDiscussion` | Unmapped | Controls 2 | #2517, #952 |
| `area-DesignToolkit` | `team-Design` | Design 27 | #8877, #8876, #8762 |
| `area-Designer` | Unmapped | Markup 2 | #8493, #3304 |
| `area-DevInternal` | `team-Controls` | Controls 17; Markup 3 | #6649, #6487, #6190 |
| `area-Dialogs` | `team-Controls` | Controls 17; Reach 1; Rendering 1 | #9785, #9712, #9343 |
| `area-DragAndDrop` | Unmapped | Controls 4; CompInput 2; Markup 2; Rendering 2 | #9717, #9520, #8859 |
| `area-EffectiveViewport` | Unmapped | Controls 1; Rendering 1 | #6423 |
| `area-ErrorHandling` | Unmapped | Markup 8; Controls 2; Core 2; CompInput 1; Reach 1 | #9522, #8774, #8310 |
| `area-Expander` | `team-Controls` | Controls 16 | #11661, #9706, #7738 |
| `area-External` | Unmapped | Controls 3; Markup 1 | #11195, #9558, #9458 |
| `area-Flyouts` | `team-Controls` | Controls 11; CompInput 1 | #9504, #8930, #8921 |
| `area-FocusManager` | `team-Reach` | Reach 13; Controls 2; Rendering 1 | #9683, #9032, #8087 |
| `area-HotReload` | `team-Markup` | Markup 5; Reach 1 | #9887, #9212, #8704 |
| `area-Hyperlink` | Unmapped | Controls 3; Rendering 1 | #10779, #6616, #1176 |
| `area-HyperlinkButton` | `team-Controls` | Controls 8 | #9595, #9449, #7808 |
| `area-Icon` | Unmapped | Controls 12; Design 2; Rendering 2 | #10373, #9467, #8693 |
| `area-ImageIcon` | Unmapped | Controls 2 | #10081, #6633 |
| `area-Images` | Unmapped | Rendering 12; Controls 8 | #10760, #10080, #9981 |
| `area-InfoBadge` | `team-Controls` | Controls 4 | #8176, #8006, #7001 |
| `area-InfoBar` | `team-Controls` | Controls 18; Markup 1 | #10733, #7620, #7611 |
| `area-Infrastructure` | Unmapped | Core 1 | #11834 |
| `area-InkCanvas` | `team-Controls` | Controls 5 | #10461, #9525, #6742 |
| `area-InkToolBar` | Unmapped | Controls 3; CompInput 1; Rendering 1 | #5844, #5468, #1530 |
| `area-InputValidation` | `team-Controls` | Controls 4; Markup 1 | #5859, #4671, #4640 |
| `area-Islands` | Unmapped | Reach 7; Core 4; Rendering 3; Markup 2 | #10050, #9334, #9130 |
| `area-ItemsRepeater` | `team-Controls` | Controls 17; Reach 1 | #11727, #11570, #10488 |
| `area-ItemsView` | `team-Controls` | Controls 7 | #10620, #10314, #9952 |
| `area-KeyboardAccelerators` | Unmapped | Reach 10; Controls 5; Rendering 5; CompInput 1 | #8931, #6672, #6488 |
| `area-Layouts` | Unmapped | Controls 10; Rendering 4 | #10200, #9827, #9617 |
| `area-Lifetime` | Unmapped | Reach 4; Rendering 2; Controls 1; Core 1 | #9534, #9063, #8194 |
| `area-Lists` | `team-Controls` | Controls 12 | #11800, #11351, #11254 |
| `area-LiveVisualTree` | Unmapped | Core 1; Markup 1; Reach 1 | #9926 |
| `area-Localization` | Unmapped | Markup 2 | #4891, #1337 |
| `area-MRT` | Unmapped | Markup 3; Core 1 | #5940, #3423, #1696 |
| `area-MapControl` | `team-Controls` | Controls 8 | #9490, #9488, #9487 |
| `area-Materials` | Unmapped | Rendering 10; Controls 9; CompInput 2; Core 1 | #10180, #9764, #9286 |
| `area-MediaElement` | `team-Rendering` | Rendering 6; Reach 1 | #8090, #8055, #8047 |
| `area-MediaPlayerElement` | Unmapped | Rendering 13; Controls 7 | #9756, #9586, #9451 |
| `area-Menus` | `team-Controls` | Controls 21; Rendering 1 | #11886, #11098, #10886 |
| `area-Migration` | Unmapped | Controls 1 | #410 |
| `area-Mouse` | Unmapped | CompInput 3; Controls 1; Rendering 1 | #8738, #5505, #1959 |
| `area-Navigation` | `team-Controls` | Controls 15; Reach 2; Rendering 1 | #11915, #9975, #9632 |
| `area-NavigationView` | `team-Controls` | Controls 21 | #11927, #11740, #11505 |
| `area-NugetPackage` | Unmapped | Controls 10; Markup 7; CompInput 1; Reach 1 | #7346, #6656, #6481 |
| `area-NumberBox` | `team-Controls` | Controls 20; Markup 1 | #10182, #9826, #9772 |
| `area-Pager` | `team-Controls` | Controls 5 | #6495, #4085, #4083 |
| `area-ParallaxView` | Unmapped | Controls 1 | #3070 |
| `area-Parser` | `team-Markup` | Markup 15; Core 1 | #7828, #7017, #6546 |
| `area-PasswordBox` | `team-Controls` | Controls 4 | #9615, #8946, #6431 |
| `area-Performance` | Unmapped | CompInput 5; Controls 4; Rendering 3; Markup 2; Reach 1 | #11144, #11048, #9703 |
| `area-PersonPicture` | `team-Controls` | Controls 5 | #11922, #6654, #6386 |
| `area-PipsPager` | `team-Controls` | Controls 19 | #11495, #11467, #11469 |
| `area-Pivot` | `team-Controls` | Controls 13 | #9664, #8968, #7718 |
| `area-Pointer` | Unmapped | CompInput 12; Rendering 4; Controls 2 | #11676, #10929, #10474 |
| `area-Popup` | Unmapped | Controls 7; CompInput 2; Rendering 2; Core 1 | #9555, #9433, #8838 |
| `area-Progress` | `team-Controls` | Controls 22 | #11705, #9872, #9010 |
| `area-ProjectSystem` | `team-Markup` | Markup 25; Reach 2 | #9974, #9959, #9937 |
| `area-PullToRefresh` | `team-Controls` | Controls 9; CompInput 1 | #11531, #9900, #9604 |
| `area-RadialGradientBrush` | `team-Controls` | Controls 6 | #7967, #3883, #3151 |
| `area-RadioButtons` | `team-Controls` | Controls 23 | #9917, #9811, #9297 |
| `area-RatingControl` | `team-Controls` | Controls 3 | #3925, #3072, #370 |
| `area-RepeatButton` | Unmapped | Controls 2 | #6609, #5979 |
| `area-Resources` | Unmapped | Markup 8; Controls 6; Core 1; Reach 1 | #9759, #8967, #8857 |
| `area-ScaleFactor` | `team-Rendering` | Rendering 3 | #9840, #7787, #7293 |
| `area-ScrollBar` | `team-Controls` | Controls 11; CompInput 1 | #10089, #9854, #9039 |
| `area-Scrolling` | Unmapped | Controls 6; CompInput 1; Core 1 | #11650, #10206, #9979 |
| `area-SelectionModel` | `team-Controls` | Controls 3 | #9589, #6086, #2404 |
| `area-SemanticZoom` | Unmapped | Controls 2 | #9239, #4262 |
| `area-Shadows` | Unmapped | Rendering 14; Controls 3; CompInput 1; Design 1 | #8080, #6248, #6206 |
| `area-Shapes` | `team-Rendering` | Rendering 15; Controls 1 | #10614, #9338, #8169 |
| `area-Slider` | `team-Controls` | Controls 10 | #7737, #6721, #6003 |
| `area-SplitButton` | `team-Controls` | Controls 13 | #9947, #9656, #9412 |
| `area-SplitView` | `team-Controls` | Controls 3 | #9844, #9141, #5776 |
| `area-Styling` | `team-Controls` | Controls 11; Markup 2 | #10372, #9989, #9662 |
| `area-SwipeControl` | `team-Controls` | Controls 22 | #8945, #8855, #8250 |
| `area-SystemBackdropEement` | Unmapped | No unambiguous observations | - |
| `area-TabView` | `team-Controls` | Controls 17 | #10749, #10667, #10207 |
| `area-TeachingTip` | `team-Controls` | Controls 24 | #9881, #9777, #9601 |
| `area-Templates` | Unmapped | Controls 2; Markup 1; Reach 1 | #2898, #1049 |
| `area-TestInfrastructure` | `team-Controls` | Controls 21; Markup 3; Core 1 | #7668, #6175, #6174 |
| `area-TextBlocks` | Unmapped | Controls 8; Rendering 6; Markup 2 | #9925, #9528, #9469 |
| `area-TextBox` | `team-Controls` | Controls 9 | #11856, #10994, #10522 |
| `area-TitleBar` | Unmapped | Controls 4; Core 2 | #11521, #11141, #10735 |
| `area-ToggleSwitch` | `team-Controls` | Controls 15 | #10430, #9830, #9326 |
| `area-ToolTip` | `team-Controls` | Controls 14; Rendering 2; Markup 1 | #11500, #9264, #9222 |
| `area-Tooling` | Unmapped | Markup 12; Core 5; CompInput 1; Controls 1; Reach 1 | #9571, #9564, #8529 |
| `area-Transitions` | Unmapped | Controls 1; Rendering 1 | #482 |
| `area-TreeView` | `team-Controls` | Controls 22 | #11518, #10747, #10309 |
| `area-TwoPaneView` | `team-Controls` | Controls 15 | #6578, #6357, #5997 |
| `area-UIDesign` | Unmapped | Controls 4; Design 2 | #9624, #6602, #6469 |
| `area-Unpackaged` | `team-Markup` | Markup 17; Core 1 | #10173, #9234, #9091 |
| `area-VSM` | Unmapped | Controls 4; Markup 3 | #8566, #6710, #1460 |
| `area-WebView` | `team-Controls` | Controls 17; Rendering 2 | #10305, #10219, #9941 |
| `area-Windowing` | `team-Core` | Core 7; CompInput 1 | #11965, #11963, #11202 |
| `area-WindowsDesign` | Unmapped | CompInput 1; Reach 1 | #7583 |
| `area-XYFocus` | Unmapped | Reach 2 | #3179, #2521 |
| `area-XamlCompiler` | Unmapped | Markup 8; Core 6 | #11825, #11812, #11808 |
| `area-XamlWindow` | Unmapped | Reach 8; Core 4; Markup 2; Rendering 2; CompInput 1 | #8816, #7564, #6421 |
| `area-wapproj` | Unmapped | Markup 1 | #8763 |
