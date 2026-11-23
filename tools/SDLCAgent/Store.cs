using System.Security.Cryptography;
using System.Text;
using Microsoft.Data.Sqlite;

namespace SDLCAgent;

public sealed class Store
{
    private readonly object sync = new();
    private readonly string connectionString;
    private readonly long maxArtifactBytes;
    public string DataDirectory { get; }

    public Store(string directory, long maxArtifactBytes = long.MaxValue)
    {
        DataDirectory = Path.GetFullPath(directory);
        Directory.CreateDirectory(DataDirectory);
        Directory.CreateDirectory(Path.Combine(DataDirectory, "artifacts"));
        Directory.CreateDirectory(Path.Combine(DataDirectory, "workspaces"));
        this.maxArtifactBytes = maxArtifactBytes;
        connectionString = new SqliteConnectionStringBuilder
        {
            DataSource = Path.Combine(DataDirectory, "sdlc.db"),
            DefaultTimeout = 30
        }.ToString();
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            PRAGMA journal_mode=WAL;
            CREATE TABLE IF NOT EXISTS metadata (version INTEGER NOT NULL);
            INSERT INTO metadata SELECT 1 WHERE NOT EXISTS (SELECT 1 FROM metadata);
            CREATE TABLE IF NOT EXISTS runs (id TEXT PRIMARY KEY, updated TEXT NOT NULL, data TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS events (
                id INTEGER PRIMARY KEY AUTOINCREMENT, run_id TEXT NOT NULL, task_id TEXT,
                kind TEXT NOT NULL, message TEXT NOT NULL, document_id TEXT, at TEXT NOT NULL,
                assignment_id TEXT, role TEXT, plan_id TEXT, step_id TEXT, attempt INTEGER);
            CREATE INDEX IF NOT EXISTS events_run ON events(run_id, id);
            CREATE TABLE IF NOT EXISTS artifacts (
                id TEXT PRIMARY KEY, run_id TEXT NOT NULL, task_id TEXT, kind TEXT NOT NULL,
                path TEXT NOT NULL, sha256 TEXT NOT NULL, at TEXT NOT NULL);
            CREATE TABLE IF NOT EXISTS failed_gates (
                run_id TEXT NOT NULL, check_name TEXT NOT NULL, source_tree TEXT NOT NULL, evidence_id TEXT NOT NULL,
                PRIMARY KEY (run_id, check_name, source_tree));
            CREATE TABLE IF NOT EXISTS failed_gate_contexts (
                run_id TEXT NOT NULL, check_name TEXT NOT NULL, source_tree TEXT NOT NULL, execution_context TEXT NOT NULL, evidence_id TEXT NOT NULL,
                PRIMARY KEY (run_id, check_name, source_tree, execution_context));
            CREATE TABLE IF NOT EXISTS build_evidence (
                run_id TEXT NOT NULL, workspace TEXT NOT NULL, check_name TEXT NOT NULL,
                definition TEXT NOT NULL, fingerprint TEXT NOT NULL, evidence_id TEXT NOT NULL,
                PRIMARY KEY (run_id, workspace, check_name));
            CREATE INDEX IF NOT EXISTS events_kind ON events(run_id, kind, id);
            CREATE INDEX IF NOT EXISTS events_plan ON events(run_id, plan_id, id);
            CREATE INDEX IF NOT EXISTS artifacts_kind ON artifacts(run_id, kind, at);
            """;
        command.ExecuteNonQuery();
        command.CommandText = "SELECT version FROM metadata";
        if (Convert.ToInt32(command.ExecuteScalar()) != 1)
            throw new InvalidDataException("Unsupported database version. Do not open a newer database with this host.");
    }

    private SqliteConnection Open()
    {
        var db = new SqliteConnection(connectionString);
        db.Open();
        return db;
    }

    public RunData Create(RunRequest request, string commit, ExecutionPolicy policy)
    {
        lock (sync)
        {
            var run = new RunData
            {
                Id = Guid.NewGuid().ToString("N"), Request = request with { Problem = Redaction.Apply(request.Problem) }, InitialCommit = commit,
                PolicyJson = policy.Text, PolicyHash = policy.Hash
            };
            using var db = Open();
            using var transaction = db.BeginTransaction();
            using (var ownership = db.CreateCommand())
            {
                ownership.Transaction = transaction;
                ownership.CommandText = """
                    SELECT COUNT(*) FROM runs WHERE lower(json_extract(data,'$.request.repository'))=lower($repo)
                    AND json_extract(data,'$.request.branch')=$branch AND json_extract(data,'$.state') NOT IN ('Completed','Canceled')
                    """;
                ownership.Parameters.AddWithValue("$repo", request.Repository);
                ownership.Parameters.AddWithValue("$branch", request.Branch);
                if (Convert.ToInt64(ownership.ExecuteScalar()) != 0)
                    throw new PolicyException("An unfinished run already owns this integration branch, including paused/blocked runs. Cancel it before starting a replacement.");
            }
            Execute(db, transaction, "INSERT INTO runs VALUES ($id,$updated,$data)",
                ("$id", run.Id), ("$updated", run.Updated.ToString("O")), ("$data", Json.Write(run)));
            Append(db, transaction, run.Id, null, "RunCreated", "Run queued; awaiting repository baseline and specification.", null);
            transaction.Commit();
            return run;
        }
    }

    public RunData Get(string id)
    {
        lock (sync)
        {
            using var db = Open();
            using var command = db.CreateCommand();
            command.CommandText = "SELECT data FROM runs WHERE id=$id";
            command.Parameters.AddWithValue("$id", id);
            return Json.Read<RunData>(command.ExecuteScalar() as string ?? throw new KeyNotFoundException("Run not found."));
        }
    }

    public RunData[] Runs(int offset = 0, int limit = 100)
    {
        lock (sync)
        {
            using var db = Open();
            using var command = db.CreateCommand();
            command.CommandText = "SELECT data FROM runs ORDER BY updated DESC LIMIT $limit OFFSET $offset";
            command.Parameters.AddWithValue("$limit", Math.Clamp(limit, 1, 200));
            command.Parameters.AddWithValue("$offset", Math.Max(0, offset));
            using var reader = command.ExecuteReader();
            var result = new List<RunData>();
            while (reader.Read()) result.Add(Json.Read<RunData>(reader.GetString(0)));
            return result.ToArray();
        }
    }

    public string[] ActiveRunIds()
    {
        lock (sync)
        {
            using var db = Open();
            using var command = db.CreateCommand();
            command.CommandText = """
                SELECT id FROM runs WHERE json_extract(data,'$.state') IN ('Queued','Running','PauseRequested')
                ORDER BY updated
                """;
            using var reader = command.ExecuteReader();
            var ids = new List<string>();
            while (reader.Read()) ids.Add(reader.GetString(0));
            return ids.ToArray();
        }
    }

    public RunData Change(string id, string kind, string message, Action<RunData> change,
        string? taskId = null, string? documentId = null)
    {
        lock (sync)
        {
            var run = Get(id);
            change(run);
            run.Updated = DateTimeOffset.UtcNow;
            run.Revision++;
            using var db = Open();
            using var transaction = db.BeginTransaction();
            Execute(db, transaction, "UPDATE runs SET data=$data, updated=$updated WHERE id=$id",
                ("$data", Json.Write(run)), ("$updated", run.Updated.ToString("O")), ("$id", id));
            var task = taskId is null ? null : run.Tasks.SingleOrDefault(t => t.Definition.Id == taskId);
            using var scope = AuditScope.Use(AuditScope.Current with
            {
                AssignmentId = AuditScope.Current.AssignmentId ?? task?.AssignmentId,
                Attempt = AuditScope.Current.Attempt ?? task?.Attempts
            });
            Append(db, transaction, id, taskId, kind, message, documentId);
            transaction.Commit();
            return run;
        }
    }

    public void Event(string runId, string kind, string message, string? taskId = null, string? documentId = null)
    {
        lock (sync)
        {
            using var db = Open();
            Append(db, null, runId, taskId, kind, message, documentId);
        }
    }

    public Artifact Save<T>(string runId, string? taskId, string kind, T value)
    {
        lock (sync)
        {
            var text = Redaction.Json(value);
            var root = Path.Combine(DataDirectory, "artifacts");
            var size = Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories).Sum(p => new FileInfo(p).Length);
            if (size + Encoding.UTF8.GetByteCount(text) > maxArtifactBytes)
                throw new BudgetException("Artifact disk quota reached. Increase the quota before resuming; evidence was not discarded.");
            var id = Guid.NewGuid().ToString("N");
            var relative = Path.Combine(runId, id + ".json");
            var path = Path.Combine(root, relative);
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path, text, new UTF8Encoding(false));
            var artifact = new Artifact(id, runId, taskId, kind, relative,
                Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(text))), DateTimeOffset.UtcNow);
            using var db = Open();
            Execute(db, null, "INSERT INTO artifacts VALUES ($id,$run,$task,$kind,$path,$hash,$at)",
                ("$id", id), ("$run", runId), ("$task", taskId), ("$kind", kind), ("$path", relative),
                ("$hash", artifact.Sha256), ("$at", artifact.At.ToString("O")));
            return artifact;
        }
    }

    public Artifact GetArtifact(string id)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = "SELECT id,run_id,task_id,kind,path,sha256,at FROM artifacts WHERE id=$id";
        command.Parameters.AddWithValue("$id", id);
        using var reader = command.ExecuteReader();
        if (!reader.Read()) throw new KeyNotFoundException("Artifact not found.");
        return new Artifact(reader.GetString(0), reader.GetString(1), reader.IsDBNull(2) ? null : reader.GetString(2),
            reader.GetString(3), reader.GetString(4), reader.GetString(5), DateTimeOffset.Parse(reader.GetString(6)));
    }

    public string ReadArtifact(string id)
    {
        var artifact = GetArtifact(id);
        var text = File.ReadAllText(Path.Combine(DataDirectory, "artifacts", artifact.Path));
        if (Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(text))) != artifact.Sha256)
            throw new InvalidDataException("Artifact integrity check failed.");
        return text;
    }

    public string? RecordGate(string runId, string name, string tree, string evidenceId, bool passed, string executionContext)
    {
        lock (sync)
        {
            using var db = Open();
            using var legacy = db.CreateCommand();
            legacy.CommandText = "SELECT evidence_id FROM failed_gates WHERE run_id=$run AND check_name=$name AND source_tree=$tree";
            legacy.Parameters.AddWithValue("$run", runId); legacy.Parameters.AddWithValue("$name", name); legacy.Parameters.AddWithValue("$tree", tree);
            if (legacy.ExecuteScalar() is string previous)
            {
                var saved = GetArtifact(previous);
                using var document = System.Text.Json.JsonDocument.Parse(ReadArtifact(previous));
                var root = document.RootElement;
                var check = Json.Read<CheckCommand>(root.GetProperty("check").GetRawText());
                if (saved.RunId != runId || saved.Kind != "CommandEvidence" || !root.GetProperty("gate").GetBoolean()
                    || root.GetProperty("sourceTree").GetString() != tree || check.Name != name || root.GetProperty("sourceChanged").GetBoolean()
                    || root.GetProperty("exitCode").GetInt32() == 0 && !root.GetProperty("timedOut").GetBoolean() && !root.GetProperty("outputTruncated").GetBoolean())
                    throw new InvalidDataException("Historical gate failure is not bound to its recorded run, check and source tree.");
                var previousContext = CheckRunner.GateContext(check, root.GetProperty("workspace").GetString()
                    ?? throw new InvalidDataException("Historical gate failure has no workspace."));
                Execute(db, null, "INSERT OR IGNORE INTO failed_gate_contexts VALUES ($run,$name,$tree,$context,$evidence)",
                    ("$run", runId), ("$name", name), ("$tree", tree), ("$context", previousContext), ("$evidence", previous));
                if (previousContext != executionContext)
                    Event(runId, "GateContextChanged", $"Historical failure {previous} used a different command/workspace layout. Evidence is retained; the corrected context requires fresh gates.",
                        GetArtifact(evidenceId).TaskId, evidenceId);
            }
            if (!passed)
                Execute(db, null, "INSERT OR IGNORE INTO failed_gate_contexts VALUES ($run,$name,$tree,$context,$evidence)",
                    ("$run", runId), ("$name", name), ("$tree", tree), ("$context", executionContext), ("$evidence", evidenceId));
            using var command = db.CreateCommand();
            command.CommandText = "SELECT evidence_id FROM failed_gate_contexts WHERE run_id=$run AND check_name=$name AND source_tree=$tree AND execution_context=$context";
            command.Parameters.AddWithValue("$run", runId);
            command.Parameters.AddWithValue("$name", name);
            command.Parameters.AddWithValue("$tree", tree);
            command.Parameters.AddWithValue("$context", executionContext);
            return command.ExecuteScalar() as string;
        }
    }

        public void InvalidateBuilds(string runId, string workspace, string? name = null)
        {
            using var db = Open();
            Execute(db, null, "DELETE FROM build_evidence WHERE run_id=$run AND workspace=$workspace AND ($name IS NULL OR check_name=$name)",
                ("$run", runId), ("$workspace", workspace), ("$name", (object?)name ?? DBNull.Value));
        }

        public void RecordBuild(string runId, string workspace, CheckCommand check, string fingerprint, string evidenceId)
        {
            using var db = Open();
            Execute(db, null, "INSERT OR REPLACE INTO build_evidence VALUES ($run,$workspace,$name,$definition,$fingerprint,$evidence)",
                ("$run", runId), ("$workspace", workspace), ("$name", check.Name), ("$definition", Json.Write(check)),
                ("$fingerprint", fingerprint), ("$evidence", evidenceId));
        }

        public bool HasBuild(string runId, string workspace, CheckCommand check, string fingerprint)
        {
            using var db = Open(); using var command = db.CreateCommand();
            command.CommandText = "SELECT COUNT(*) FROM build_evidence WHERE run_id=$run AND workspace=$workspace AND check_name=$name AND definition=$definition AND fingerprint=$fingerprint";
            command.Parameters.AddWithValue("$run", runId); command.Parameters.AddWithValue("$workspace", workspace);
            command.Parameters.AddWithValue("$name", check.Name); command.Parameters.AddWithValue("$definition", Json.Write(check));
            command.Parameters.AddWithValue("$fingerprint", fingerprint);
            return Convert.ToInt64(command.ExecuteScalar()) == 1;
        }
    public Artifact[] Artifacts(string runId)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = "SELECT id,run_id,task_id,kind,path,sha256,at FROM artifacts WHERE run_id=$run ORDER BY at DESC LIMIT 1000";
        command.Parameters.AddWithValue("$run", runId);
        using var reader = command.ExecuteReader();
        var result = new List<Artifact>();
        while (reader.Read()) result.Add(new Artifact(reader.GetString(0), reader.GetString(1),
            reader.IsDBNull(2) ? null : reader.GetString(2), reader.GetString(3), reader.GetString(4),
            reader.GetString(5), DateTimeOffset.Parse(reader.GetString(6))));
        return result.ToArray();
    }

    public PlanHistory[] PlanHistory(string runId)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT a.id,a.task_id,a.kind,a.at,MAX(CASE WHEN e.kind IN ('PlanStepCompleted','ReviewRepairStepCompleted') THEN e.at END),MAX(e.role)
            FROM artifacts a LEFT JOIN events e ON e.run_id=a.run_id AND e.plan_id=a.id
            WHERE a.run_id=$run AND a.kind IN ('ImplementationPlan','TestPlan','ReviewRepairPlan')
            GROUP BY a.id ORDER BY a.at DESC
            """;
        command.Parameters.AddWithValue("$run", runId);
        using var reader = command.ExecuteReader();
        var plans = new List<PlanHistory>();
        while (reader.Read()) plans.Add(new(reader.GetString(0), reader.GetString(1),
            reader.GetString(2) == "ReviewRepairPlan" ? (reader.IsDBNull(5) ? "Review repair" : reader.GetString(5))
                : reader.GetString(2) == "TestPlan" ? "Tester" : "SpecImplementer", DateTimeOffset.Parse(reader.GetString(3)),
            reader.IsDBNull(4) ? null : DateTimeOffset.Parse(reader.GetString(4))));
        return plans.ToArray();
    }

    public AuditEvent[] DashboardEvents(string runId, string[] testNames)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT id,run_id,task_id,kind,message,document_id,at,assignment_id,role,plan_id,step_id,attempt FROM (
                SELECT *,ROW_NUMBER() OVER (
                    PARTITION BY CASE WHEN kind='CommandFinished' THEN 'latest-test' ELSE COALESCE(task_id,'')||':'||kind END
                    ORDER BY id DESC) AS position
                FROM events WHERE run_id=$run AND (
                    kind IN ('PairAssigned','PairVerified','TaskIntegrated','PairRepairRequired','WorkerProtocolBlocked','WorkspaceBlocked','WorkerContractBlocked','PreparationRepairRequired','ReviewHumanRequired',
                        'IntegrationRejected','RunBlocked','RunPaused','PauseRequested','RunLimitReached','AgentStarted')
                    OR (kind='CommandFinished' AND EXISTS (SELECT 1 FROM json_each($tests)
                        WHERE substr(message,1,length(value)+2)=value||': ')))
            ) WHERE position=1 ORDER BY id DESC
            """;
        command.Parameters.AddWithValue("$run", runId);
        command.Parameters.AddWithValue("$tests", Json.Write(testNames));
        return ReadEvents(command);
    }

    public bool HasStepEvidence(string runId, string taskId, StepAssignment assignment, string evidenceId)
        => HasObservation(runId, taskId, assignment.AssignmentId, assignment.PlanId, assignment.CurrentStep.Id, evidenceId, allowFailure: true);

    public bool HasObservation(string runId, string taskId, string assignmentId, string planId, string stepId, string evidenceId, bool allowFailure = false)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT EXISTS(SELECT 1 FROM events e JOIN artifacts a ON a.id=e.document_id
                WHERE e.run_id=$run AND e.task_id=$task AND e.assignment_id=$assignment AND e.plan_id=$plan AND e.step_id=$step
                AND a.id=$evidence AND a.run_id=$run AND a.task_id=$task
                AND (a.kind IN ('RepositoryEvidence','CommandEvidence') OR ($failure=1 AND a.kind='ToolFailure')))
            """;
        command.Parameters.AddWithValue("$run", runId);
        command.Parameters.AddWithValue("$task", taskId);
        command.Parameters.AddWithValue("$assignment", assignmentId);
        command.Parameters.AddWithValue("$plan", planId);
        command.Parameters.AddWithValue("$step", stepId);
        command.Parameters.AddWithValue("$evidence", evidenceId);
        command.Parameters.AddWithValue("$failure", allowFailure ? 1 : 0);
        return Convert.ToInt64(command.ExecuteScalar()) != 0;
    }

    public (AuditEvent[] Events, bool Truncated) TaskExecutionTrace(string runId, string taskId)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT id,run_id,task_id,kind,message,document_id,at,assignment_id,role,plan_id,step_id,attempt FROM events
            WHERE run_id=$run AND task_id=$task AND kind IN
                ('WorkspaceIntent','PlanStepCompleted','LogicalCommit','TddRedConfirmed','TddException','TaskSynchronized','CommandFinished','FileWritten')
            ORDER BY id DESC LIMIT 201
            """;
        command.Parameters.AddWithValue("$run", runId); command.Parameters.AddWithValue("$task", taskId);
        var events = ReadEvents(command);
        return (events.Take(200).OrderBy(e => e.Id).ToArray(), events.Length > 200);
    }

    public (AuditEvent[] Events, bool Truncated) StepObservations(string runId, string taskId, StepAssignment execution)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT id,run_id,task_id,kind,message,document_id,at,assignment_id,role,plan_id,step_id,attempt FROM events
            WHERE run_id=$run AND task_id=$task AND assignment_id=$assignment AND plan_id=$plan AND step_id=$step
                AND document_id IS NOT NULL AND kind IN ('CommandFinished','FileRead','FileWritten','FileDeleted','DiffRead','WorkingDiffRead','FilesListed','StepCommitObserved')
            ORDER BY id DESC LIMIT 101
            """;
        command.Parameters.AddWithValue("$run", runId); command.Parameters.AddWithValue("$task", taskId);
        command.Parameters.AddWithValue("$assignment", execution.AssignmentId); command.Parameters.AddWithValue("$plan", execution.PlanId);
        command.Parameters.AddWithValue("$step", execution.CurrentStep.Id);
        var events = ReadEvents(command);
        return (events.Take(100).OrderBy(e => e.Id).ToArray(), events.Length > 100);
    }

    public AuditEvent[] Events(string runId, string? taskId = null, long before = long.MaxValue, int limit = 100, string? kind = null)
    {
        using var db = Open();
        using var command = db.CreateCommand();
        command.CommandText = """
            SELECT id,run_id,task_id,kind,message,document_id,at,assignment_id,role,plan_id,step_id,attempt FROM events
            WHERE run_id=$run AND ($task IS NULL OR task_id=$task) AND ($kind IS NULL OR kind=$kind) AND id<$before ORDER BY id DESC LIMIT $limit
            """;
        command.Parameters.AddWithValue("$run", runId);
        command.Parameters.AddWithValue("$task", (object?)taskId ?? DBNull.Value);
        command.Parameters.AddWithValue("$kind", (object?)kind ?? DBNull.Value);
        command.Parameters.AddWithValue("$before", before);
        command.Parameters.AddWithValue("$limit", Math.Clamp(limit, 1, 200));
        return ReadEvents(command);
    }

    private static AuditEvent[] ReadEvents(SqliteCommand command)
    {
        using var reader = command.ExecuteReader();
        var events = new List<AuditEvent>();
        while (reader.Read()) events.Add(new AuditEvent(reader.GetInt64(0), reader.GetString(1),
            reader.IsDBNull(2) ? null : reader.GetString(2), reader.GetString(3), reader.GetString(4),
            reader.IsDBNull(5) ? null : reader.GetString(5), DateTimeOffset.Parse(reader.GetString(6)),
            reader.IsDBNull(7) ? null : reader.GetString(7), reader.IsDBNull(8) ? null : reader.GetString(8),
            reader.IsDBNull(9) ? null : reader.GetString(9), reader.IsDBNull(10) ? null : reader.GetString(10),
            reader.IsDBNull(11) ? null : reader.GetInt32(11)));
        return events.ToArray();
    }

    private static void Append(SqliteConnection db, SqliteTransaction? transaction, string runId,
        string? taskId, string kind, string message, string? documentId) => Execute(db, transaction,
        """
        INSERT INTO events(run_id,task_id,kind,message,document_id,at,assignment_id,role,plan_id,step_id,attempt)
        VALUES ($run,$task,$kind,$message,$doc,$at,$assignment,$role,$plan,$step,$attempt)
        """,
        ("$run", runId), ("$task", taskId), ("$kind", kind), ("$message", Redaction.Apply(message)),
        ("$doc", documentId), ("$at", DateTimeOffset.UtcNow.ToString("O")),
        ("$assignment", AuditScope.Current.AssignmentId), ("$role", AuditScope.Current.Role),
        ("$plan", AuditScope.Current.PlanId), ("$step", AuditScope.Current.StepId), ("$attempt", AuditScope.Current.Attempt));

    private static void Execute(SqliteConnection db, SqliteTransaction? transaction, string sql,
        params (string Name, object? Value)[] parameters)
    {
        using var command = db.CreateCommand();
        command.Transaction = transaction;
        command.CommandText = sql;
        foreach (var (name, value) in parameters) command.Parameters.AddWithValue(name, value ?? DBNull.Value);
        command.ExecuteNonQuery();
    }
}
