// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Diagnostics;
using System.Reflection;
using System.Text.Json;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;
using WinUITools.Telemetry;

namespace WinUIProfiler.Mcp;

internal static class Program
{
    // The server's tool names. Usage events name only these, never what a client sent.
    private static readonly HashSet<string> ToolNames =
    [
        .. typeof(Program).Assembly.GetTypes()
            .SelectMany(type => type.GetMethods(BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static))
            .Select(method => method.GetCustomAttribute<McpServerToolAttribute>()?.Name)
            .OfType<string>()
    ];

    /// <summary>
    /// Serializes tool arguments and results. XAML trees nest far deeper than the default limit of 64. Dictionary keys,
    /// such as event field names, keep their case.
    /// </summary>
    internal static JsonSerializerOptions ToolSerializerOptions { get; } = new(McpJsonUtilities.DefaultOptions)
    {
        MaxDepth = 512,
        DictionaryKeyPolicy = null,
    };

    private static async Task<int> Main(string[] args)
    {
        // Standard output carries only the server's JSON-RPC messages: the stdio transport writes to
        // the raw stream, and anything else written to Console.Out goes to standard error.
        var output = Console.Out;
        Console.SetOut(Console.Error);

        if (args.Length == 0)
        {
            await RunServerAsync().ConfigureAwait(false);
            return 0;
        }

        switch (args[0])
        {
            case "--version" when args.Length == 1:
                output.WriteLine(ServerVersion.Value);
                return 0;
            case "--listtools" when args.Length == 1:
                foreach (var tool in ListTools())
                {
                    output.WriteLine(tool.ProtocolTool.Name);
                }

                return 0;
            default:
                Console.Error.WriteLine("Usage: WinUIProfiler.Mcp [--version | --listtools]");
                Console.Error.WriteLine("Without arguments, runs the MCP server over standard input and output.");
                return 2;
        }
    }

    /// <summary>Returns the tools the server offers, in registration order.</summary>
    internal static IReadOnlyList<McpServerTool> ListTools()
    {
        var builder = Host.CreateEmptyApplicationBuilder(settings: null);
        AddServer(builder.Services);
        using var host = builder.Build();
        return [.. host.Services.GetServices<McpServerTool>()];
    }

    private static async Task RunServerAsync()
    {
        UsageTelemetry.Initialize("profiler", "mcp", ServerVersion.Value);
        var builder = Host.CreateEmptyApplicationBuilder(settings: null);
        builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
        AddServer(builder.Services).WithStdioServerTransport();
        using var host = builder.Build();
        await host.RunAsync().ConfigureAwait(false);
        await UsageTelemetry.FlushAsync(TimeSpan.FromSeconds(2)).ConfigureAwait(false);
    }

    private static IMcpServerBuilder AddServer(IServiceCollection services) =>
        services
            .AddMcpServer(options => options.ServerInfo = new Implementation
            {
                Name = ServerVersion.ServerName,
                Version = ServerVersion.Value,
            })
            .WithToolsFromAssembly(typeof(Program).Assembly, ToolSerializerOptions)
            .WithRequestFilters(filters => filters.AddCallToolFilter(TrackToolCall));

    // Sends one usage event for each tool call: the tool's name, outcome and duration, never its arguments or result.
    private static McpRequestHandler<CallToolRequestParams, CallToolResult> TrackToolCall(
        McpRequestHandler<CallToolRequestParams, CallToolResult> next) =>
        async (request, cancellationToken) =>
        {
            var started = Stopwatch.GetTimestamp();
            var outcome = "failed";
            try
            {
                var result = await next(request, cancellationToken).ConfigureAwait(false);
                outcome = result.IsError == true ? "failed" : "success";
                return result;
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                outcome = "cancelled";
                throw;
            }
            finally
            {
                var name = request.Params?.Name;
                UsageTelemetry.Track("Profiler.McpToolCall", new Dictionary<string, string>
                {
                    ["mcpTool"] = name is not null && ToolNames.Contains(name) ? name : "unknown",
                    ["outcome"] = outcome,
                }, new Dictionary<string, double> { ["durationMs"] = Stopwatch.GetElapsedTime(started).TotalMilliseconds });
            }
        };
}
