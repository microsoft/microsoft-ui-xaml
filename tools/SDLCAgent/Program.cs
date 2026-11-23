using System.Diagnostics;
using System.Net;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Text;
using Microsoft.AspNetCore.Server.Kestrel.Core;

namespace SDLCAgent;

public static class Program
{
    public static async Task<int> Main(string[] args)
    {
        try { return await Cli.Run(args); }
        catch (Exception ex) when (ex is not OutOfMemoryException)
        {
            Console.Error.WriteLine(Redaction.Apply(ex.Message));
            return 1;
        }
    }
}

public sealed record HostConnection(int Pid, int Port, string Token);

public static class Cli
{
    public static async Task<int> Run(string[] args)
    {
        var command = args.FirstOrDefault() ?? "help";
        var data = Path.GetFullPath(Option(args, "--data")
            ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "SDLCAgent"));
        var config = Path.GetFullPath(Option(args, "--config") ?? Path.Combine(AppContext.BaseDirectory, "appsettings.json"));
        var policy = Path.GetFullPath(Option(args, "--policy") ?? Path.Combine(AppContext.BaseDirectory, "execution-policy.json"));
        if (command == "help")
        {
            Console.WriteLine("""
                SDLCAgent - native trusted-repository autonomous SDLC orchestration
                start [--data DIR] [--config FILE] [--policy FILE]   Start independent local host
                serve [same options]                               Run host in the foreground
                stop [--data DIR]                                  Stop host (progress retained)
                doctor                                              Verify Copilot runtime/authentication
                create --request FILE [--data DIR]                  Queue a trusted-repository run
                list                                                List runs
                status RUN                                          Show run, tasks, and artifact identifiers
                approve RUN --version SPEC_ARTIFACT_ID               Approve exact specification
                feedback RUN --file FILE                            Answer questions / request spec revision
                pause RUN | resume RUN | cancel RUN                 Durable controls (pause/drain before cancel)
                events RUN [--task ID] [--before EVENT_ID]            Paginated execution history
                artifact ARTIFACT_ID                                Read a saved spec/plan/evidence document
                viewer                                              Print authenticated dashboard URL
                All host-client commands accept --data DIR. Closing CLI/browser does not stop the host.
                Never run elevated. Native worker processes are NOT a security sandbox.
                """);
            return 0;
        }
        if (command == "doctor")
        {
            using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(60));
            Console.WriteLine(await CopilotBackend.Doctor(OptionsLoader.Load(config).Copilot, timeout.Token));
            return 0;
        }
        if (command == "serve")
        {
            await AgentHost.Run(data, config, policy);
            return 0;
        }
        if (command == "start")
        {
            ExecutionPolicy.RequireNonElevated();
            Directory.CreateDirectory(data);
            var appHost = Path.ChangeExtension(typeof(Program).Assembly.Location, OperatingSystem.IsWindows() ? ".exe" : null);
            var executable = File.Exists(appHost) ? appHost : Environment.ProcessPath!;
            var start = new ProcessStartInfo(executable) { UseShellExecute = false, CreateNoWindow = true };
            if (string.Equals(Path.GetFileNameWithoutExtension(executable), "dotnet", StringComparison.OrdinalIgnoreCase))
                start.ArgumentList.Add(typeof(Program).Assembly.Location);
            foreach (var arg in new[] { "serve", "--data", data, "--config", config, "--policy", policy }) start.ArgumentList.Add(arg);
            if (!await IsRunning(data))
            {
                using var process = Process.Start(start) ?? throw new IOException("Could not start host.");
                for (var i = 0; i < 100 && !await IsRunning(data); i++)
                {
                    if (process.HasExited) throw new IOException("Host exited during startup. Check host.log in the data directory.");
                    await Task.Delay(200);
                }
                if (!await IsRunning(data)) throw new TimeoutException("Host startup did not become responsive. Check host.log.");
            }
            Console.WriteLine("Host running. CLI/browser closure will not stop it.");
            Console.WriteLine(Viewer(Connection(data)));
            return 0;
        }
        var connection = Connection(data);
        using var client = Client(connection);
        if (command == "viewer") { Console.WriteLine(Viewer(connection)); return 0; }
        string? route = command switch
        {
            "list" => "/api/runs", "status" => "/api/runs/" + Positional(args),
            "artifact" => "/api/artifacts/" + Positional(args),
            "events" => "/api/runs/" + Positional(args) + "/events?before=" + (Option(args, "--before") ?? long.MaxValue.ToString())
                + "&taskId=" + Uri.EscapeDataString(Option(args, "--task") ?? ""),
            "create" => "/api/runs", "stop" => "/api/stop",
            "approve" or "feedback" or "pause" or "resume" or "cancel" => "/api/runs/" + Positional(args) + "/" + command,
            _ => throw new InvalidDataException("Unknown command. Run SDLCAgent help.")
        };
        HttpResponseMessage response;
        if (command is "list" or "status" or "artifact" or "events") response = await client.GetAsync(route);
        else
        {
            var body = command switch
            {
                "create" => File.ReadAllText(Required(args, "--request")),
                "approve" => Json.Write(new ControlRequest(Required(args, "--version"))),
                "feedback" => Json.Write(new ControlRequest(Feedback: File.ReadAllText(Required(args, "--file")))),
                _ => "{}"
            };
            response = await client.PostAsync(route, new StringContent(body, Encoding.UTF8, "application/json"));
        }
        using (response)
        {
            var text = await response.Content.ReadAsStringAsync();
            if (!response.IsSuccessStatusCode) throw new WorkflowException($"HTTP {(int)response.StatusCode}: {text}");
            Console.WriteLine(text);
        }
        return 0;
    }

    public static HostConnection Connection(string data)
    {
        var path = Path.Combine(data, "host.json");
        if (!File.Exists(path)) throw new IOException("No host descriptor. Run SDLCAgent start first.");
        return Json.Read<HostConnection>(File.ReadAllText(path));
    }

    public static HttpClient Client(HostConnection connection)
    {
        if (connection.Port is < 1024 or > 65535) throw new InvalidDataException("Invalid host port.");
        var client = new HttpClient { BaseAddress = new Uri($"http://127.0.0.1:{connection.Port}"), Timeout = TimeSpan.FromSeconds(30) };
        client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", connection.Token);
        return client;
    }

    private static async Task<bool> IsRunning(string data)
    {
        if (!File.Exists(Path.Combine(data, "host.json"))) return false;
        try
        {
            using var client = Client(Connection(data));
            client.Timeout = TimeSpan.FromSeconds(1);
            using var result = await client.GetAsync("/api/health");
            return result.IsSuccessStatusCode;
        }
        catch (Exception ex) when (ex is HttpRequestException or TaskCanceledException or IOException)
        {
            return false;
        }
    }

    private static string Viewer(HostConnection connection) => $"http://127.0.0.1:{connection.Port}/#token={connection.Token}";
    private static string Positional(string[] args) => args.Length > 1 && !args[1].StartsWith('-')
        ? Uri.EscapeDataString(args[1]) : throw new InvalidDataException("A run or artifact identifier is required.");
    private static string Required(string[] args, string name) => Option(args, name) ?? throw new InvalidDataException($"Missing {name}.");
    private static string? Option(string[] args, string name)
    {
        var index = Array.IndexOf(args, name);
        if (index < 0) return null;
        if (index + 1 == args.Length || args[index + 1].StartsWith("--", StringComparison.Ordinal))
            throw new InvalidDataException($"Missing value for {name}.");
        return args[index + 1];
    }
}

public static class AgentHost
{
    public static async Task Run(string directory, string configFile, string policyFile)
    {
        ExecutionPolicy.RequireNonElevated();
        var options = OptionsLoader.Load(configFile);
        var policy = new ExecutionPolicy(File.ReadAllText(policyFile));
        Directory.CreateDirectory(directory);
        using var ownership = new FileStream(Path.Combine(directory, "host.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        var (app, token) = Build(directory, options, policy);
        await using (app)
        {
            await app.StartAsync();
            var descriptor = Path.Combine(directory, "host.json");
            await File.WriteAllTextAsync(descriptor + ".tmp", Json.Write(new HostConnection(Environment.ProcessId, options.Host.Port, token)));
            File.Move(descriptor + ".tmp", descriptor, overwrite: true);
            await app.WaitForShutdownAsync();
        }
    }

    internal static (WebApplication App, string Token) Build(string directory, AgentOptions options, ExecutionPolicy policy, IAgentBackend? backend = null)
    {
        var store = new Store(directory, options.Execution.MaxArtifactBytes);
        var builder = WebApplication.CreateBuilder(new WebApplicationOptions
        {
            ContentRootPath = AppContext.BaseDirectory, WebRootPath = Path.Combine(AppContext.BaseDirectory, "wwwroot"), Args = []
        });
        builder.WebHost.ConfigureKestrel(server => server.Listen(IPAddress.Loopback, options.Host.Port,
            listen => listen.Protocols = HttpProtocols.Http1));
        builder.Logging.ClearProviders();
        builder.Logging.AddProvider(new FileLoggerProvider(Path.Combine(directory, "host.log"), options.Execution.RawLogRetentionDays));
        builder.Services.Configure<Microsoft.Extensions.Hosting.HostOptions>(o => o.ShutdownTimeout = TimeSpan.FromSeconds(30));
        builder.Services.AddSingleton(options);
        builder.Services.AddSingleton(policy);
        builder.Services.AddSingleton(store);
        builder.Services.AddSingleton<ProcessRunner>();
        builder.Services.AddSingleton<GitWorkspace>();
        builder.Services.AddSingleton(options.Execution);
        builder.Services.AddSingleton<CheckRunner>();
        if (backend is null) builder.Services.AddSingleton<IAgentBackend, CopilotBackend>();
        else builder.Services.AddSingleton(backend);
        builder.Services.AddSingleton<AgentRunner>();
        builder.Services.AddSingleton<RunControls>();
        builder.Services.AddSingleton<Dashboard>();
        builder.Services.ConfigureHttpJsonOptions(o =>
        {
            o.SerializerOptions.UnmappedMemberHandling = System.Text.Json.Serialization.JsonUnmappedMemberHandling.Disallow;
            o.SerializerOptions.RespectNullableAnnotations = true;
            o.SerializerOptions.RespectRequiredConstructorParameters = true;
        });
        builder.Services.AddHostedService<Orchestrator>();
        var app = builder.Build();
        var token = Convert.ToHexString(RandomNumberGenerator.GetBytes(32));
        app.Use(async (context, next) =>
        {
            if (context.Request.Host.Host != "127.0.0.1" && context.Request.Host.Host != "localhost")
            { context.Response.StatusCode = 400; return; }
            context.Response.Headers.ContentSecurityPolicy = "default-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'";
            context.Response.Headers.XContentTypeOptions = "nosniff";
            context.Response.Headers.CacheControl = "no-store";
            if (context.Request.Path.StartsWithSegments("/api"))
            {
                var header = context.Request.Headers.Authorization.ToString();
                if (!CryptographicOperations.FixedTimeEquals(Encoding.UTF8.GetBytes(header), Encoding.UTF8.GetBytes("Bearer " + token)))
                { context.Response.StatusCode = 401; return; }
            }
            try { await next(); }
            catch (Exception ex) when (ex is InvalidDataException or PolicyException or KeyNotFoundException or WorkflowException or System.Text.Json.JsonException or BudgetException or ArgumentException)
            {
                context.Response.StatusCode = ex is KeyNotFoundException ? 404 : 400;
                await context.Response.WriteAsJsonAsync(new { error = Redaction.Apply(ex.Message) });
            }
        });
        app.UseDefaultFiles();
        app.UseStaticFiles();
        app.MapGet("/api/health", () => new { status = "ready", pid = Environment.ProcessId });
        app.MapGet("/api/config", () => new { options.Host, options.Execution, policyHash = policy.Hash, policy = System.Text.Json.JsonDocument.Parse(policy.Text).RootElement });
        app.MapGet("/api/runs", (int? offset) => store.Runs(offset ?? 0));
        app.MapGet("/api/runs/{id}", (string id) => new { run = store.Get(id), artifacts = store.Artifacts(id) });
        app.MapGet("/api/runs/{id}/events", (string id, string? taskId, long? before, int? limit) => store.Events(id, string.IsNullOrEmpty(taskId) ? null : taskId, before ?? long.MaxValue, limit ?? 100));
        app.MapGet("/api/runs/{id}/dashboard", (string id, Dashboard dashboard, CancellationToken token) => dashboard.Get(id, token));
        app.MapGet("/api/runs/{id}/commits", (string id, string? taskId, int? offset, Dashboard dashboard, CancellationToken token) => dashboard.Commits(id, taskId, offset ?? 0, token));
        app.MapGet("/api/runs/{id}/execution-commits", (string id, long? before, Dashboard dashboard) => dashboard.ExecutionCommits(id, before ?? long.MaxValue));
        app.MapGet("/api/artifacts/{id}", (string id) => Results.Text(store.ReadArtifact(id), "application/json"));
        app.MapPost("/api/runs", async (RunRequest request, GitWorkspace git, CancellationToken token) =>
        {
            Contracts.ValidateChecks(request.Checks);
            if (string.IsNullOrWhiteSpace(request.Repository)) throw new InvalidDataException("Repository path is required.");
            var normalized = request with { Repository = Path.GetFullPath(request.Repository) };
            var commit = await git.ValidateRepository(normalized, token);
            return store.Create(normalized, commit, policy);
        });
        app.MapPost("/api/runs/{id}/approve", (string id, ControlRequest request, RunControls controls) => controls.Approve(id, request));
        app.MapPost("/api/runs/{id}/feedback", (string id, ControlRequest request, RunControls controls) => controls.Feedback(id, request));
        app.MapPost("/api/runs/{id}/pause", (string id, RunControls controls) => controls.Pause(id));
        app.MapPost("/api/runs/{id}/resume", (string id, RunControls controls) => controls.Resume(id));
        app.MapPost("/api/runs/{id}/unblock", (string id, UnblockControlRequest request, RunControls controls) => controls.Unblock(id, request));
        app.MapPost("/api/runs/{id}/cancel", (string id, RunControls controls) => controls.Cancel(id));
        app.MapPost("/api/stop", (IHostApplicationLifetime lifetime) =>
        {
            lifetime.StopApplication();
            return new { status = "stopping", message = "Durable progress retained; restart reconciles unfinished work." };
        });
        return (app, token);
    }
}

internal sealed class FileLoggerProvider(string path, int retentionDays) : ILoggerProvider
{
    private readonly object sync = new();
    private readonly string logPath = path;
    private readonly int retention = retentionDays;
    public ILogger CreateLogger(string categoryName) => new FileLogger(this, categoryName);
    public void Dispose() { }

    private sealed class FileLogger(FileLoggerProvider owner, string category) : ILogger
    {
        public IDisposable? BeginScope<TState>(TState state) where TState : notnull => null;
        public bool IsEnabled(LogLevel logLevel) => logLevel >= LogLevel.Warning;
        public void Log<TState>(LogLevel logLevel, EventId eventId, TState state, Exception? exception,
            Func<TState, Exception?, string> formatter)
        {
            if (!IsEnabled(logLevel)) return;
            lock (owner.sync)
            {
                if (File.Exists(owner.logPath) && File.GetLastWriteTimeUtc(owner.logPath).Date < DateTime.UtcNow.Date)
                {
                    var archive = Path.Combine(Path.GetDirectoryName(owner.logPath)!, "raw-logs");
                    Directory.CreateDirectory(archive);
                    File.Move(owner.logPath, Path.Combine(archive, $"host-{File.GetLastWriteTimeUtc(owner.logPath):yyyyMMdd}-{Guid.NewGuid():N}.log"));
                    foreach (var old in Directory.EnumerateFiles(archive, "host-*.log"))
                        if (File.GetLastWriteTimeUtc(old) < DateTime.UtcNow.AddDays(-owner.retention)) File.Delete(old);
                }
                File.AppendAllText(owner.logPath, Redaction.Apply($"{DateTimeOffset.UtcNow:O} {logLevel} {category}: {formatter(state, exception)} {exception}\n"));
            }
        }
    }
}
