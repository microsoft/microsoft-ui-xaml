// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Text.Json;
using Microsoft.Extensions.DependencyInjection;
using Microsoft.Extensions.Hosting;
using Microsoft.Extensions.Logging;
using ModelContextProtocol;
using ModelContextProtocol.Protocol;
using ModelContextProtocol.Server;
using WinUIProfiler.Mcp.SelfTest;

namespace WinUIProfiler.Mcp;

internal static class Program
{
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
            case "--selftest" when args.Length <= 2:
                return SelfTestRunner.Run(args.Length == 2 ? args[1] : null, output);
            default:
                Console.Error.WriteLine("Usage: WinUIProfiler.Mcp [--version | --listtools | --selftest [filter]]");
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
        var builder = Host.CreateEmptyApplicationBuilder(settings: null);
        builder.Logging.AddConsole(options => options.LogToStandardErrorThreshold = LogLevel.Trace);
        AddServer(builder.Services).WithStdioServerTransport();
        using var host = builder.Build();
        await host.RunAsync().ConfigureAwait(false);
    }

    private static IMcpServerBuilder AddServer(IServiceCollection services) =>
        services
            .AddMcpServer(options => options.ServerInfo = new Implementation
            {
                Name = ServerVersion.ServerName,
                Version = ServerVersion.Value,
            })
            .WithToolsFromAssembly(typeof(Program).Assembly, ToolSerializerOptions);
}
