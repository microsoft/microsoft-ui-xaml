// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.UI.Xaml.Markup.Compiler.Microsoft.Xaml.XamlCompiler
{
    class SuccinctSyntaxListener: SuccinctSyntaxBaseListener
	{
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.program"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterProgram([NotNull] SuccinctSyntaxParser.ProgramContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.program"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitProgram([NotNull] SuccinctSyntaxParser.ProgramContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.items"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterItems([NotNull] SuccinctSyntaxParser.ItemsContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.items"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitItems([NotNull] SuccinctSyntaxParser.ItemsContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.z"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterZ([NotNull] SuccinctSyntaxParser.ZContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.z"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitZ([NotNull] SuccinctSyntaxParser.ZContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.item"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterItem([NotNull] SuccinctSyntaxParser.ItemContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.item"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitItem([NotNull] SuccinctSyntaxParser.ItemContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.text"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterText([NotNull] SuccinctSyntaxParser.TextContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.text"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitText([NotNull] SuccinctSyntaxParser.TextContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.literal_text"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterLiteral_text([NotNull] SuccinctSyntaxParser.Literal_textContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.literal_text"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitLiteral_text([NotNull] SuccinctSyntaxParser.Literal_textContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.reserved_symbol"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterReserved_symbol([NotNull] SuccinctSyntaxParser.Reserved_symbolContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.reserved_symbol"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitReserved_symbol([NotNull] SuccinctSyntaxParser.Reserved_symbolContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.quote"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterQuote([NotNull] SuccinctSyntaxParser.QuoteContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.quote"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitQuote([NotNull] SuccinctSyntaxParser.QuoteContext context) { }
		/// <summary>
		/// Enter a parse tree produced by <see cref="SuccinctSyntaxParser.sequence"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void EnterSequence([NotNull] SuccinctSyntaxParser.SequenceContext context) { }
		/// <summary>
		/// Exit a parse tree produced by <see cref="SuccinctSyntaxParser.sequence"/>.
		/// <para>The default implementation does nothing.</para>
		/// </summary>
		/// <param name="context">The parse tree.</param>
		public virtual void ExitSequence([NotNull] SuccinctSyntaxParser.SequenceContext context) { }

		/// <inheritdoc/>
		/// <remarks>The default implementation does nothing.</remarks>
		public virtual void EnterEveryRule([NotNull] ParserRuleContext context) { }
		/// <inheritdoc/>
		/// <remarks>The default implementation does nothing.</remarks>
		public virtual void ExitEveryRule([NotNull] ParserRuleContext context) { }
		/// <inheritdoc/>
		/// <remarks>The default implementation does nothing.</remarks>
		public virtual void VisitTerminal([NotNull] ITerminalNode node) { }
		/// <inheritdoc/>
		/// <remarks>The default implementation does nothing.</remarks>
		public virtual void VisitErrorNode([NotNull] IErrorNode node) { }
	}
}
