# Teaching Tip - Focus Behavior

### Non-light-dismiss TeachingTip:
- When it opens, it does not take focus.
- You can manually move focus to the teaching tip by pressing f6.
- You can move focus back to the main page by pressing f6 again. 
- Focus moves back to the element that previously had focus before the teaching tip opened.

### For a light-dismiss teaching tip:
- When it opens it does take focus.
- Once focus moves out of the teaching tip, it will close (due to light dismiss). 
- You can move focus out of the teaching tip with f6 or esc.
- When the teachingtip closes, focus moves back to the element in the page that originally had focus.

### Dismissal during animations
- Closing during the opening animation must still close the tip, including when the light-dismiss indicator popup has already closed ([#9143](https://github.com/microsoft/microsoft-ui-xaml/issues/9143)).
- Reopening during the closing animation remains ignored, to avoid an open `IsOpen` property with a closed popup ([#6541](https://github.com/microsoft/microsoft-ui-xaml/pull/6541)).
- A pending `Closing` deferral keeps the tip busy even after the opening animation finishes. Canceling the deferral restores the open state.

The TeachingTip API regression tests exercise these transitions with real composition animations and bounded waits. The light-dismiss test closes the indicator popup directly; it does not replace interactive testing of outside clicks, Escape, or window deactivation. To check those paths in the TeachingTip test page, enable light dismiss, lengthen the expand animation using the existing animation-duration controls, and dismiss before it finishes. Repeat with normal animation durations and rapid open/close input.
