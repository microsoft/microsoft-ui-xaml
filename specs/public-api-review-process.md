WinUI Platform Public Spec Review Process
===

## Introduction

We are excited to formally re-introduce a community feedback process to our API authoring workflow! This will leverage the collective wisdom and insights of the community, allowing us to develop more robust and comprehensive APIs that meet the needs of our diverse user base.

We assure our community that their input is invaluable to us, and we are dedicated to fostering a transparent and collaborative environment. By adopting this structured feedback process outlined below, we aim to set clear expectations and handle all contributions with the utmost professionalism and consideration.

## Public Post on Github

- The drafted spec from is posted as a PR under the `./specs/` directory.

  - Example PR: [Popup placement properties API spec](https://github.com/microsoft/microsoft-ui-xaml/pull/4905)
  - Every spec will link to this public spec markdown file. <!-- TODO: Add link once it is public -->
- A dedicated discussion thread will be created to announce that we are actively gathering feedback for the API.
- A Github issue with `spec-review` tag will be created to represent the new API proposal. It will be tracked in the Github API Spec Review Board:

> [API Specs Review](https://github.com/orgs/microsoft/projects/1328/views/1)

- If the proposed API addresses any existing Github issues, the existing issue will be linked to the `spec-review` issue representing the API proposal.

## Gather Feedback

The issue is open for a predefined period of **1 month** to gather feedback from the community.
The start and end dates will be outlined in the spec and in the [API Specs Review](https://github.com/orgs/microsoft/projects/1328/views/1) Board.

- Community members can provide feedback by **commenting directly on the PR**, or through a **code suggestion** with Github tools.
- Feedback should be constructive and specific, addressing potential use cases, design concerns, and functionality.

> **Examples of constructive feedback:**
>
> - Developer use cases and edge cases – sample code and screenshots of such scenarios are very much welcomed.
>
>   - ~~Don't: "You should use a ToggleButton and have it to the left"~~
>   - Do: "Can I customize the content here? Here is some code and screenshots showcasing my needed scenario.”
>
> - Consistent API naming conventions – and comparisons of said APIs across WinUI and industry standards.
>
>   - ~~Don't: "This is a bad name, it should be called something different."~~
>   - Do: "Normally, these types of properties are named IsFoo, like IsFoo on X's BarControl. < link to learn.microsoft.com API property page or other framework's documentation />"
>
> - Raised accessibility concerns and use cases
>
>   - ~~Don't: "This control is not accessible."~~
>   - Do: "How does this control support screen readers? Here is an example of how it should announce state changes to ensure accessibility."
> - Raised usability concerns with different languages
>
>   - ~~Don't: "This won't work support other languages."~~
>   - Do: “How will the control layout handle RTL languages? Here is an example of what could appear on the control.”
>
> - Inquiries and suggestions regarding scenarios and usages
>
>   - ~~Don't: "This control is missing features."~~
>   - Do: “Does it have an orientation property to toggle horizontal and vertical? Here is an example of how this property can be used in different scenarios.”

## Handle Feedback

We are committed to respond to all feedback in different manners depending on the feedback category.

**Feedback categorization:** In general, feedback will be prioritized into the categories below based on relevance, impact, and feasibility.

- **Immediate Action:** Feedback that will be addressed in the current specification iteration due to its high relevance and feasibility.
- **Deferred Action:** Feedback that is valuable but not feasible for immediate implementation. This will be considered for future updates.
- **Not Planned:** Feedback that is not aligned with current goals or is not feasible to implement. Reasons for this will be clearly communicated.
- **Discussion:** Feedback is an open-ended discussion, and no immediate action is necessary.

**Acknowledgment:** All feedback will be acknowledged by at least one of the following means:

- **Immediate & Deferred Action:**

  - Final post calling out specific points that influenced decisions

        *and / or*

  - In-line response to comments

        *and / or*

  - “Resolve comment” to signify that the feedback has been incorporated into the spec draft

        *and / or*

  - Directly commit the suggestion if it is done via code suggestion with Github tools, and it is appropriate

- **Not Planned:**

  - In-line response to comments explaining reasoning

        *and*

  - “Resolve comment” to signify that it is not planned

- **Discussion:**

  - An “eye” emoji by the author / poster to signify that it is read.

## Consolidate Feedback

We will carefully consider each contribution and update the spec to reflect the accepted feedback. A summary of changes will be shared, highlighting each significant feedback point. Finally, the PR containing the final spec is merged in.

### Passive Monitoring of Relevant Feedback and Issues

- Continuous monitoring for any late feedback or issues is passively maintained.
- Future enhancements or changes based on post-implementation feedback are considered in subsequent spec reviews.
