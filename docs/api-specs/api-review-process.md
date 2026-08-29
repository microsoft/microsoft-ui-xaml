# API Review for WinUI

All APIs shipped in WinUI must go through the Windows API review process.

This document outlines the overall process and provides the guidelines that all APIs should adhere to.

This document describes the process from the point of view of WinUI.

## Experimental vs Stable APIs
APIs that are marked as Experimental do not need to go through API review. Experimental APIs will ship in experimental
releases of WinAppSDK only. They will not be available in stable releases of WinAppSDK. There is no guarantee of API
stability for experimental APIs so it is ok if they change in future releases. This is not the case for stable APIs
which cannot be changed once they ship.

New APIs that are under development should initially be marked as Experimental. Only after the API has gone through API
review should it be marked as stable.

## The Process
1. Work with one of the API Reps that are on the team.
2. Create an API Spec as a markdown doc in this repo.
3. Create a PR and review it locally.
4. Once the PR has been approved locally, work with the API rep to get an official API review scheduled.
5. Attend the API review meeting.
6. After the review, apply any feedback on the design to the API.
7. Mark the API as stable

## API Reps
Work with one of the API reps on the team.

## Creating the API spec
The first step is to create an API spec. Create a markdown document in this repo under the path `docs\api-specs`.
Use this doc as a template: https://github.com/microsoft/WindowsAppSDK/blob/main/specs/spec_template.md

When writing the spec you should write it with a particular audience in mind. You should assume that your reader is
broadly familiar with Xaml but is NOT an expert. You should not write the document with a member of the Xaml dev
team in mind. The API spec doc will form the basis of the public documentation so you should write with that in mind.
Also, the API spec will be reviewed by members of the Windows API review board, most of whom are not experts on Xaml.
Avoid using internal code names for things or getting too much into topics of 'inside baseball'.

If there is some information that is useful for internal discussions but is not to be a part of the public documentation
you can add a Spec Note. Just prefix the note with "Spec Note" and put it in _italic text_ to distinguish it from the
main content.

## Local review of API spec
Before taking the spec to the official API Review Board we should make sure that our local team reviews the spec first.
Create a PR into the main branch of this repo with your spec. Include the local API Rep in the review. Respond to any
feedback. Once the PR has been approved by the API rep, it can be merged to main.

## Official API Review
Create a new PR containing your API spec. You can do this by making a trivial whitespace change to the doc and creating
a PR from that. Alternatively, you can opt to not merge the original PR and use that for the official review instead.
Creating a new PR is often preferable, since it results in a fresh clean PR for the official review.

Work with your API rep to get an API review scheduled.

Members of the API Review Board will add comments to the PR before the meeting.

Once the API Review meeting has been scheduled, do not make any further updates to the PR until after the review has
occurred. Do not apply any feedback that is suggested in the PR comments until the review has been completed.

During the API Review, the API rep will go through each of the active comments and the meeting attendees will come to
a consensus on how to respond. Changes will be proposed to the doc and will be in the form of a comment prefixed with
'RECOMMENDED'.

## Apply Feedback to Spec and Implementation
After the API Review, respond to any feedback provided by updating the spec. Push your changes and get the PR merged to
main. If you have any questions on what to do, work with your API rep.

Once the API spec is finalized, make sure to make any required changes to the implementation to ensure that it matches
what is in the spec.

## Mark the new API as Stable
Remove the Experimental tag from the API and mark it as stable by adding it to the appropriate contract. Do not mark an
API as stable until it has gone through the full API review process. Once an API has been marked stable, do not make
any changes to the API as that will result in a breaking change.
