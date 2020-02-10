---
name: Feature Proposal
about: Suggest a new feature or idea
title: 'Proposal:'
labels: feature proposal
assignees: ''

---

<!-- This is a template for new feature or API proposals.
For example you can use this to propose a new API on an existing type, or an idea for a new UI control.
It's fine if you don't have all the details: you can start with the Summary and Problem to Solve.

This link describes the WinUI feature/API proposal process:
https://github.com/Microsoft/microsoft-ui-xaml/blob/master/docs/feature_proposal_process.md
-->

# Proposal: [your title here] 
<!-- Add a title for your feature or API proposal. Please be short and descriptive -->

## Summary
<!-- Include 1-2 sentences summarizing your feature or API proposal -->

<!-- If applicable, please provide before / after images -->
**Before** | **After**
------------ | -------------
Image | Image

## Problem to Solve
<!-- Create a list that describes WHY the feature / update should be added to WinUI for all developers and user - what problem is it solving?
Proposals often have multiple motives for why we should do the work, so list each one as a separate bullet.
If applicable you can also describe how the proposal aligns to the current WinUI roadmap and priorities in a separate paragraph:
https://github.com/Microsoft/microsoft-ui-xaml/blob/master/docs/roadmap.md
-->
* {First problem this proposed feature is solving}
* {Second problem this proposed feature is solving}
* {etc}

<!-- Provide user research if applicable.
Is there a research study that supports the rationale of this proposed update? -->
<!--## Supporting User Research-->

<!-- Provide competitive research if applicable (what are other teams' solutions, both within and outside of Microsoft? How can we leverage their existing solutions) -->
<!--## Existing Solutions-->

<!----------------------
The below sections are optional when submitting an idea or proposal.
All sections are required before we'll accept a PR to master, but aren't necessary to start the discussion.
------------------------>

## Scope
<!-- Please include a list of what the feature should and shouldn't do by filling in the table below.
'Must' implies that the feature should not ship without this capability.  
'Should' is something we should push hard for, but is not absolutely required to ship.
'Could' is a nice-to-have; a good stretch goal that isn't painful if we don't achieve it.
'Won't' is a clear statement that the proposal/feature will intentionally not have that capability.
This list will evolve and grow as the proposal becomes more refined over time.
A good rule of thumb is to start your proposal with no more than 7 high-level requirements.
-->
| Capability | Priority |
| :---------- | :------- |
| This proposal will allow developers to accomplish W | Must |
| This proposal will allow end users to accomplish X | Should |
| This proposal will allow developers to accomplish Y | Could |
| This proposal will allow end users to accomplish Z | Won't |

## Visuals Comps 

### Controls States
<!-- What visual states are required for this control and what do the visuals look like? -->

States | Image
------------ | -------------
StateName1 | Image1 
StateName2 | Image2
StateName3 | Image3

### Controls Property Mapping
<!-- What properties are editable in the XAML for the end users, and what does it map to? -->

Property Name | Type | Controls Mapping
------------ | ------------- | -------------
`PropertyName1` | Type | What does this property change about the control?
`PropertyName2` | Type | What does this property change about the control?
`PropertyName3` | Type | What does this property change about the control?
`Foreground` | Type | What part does the foreground property map to on the control?
`Background` | Type | What part does the background property map to on the control?
`Stroke` | Type | How does changing the stroke property affect the control?
`Margin` | Type| How does changing the margin property affect the control?
`Padding` | Type | How does changing the margin property affect the control?


## Context
<!-- Please include images of the control in different context -->

## Animation / Link to Prototype
<!-- If applicable, please include an animation or prototype to better demonstrate the control -->

## Important Notes
<!-- Please include any other important details.
This could include one or more of:
- an API proposal (any supported language or pseudocode is fine)
- other implementation notes
-->

## Open Questions
<!-- Please list any open issues that you think still need to be addressed.
These could include areas you think would benefit from community or WinUI team input -->
