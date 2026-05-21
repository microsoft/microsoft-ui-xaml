# Documentation Style Guide

## Table of Contents

- [Headings](#headings)
- [Tables of Contents](#tables-of-contents)
- [Code](#code)
- [Links](#links)
- [Bold and italic text](#bold-and-italic-text)
- [Tables](#tables)
- [Comments](#comments)
- [Readability](#readability)

Here are some style guides and best practices for documentation in the WinUI 3 repo. The suggestions here are not
hard-and-fast rules and are only enforced by the contributors to this repo, and you can choose alternative formatting
on a case-by-case basis. You should, however, ensure that your formatting works in the ADO Preview pane, where most
people will be reading these articles.

This style guide is based on standard Markdown, as well as the
[Microsoft Documentation Contributor Guide](https://review.docs.microsoft.com/en-us/help/contribute/markdown-reference?branch=main).
Not everything there will apply, however, as the public Microsoft docs have various plug-ins that our repo in ADO does not.

## Headings

``` markdown
# This is a first level heading (H1)

## This is a second level heading (H2)

...

###### This is a sixth level heading (H6)
```

* Each Markdown file must have one and only one H1 heading, which should be the title and first content in the file.
* Many levels of headings should be avoided, as they can be hard to differentiate when reading, and make a Table of
Contents confusingly long (more below).
* Headings should be created with `#` symbols, not `===` or `---` syntax.

## Tables of Contents

Tables of Contents are very useful in long documents. To create a table of contents, add a **\[[\_TOC\_]]**.
The TOC is generated when the tag gets added and there's at least one heading on the page.

Tables of Contents should be placed after the title (H1) and before any other headings.

## Code

There are several ways to include code in an article.

* Individual elements (words) within a line.
  * Here's an example of `code` style.
  * Use code format when referring to named parameters and variables in a nearby code block in your text. Code format
  may also be used for properties, methods, classes, and language keywords.
* Code blocks
  * Blocks are created by surrounding code with triple backticks (three \` characters). Do not create code blocks with
    only indentation.
  * Include the programming language to get syntax highlighting.
    * When specifying language, use lowercase -- some editors work well with uppercase, but ADO will only give syntax
      highlighting in the raw view when using lowercase.

  ```markdown
      ```csharp
      public static void Log(string message)
      {
          _logger.LogInformation(message);
      }
      ```
  ```

<!-- ADO bug means that without this blank code snippet, Contents tab formatting is broken. The code-ception above messes it up.
```cpp
```
-->

* Placeholders
  * If you want the user to replace a section of displayed code with their own values, use placeholder text marked off
  by angle brackets. For example:
    * `az group delete -n <ResourceGroupName>`

## Links

Much of the [public documentation](https://docs.microsoft.com/en-us/contribute/how-to-write-links) regarding links applies to this repo.

* Descriptive link text (rather than "click here" links) should be preferred.
* **TODO** to relative URL links for work us?
* **File links** are used to link from one file to another within the repo:
  * All file paths use forward-slash (/) characters instead of back-slash characters.
  * An article links to another article in the same directory:
    *  `[link text](article-name.md)`
  * An article links to an article in a subdirectory of the parent directory of the current directory:
    * `[link text](../directory/article-name.md)`
* **Bookmark Links**
  * For a bookmark link to a heading in the current file, use a hash symbol followed by the lowercase words of the
  heading. Remove punctuation from the heading and replace spaces with dashes:
    * ` [Managed Disks](#managed-disks) `
  * To link to a bookmark heading in another article, use the file-relative or site-relative link plus a hash symbol,
  followed by the words of the heading. Remove punctuation from the heading and replace spaces with dashes:
    * ` [Managed Disks](../../linux/overview.md#managed-disks) `

## Bold and italic text

* To format text as bold, enclose it in two asterisks:
  * This text is **bold** (\*\*bold\*\*).
* To format text as italic, enclose it in a single asterisk:
  * This text is *italic* (\*italic\*).
* To format text as both bold and italic, enclose it in three asterisks:
  * This text is both ***bold and italic*** (\*\*\*bold and italic\*\*\*).

## Tables

See the [official documentation](https://review.docs.microsoft.com/en-us/help/contribute/markdown-reference?branch=main#tables)
for how to format tables. There are different ways to do tables in Markdown, but this one works in ADO. (The section
about line breaking using a custom div class does not apply to our repo).

## Comments

ADO supports HTML comments if you must comment out sections of your article: `<!--- Here's my comment --->`

## Readability

* All headings should be surrounded by blank lines.

   ```markdown
   some text here. There will be a blank line before the next heading.

   ## Heading 2

   Some more text after a blank line.
   ```

* All code blocks should be surrounded by blank lines.
* **Word wrapping**: The raw markdown view in ADO does not wrap lines, which can make the raw view very hard to read.
Consider writing your documents with newlines at about 120 columns. You can put a guide line in VS Code to help you, or
use an VS Code extension like Rewrap. You may want to leave the long lines in very long articles where such manual
manipulation would be a lot of work, or in articles you expect to change with some frequency (and changing the newlines
would make history very hard to read). You could also consider only adding newlines after getting PR approval on your
document, since at that point most changes will already have been made.