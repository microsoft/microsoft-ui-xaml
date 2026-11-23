'use strict';
const fragment = new URLSearchParams(location.hash.slice(1));
if (fragment.has('token')) {
  sessionStorage.setItem('sdlc-token', fragment.get('token'));
  history.replaceState(null, '', location.pathname);
}
const token = sessionStorage.getItem('sdlc-token');
let runId = null, taskId = null, revision = -1, runOffset = 0, eventList = [], older = false, documentId = null;
let interactionKey = null, actionPending = false, noticeRunId = null;
const feedbackDrafts = new Map();
const unblockDrafts = new Map(), executionViews = new Map();
let unblockKey = null;
const documentCache = new Map(), expanded = new Set(), collapsed = new Set(), branchViews = new Map(), planViews = new Map();
let approvedKey = null, focusedEvent = null, historyGeneration = 0;
const $ = id => document.getElementById(id);
const node = (tag, text, className) => {
  const element = document.createElement(tag);
  if (text !== undefined) element.textContent = text;
  if (className) element.className = className;
  return element;
};
const button = (text, action) => { const b = node('button', text); b.addEventListener('click', action); return b; };
async function api(path, body) {
  const options = { headers: { Authorization: 'Bearer ' + token } };
  if (body !== undefined) {
    options.method = 'POST'; options.headers['Content-Type'] = 'application/json'; options.body = JSON.stringify(body);
  }
  const response = await fetch('/api/' + path, options);
  if (!response.ok) throw new Error(response.status === 401 ? 'Authentication required. Open the link printed by SDLCAgent viewer.' : await response.text());
  return response.json();
}
function showError(error) { $('error').hidden = false; $('error').textContent = error.message; }
async function protect(action, clearError = true) { try { await action(); if (clearError) $('error').hidden = true; } catch (error) { showError(error); } }
function tree(value, label = 'Document', depth = 0) {
  if (value === null || typeof value !== 'object') {
    const container = node('div'); container.append(node('strong', label + ': '), node('pre', String(value)));
    return container;
  }
  const details = node('details'); details.open = depth < 1;
  details.append(node('summary', label + (Array.isArray(value) ? ' (' + value.length + ')' : '')));
  for (const [key, child] of Object.entries(value)) details.append(tree(child, key, depth + 1));
  return details;
}
async function showDocument(id) {
  documentId = id;
  const selectedRunId = runId, value = await readDocument(id);
  if (selectedRunId !== runId) return;
  $('detail').replaceChildren(node('h3', 'Document ' + id), tree(value));
  $('logs').open = true; $('detail').scrollIntoView({ block: 'nearest' });
}
function readDocument(id) {
  if (!documentCache.has(id)) documentCache.set(id, api('artifacts/' + encodeURIComponent(id)).catch(error => { documentCache.delete(id); throw error; }));
  return documentCache.get(id);
}
function collapsible(key, title, initiallyOpen = false) {
  const details = node('details'); details.open = expanded.has(key) || (initiallyOpen && !collapsed.has(key)); details.append(node('summary', title));
  details.addEventListener('toggle', () => { if (details.isConnected) { if (details.open) { expanded.add(key); collapsed.delete(key); } else { expanded.delete(key); collapsed.add(key); } } });
  return details;
}
function lazyDocument(id, title, active) {
  if (planViews.has(id)) {
    const saved = planViews.get(id); saved.querySelector('summary').textContent = title; saved.classList.toggle('plan-active', active); return saved;
  }
  const details = collapsible('document:' + id, title), content = node('div');
  if (active) details.classList.add('plan-active'); details.append(content);
  let loaded = false;
  async function load() {
    if (!details.open || loaded) return;
    loaded = true;
    try { content.replaceChildren(tree(await readDocument(id), 'Saved plan')); }
    catch (error) { loaded = false; showError(error); }
  }
  details.addEventListener('toggle', load); if (details.open) load();
  planViews.set(id, details);
  return details;
}
async function selectRun(id) {
  runId = id; taskId = null; revision = -1; older = false; documentId = null; focusedEvent = null; historyGeneration++;
  interactionKey = null; unblockKey = null; approvedKey = null; $('interaction').hidden = true; $('unblock').hidden = true; $('action-status').hidden = noticeRunId !== runId;
  eventList = []; $('older-events').hidden = true; $('overview').replaceChildren(node('h2', 'Loading run...'));
  for (const id of ['detail', 'events', 'documents', 'branches', 'execution-commits', 'approved-content']) $(id).replaceChildren();
  await loadRun(); await loadEvents();
}
async function loadRuns() {
  const runs = await api('runs?offset=' + runOffset);
  if (!runOffset) $('runs').replaceChildren();
  for (const run of runs) {
    const b = button(run.request.branch + ' - ' + run.state + '\n' + run.id.slice(0, 12), () => protect(() => selectRun(run.id)));
    b.dataset.runId = run.id;
    if (run.id === runId) b.classList.add('selected');
    $('runs').append(b);
  }
  $('more-runs').hidden = runs.length < 100;
  if (!runId && runs.length) await selectRun(runs[0].id);
}
async function loadRun() {
  if (!runId) return;
  const selectedRunId = runId;
  const data = await api('runs/' + encodeURIComponent(selectedRunId) + '/dashboard');
  if (selectedRunId !== runId) return;
  const run = data.run;
  if (run.revision < revision) return;
  $('freshness').textContent = 'Refreshed ' + new Date().toLocaleTimeString() + ' | last state update ' + new Date(run.updated).toLocaleTimeString();
  revision = run.revision;
  for (const entry of $('runs').querySelectorAll('button[data-run-id]')) {
    entry.classList.toggle('selected', entry.dataset.runId === run.id);
    if (entry.dataset.runId === run.id) entry.textContent = run.request.branch + ' - ' + run.state + '\n' + run.id.slice(0, 12);
  }
  const overview = $('overview'); overview.replaceChildren(node('h2', 'State: ' + run.state, 'status-badge status-' + run.state.toLowerCase()));
  overview.append(node('p', run.request.repository + ' | ' + run.request.branch), node('code', run.id));
  const problem = node('details'); problem.append(node('summary', 'Original task / problem statement'), node('pre', run.request.problem)); overview.append(problem);
  const metrics = node('div', undefined, 'metrics');
  for (const [label, value] of Object.entries({ 'Items complete': data.itemCompleteCount + '/' + data.totalItemCount, Phase: run.phase,
    'Implementer/tester pairs running': data.activePairs.length, 'Tests passing (latest suite)': data.tests.counts?.passed ?? 'Unknown',
    'Tests implemented (latest suite)': data.tests.counts?.total ?? 'Unknown', 'Total tests (approved target)': data.plannedTestCount ?? 'Not recorded',
    'Success metrics': run.state === 'Completed' ? 'Met the goal' : 'Yet to meet the goal',
    'Managed branch': data.branch.branch, 'Branch total commits': data.branch.totalCommits ?? 'Unavailable', 'Commits added during run': data.branch.addedCommits ?? 'Unavailable',
    'Active time': Math.round(run.activeSeconds) + 's', 'Reported tokens (partial, millions)': run.reportedTokens == null ? 'Unknown' : (run.reportedTokens / 1000000).toLocaleString(undefined, { minimumFractionDigits: 3, maximumFractionDigits: 6 }) + ' M' }))
    metrics.append(node('div', label + ': ' + value, 'metric'));
  overview.append(metrics);
  if (run.failure || ['Blocked', 'Paused', 'PauseRequested'].includes(run.state)) {
    const attention = node('div', undefined, 'attention'); attention.append(node('strong', run.state + ': '), node('pre', run.failure || data.attentionEvent?.message || 'Operator control requested.'));
    if (data.attentionEvent) {
      const link = node('a', 'View relevant blocking/pause log'); link.href = '#logs'; link.id = 'attention-log';
      link.addEventListener('click', event => { event.preventDefault(); protect(() => focusEvent(data.attentionEvent)); }); attention.append(link);
    }
    overview.append(attention);
  }
  if (data.branch.error) overview.append(node('pre', 'Git status unavailable: ' + data.branch.error, 'failed'));
  const testInfo = node('p', data.tests.description + (data.tests.at ? ' At ' + new Date(data.tests.at).toLocaleString() + ', ' + (data.tests.taskId || 'integration/baseline') + ', revision ' + data.tests.revision + ', exit ' + data.tests.exitCode + (data.tests.timedOut ? ' (timed out)' : '') : ''), 'muted');
  overview.append(testInfo);
  if (data.tests.evidenceId) overview.append(button('Open test evidence', () => protect(() => showDocument(data.tests.evidenceId))));
  if (run.finalCommit) overview.append(node('p', 'Verified final commit: ' + run.finalCommit, 'passed'));
  if (run.specificationId) { const current = button('Current specification', () => protect(() => showDocument(run.specificationId))); current.id = 'current-spec'; overview.append(current); }
  if (run.state === 'AwaitingApproval') overview.append(node('p', 'Human input required. Review the specification and respond below. Implementation cannot start before approval.', 'warning'));
  if (['Running', 'Queued', 'AwaitingApproval'].includes(run.state)) overview.append(button('Pause run', () => protect(() => control(run.id, 'pause', {}, 'Pause requested. In-flight work will drain to a durable checkpoint.'))));
  if (run.state === 'Paused') overview.append(button('Resume run', () => protect(() => control(run.id, 'resume', {}, 'Resume requested. The host will continue from durable progress.'))));
  overview.append(node('h3', 'Running implementer / tester pairs'));
  const active = node('div'); active.id = 'active-pairs';
  if (!data.activePairs.length) active.append(node('p', 'No pairs currently running.'));
  for (const pair of data.activePairs) active.append(node('p', pair.taskId + ' - ' + pair.title + ' | Started: ' + (pair.startedAt ? new Date(pair.startedAt).toLocaleString() : 'Not recorded') + ' | Current role: ' + (pair.role || 'Assigning') + ' | Stage: ' + pair.stage + ' | Branch: ' + (pair.branch || 'Creating')));
  overview.append(active, node('h3', 'Backlog not yet scheduled'));
  const backlog = node('ul', undefined, 'backlog'); backlog.id = 'backlog';
  for (const task of data.backlog) backlog.append(node('li', task.id + ' - ' + task.title + (task.dependencies.length ? ' | Waiting on: ' + task.dependencies.join(', ') : ' | Independent')));
  overview.append(data.backlog.length ? backlog : node('p', 'No unscheduled backlog items.'));
  const items = collapsible('items:' + run.id, 'All work items and dependencies');
  const table = node('table'); const heading = node('tr');
  for (const text of ['Task / criteria', 'State / current stage', 'Dependencies', 'Attempts / heartbeat']) heading.append(node('th', text));
  table.append(heading);
  for (const task of run.tasks) {
    const row = node('tr'), cell = node('td');
    cell.append(button(task.definition.title, () => protect(() => filterTask(task.definition.id))));
    cell.append(node('div', task.definition.id + ': ' + task.definition.criterionIds.join(', ')));
    row.append(cell, node('td', task.state + ' / ' + task.stage + ' step cursor ' + task.stepIndex), node('td', task.definition.dependencies.join(', ') || 'Independent'));
    const age = task.heartbeat ? Math.round((Date.now() - Date.parse(task.heartbeat)) / 1000) : null;
    row.append(node('td', task.attempts + ' attempts | heartbeat ' + (age === null ? 'not started' : age + 's ago'), age > 60 && task.state === 'Running' ? 'warning' : ''));
    table.append(row);
  }
  items.append(table); overview.append(items);
  $('documents').replaceChildren();
  if (!data.plans.length) $('documents').append(node('p', 'No worker plans saved yet.'));
  for (const entry of data.plans) {
    const plan = entry.plan, title = (entry.active ? 'ACTIVE | ' : entry.current ? 'Current | ' : 'Historical | ') + plan.role + ' | ' + plan.taskId + ' | ' + (entry.finishedAt ? 'Finished/last activity: ' + new Date(entry.finishedAt).toLocaleString() : 'Created: ' + new Date(plan.created).toLocaleString());
    $('documents').append(lazyDocument(plan.id, title, entry.active));
  }
  $('branches').replaceChildren(branchView(run, null, run.request.branch));
  for (const task of run.tasks.filter(t => t.branch)) $('branches').append(branchView(run, task.definition.id, task.branch));
  loadUnblock(run);
  await loadExecutionCommits(run);
  await loadApproved(run);
  await loadInteraction(run);
}
function loadUnblock(run) {
  const panel = $('unblock'); panel.dataset.revision = run.revision;
  const drained = !run.tasks.some(t => ['Running', 'Integrating'].includes(t.state));
  const key = run.id + ':' + run.state + ':' + run.pendingUnblockId + ':' + drained;
  if (unblockKey === key) return;
  unblockKey = key; panel.hidden = true;
  if (run.pendingUnblockId) {
    panel.replaceChildren(node('h2', 'Orchestrator is assessing your unblock instructions'), node('p', 'No worker execution resumes until this read-only assessment accepts the guidance.'));
    panel.hidden = false; return;
  }
  if (run.state !== 'Blocked') return;
  const label = node('label', 'Instructions for the orchestrator'), input = node('textarea');
  input.id = 'unblock-instruction'; label.htmlFor = input.id; input.maxLength = 16000;
  input.value = unblockDrafts.get(run.id) || '';
  const send = button('Send unblock instructions', () => protect(async () => {
    if (!input.value.trim()) throw new Error('Enter instructions before sending.');
    await control(run.id, 'unblock', { revision: Number(panel.dataset.revision), instruction: input.value }, 'Instructions sent to the orchestrator for assessment.');
  }));
  send.id = 'submit-unblock';
  function update() { input.disabled = !drained || actionPending; send.disabled = !drained || !input.value.trim() || actionPending; }
  input.addEventListener('input', () => { unblockDrafts.set(run.id, input.value); update(); });
  panel.replaceChildren(node('h2', 'Unblock this run'), node('p', 'Describe the correction or remediation. The orchestrator assesses it before continuing existing work. Approval, task scopes and validation gates remain unchanged. Requirement changes belong in specification feedback.'), label, input, send);
  if (!drained) panel.append(node('p', 'Wait for active work to drain before submitting.', 'warning'));
  update(); panel.hidden = false;
}
async function loadExecutionCommits(run) {
  if (!executionViews.has(run.id)) {
    const view = node('div'), content = node('div'), more = button('More execution commits', () => protect(() => load(false)));
    const commits = new Map(); let before = null, busy = false, paged = false;
    view.append(content, more); more.hidden = true;
    async function load(latest) {
      if (busy) return;
      busy = true;
      try {
        const history = await api('runs/' + run.id + '/execution-commits' + (!latest && before !== null ? '?before=' + before : ''));
        for (const commit of history.commits) commits.set(commit.eventId, commit);
        if (!latest || !paged) before = history.nextBefore;
        if (!latest) paged = true;
        content.replaceChildren();
        for (const commit of Array.from(commits.values()).sort((a, b) => b.eventId - a.eventId))
          content.append(node('p', commit.hash + ' | ' + (commit.taskId || 'Run') + ' | ' + (commit.role || 'Role not recorded') + ' | ' + new Date(commit.at).toLocaleString() + '\n' + commit.subject));
        if (!commits.size) content.append(node('p', 'No commits recorded in this run yet.'));
        more.hidden = before === null;
      } finally { busy = false; }
    }
    executionViews.set(run.id, { view, load });
  }
  const saved = executionViews.get(run.id); $('execution-commits').replaceChildren(saved.view);
  await saved.load(true);
}
async function loadApproved(run) {
  const key = run.id + ':' + run.approvedSpecificationId;
  if (approvedKey === key) return;
  approvedKey = key;
  if (!run.approvedSpecificationId) { $('approved-content').textContent = 'No specification has been approved yet. Review the current draft and questions below.'; return; }
  try {
    const spec = await readDocument(run.approvedSpecificationId);
    if (runId === run.id && approvedKey === key) $('approved-content').replaceChildren(node('p', 'Approved version: ' + run.approvedSpecificationId + ' | SHA256: ' + run.approvedSpecificationHash), tree(spec, spec.title));
  } catch (error) { if (approvedKey === key) approvedKey = null; throw error; }
}
function branchView(run, task, branch) {
  const key = run.id + ':' + (task || 'integration');
  if (branchViews.has(key)) return branchViews.get(key);
  const details = node('details'); details.append(node('summary', branch + (task ? ' | ' + task + ' | commits since assigned base' : ' | full managed-branch history')));
  const content = node('div'), more = button('More commits', () => protect(() => load(false))), refresh = button('Refresh commits', () => protect(() => load(true)));
  let offset = 0, loaded = false, busy = false;
  details.append(refresh, content, more); more.hidden = true;
  async function load(reset) {
    if (busy) return;
    busy = true;
    try {
      const history = await api('runs/' + run.id + '/commits?offset=' + (reset ? 0 : offset) + (task ? '&taskId=' + encodeURIComponent(task) : ''));
      if (reset) content.replaceChildren();
      for (const commit of history.commits) content.append(node('p', commit.hash + ' | ' + (commit.role || 'Role not recorded') + ' | ' + new Date(commit.at).toLocaleString() + '\n' + commit.subject));
      if (!history.commits.length && reset) content.append(node('p', 'No commits in this branch range yet.'));
      offset = history.nextOffset; more.hidden = offset === null; loaded = true;
    } finally { busy = false; }
  }
  details.addEventListener('toggle', () => { if (details.open && !loaded) protect(() => load(true)); });
  branchViews.set(key, details); return details;
}
async function control(id, action, body, message) {
  if (actionPending) return;
  actionPending = true;
  document.querySelectorAll('#interaction button, #interaction input, #interaction textarea, #unblock button, #unblock textarea, #overview button').forEach(input => { input.disabled = true; });
  try {
    await api('runs/' + encodeURIComponent(id) + '/' + action, body);
    if (action === 'feedback') feedbackDrafts.delete(id + ':' + body.version);
    if (action === 'unblock') unblockDrafts.delete(id);
    noticeRunId = id; $('action-status').textContent = message; $('action-status').hidden = runId !== id;
  } finally {
    actionPending = false; revision = -1; interactionKey = null; unblockKey = null;
    await loadRun(); await loadEvents();
  }
}
async function loadInteraction(run) {
  if (runId !== run.id) return;
  const key = run.id + ':' + run.specificationId + ':' + run.state;
  if (interactionKey === key || actionPending) return;
  interactionKey = key;
  const panel = $('interaction'); panel.hidden = true;
  if (!run.specificationId || !['AwaitingApproval', 'Paused', 'Blocked'].includes(run.state)) return;
  let spec;
  try { spec = await api('artifacts/' + encodeURIComponent(run.specificationId)); }
  catch (error) { if (interactionKey === key) interactionKey = null; throw error; }
  if (runId !== run.id || interactionKey !== key) return;
  panel.replaceChildren(node('h2', 'Review specification: ' + spec.title), node('p', 'Version: ' + run.specificationId));
  panel.append(tree(spec, 'Full specification (scope, approach, and non-goals)'));
  panel.append(node('h3', 'Success criteria and measurements'));
  for (const criterion of spec.criteria)
    panel.append(node('p', criterion.id + ': ' + criterion.outcome), node('pre', criterion.measurement + '\nChecks: ' + criterion.checks.join(', ')));
  const policy = node('details'); policy.append(node('summary', 'Execution policy approved with this specification'), node('p', 'Policy hash: ' + run.policyHash), node('pre', run.policyJson)); panel.append(policy);
  const draftKey = run.id + ':' + run.specificationId;
  if (!feedbackDrafts.has(draftKey)) feedbackDrafts.set(draftKey, { answers: spec.questions.map(() => ''), feedback: '' });
  const draft = feedbackDrafts.get(draftKey);
  const questions = node('div'); questions.id = 'spec-questions';
  questions.append(node('h3', spec.questions.length ? 'Questions requiring your input' : 'No outstanding questions'));
  const inputs = [];
  for (const [i, question] of spec.questions.entries()) {
    const container = node('div', undefined, 'question'), label = node('label', (i + 1) + '. ' + question), input = node('textarea');
    input.id = 'spec-answer-' + i; label.htmlFor = input.id; input.value = draft.answers[i];
    input.addEventListener('input', () => { draft.answers[i] = input.value; updateButtons(); });
    container.append(label, input); questions.append(container); inputs.push(input);
  }
  panel.append(questions);
  const label = node('label', 'Additional feedback or requested changes'), feedback = node('textarea');
  feedback.id = 'spec-feedback'; label.htmlFor = feedback.id; feedback.value = draft.feedback;
  feedback.addEventListener('input', () => { draft.feedback = feedback.value; updateButtons(); }); inputs.push(feedback);
  panel.append(label, feedback);
  const send = button('Send feedback / answers', () => protect(async () => {
    const answers = spec.questions.flatMap((question, i) => draft.answers[i].trim() ? ['Question: ' + question + '\nAnswer: ' + draft.answers[i].trim()] : []);
    if (draft.feedback.trim()) answers.push(draft.feedback.trim());
    if (!answers.length) throw new Error('Enter an answer or feedback before sending.');
    await control(run.id, 'feedback', { version: run.specificationId, feedback: answers.join('\n\n') }, 'Feedback sent. SpecCreator will revise the specification and request review again.');
  }));
  send.id = 'submit-feedback'; panel.append(send);
  const confirmation = node('label', undefined, 'review-confirmation'), reviewed = node('input');
  reviewed.type = 'checkbox'; reviewed.id = 'review-spec';
  confirmation.append(reviewed, document.createTextNode('I reviewed this exact specification, its success criteria, and execution policy. Start autonomous implementation.'));
  const approve = button('Approve and start implementation', () => protect(async () => {
    if (!reviewed.checked || spec.questions.length || hasFeedback()) throw new Error('Resolve questions, submit any draft feedback, and confirm review before approval.');
    await control(run.id, 'approve', { version: run.specificationId }, 'Approved the displayed specification version. Autonomous planning and implementation can now proceed.');
  }));
  approve.id = 'approve-spec'; reviewed.addEventListener('change', updateButtons);
  panel.append(confirmation, approve, node('p', spec.questions.length ? 'Answer questions and send feedback first. Approval remains disabled until a revised specification has no outstanding questions.' : 'Feedback creates a new version that must be reviewed again. Approval applies only to the version shown above.', 'muted'));
  const drained = !run.tasks.some(t => ['Running', 'Integrating'].includes(t.state));
  const canApprove = run.state === 'AwaitingApproval' && !spec.questions.length;
  function hasFeedback() { return draft.feedback.trim() || draft.answers.some(answer => answer.trim()); }
  function updateButtons() {
    send.disabled = !drained || !hasFeedback() || actionPending;
    reviewed.disabled = !canApprove || !!hasFeedback() || actionPending;
    approve.disabled = !canApprove || !reviewed.checked || !!hasFeedback() || actionPending;
    for (const input of inputs) input.disabled = !drained || actionPending;
  }
  updateButtons(); panel.hidden = false;
}
async function loadEvents(append = false) {
  if (!runId) return;
  const selectedRunId = runId, selectedTaskId = taskId, generation = historyGeneration;
  const before = append && eventList.length ? eventList[eventList.length - 1].id : '9223372036854775807';
  const events = await api('runs/' + selectedRunId + '/events?limit=' + (append ? 200 : 25) + '&before=' + before + '&taskId=' + encodeURIComponent(selectedTaskId || ''));
  if (selectedRunId !== runId || selectedTaskId !== taskId || generation !== historyGeneration) return false;
  eventList = append ? [...eventList, ...events] : events;
  $('event-context').textContent = (taskId || 'Entire run') + (older ? ' | complete history: ' + eventList.length + ' records loaded' : ' | recent 25 records, live');
  renderEvents();
  return events.length === (append ? 200 : 25);
}
async function filterTask(id) {
  taskId = id; older = false; focusedEvent = null; historyGeneration++;
  $('older-events').hidden = true;
  $('logs').open = true; await loadEvents(); $('logs').scrollIntoView({ block: 'start' });
}
async function focusEvent(event) {
  focusedEvent = event; taskId = null; older = false; historyGeneration++; $('event-filter').value = ''; $('logs').open = true; await loadEvents();
  const target = document.getElementById('event-' + event.id);
  for (let current = target; current; current = current.parentElement) if (current.tagName === 'DETAILS') current.open = true;
  target?.scrollIntoView({ block: 'center' });
}
async function completeHistory() {
  older = true; const generation = ++historyGeneration; $('older-events').hidden = false;
  try {
    let more = eventList.length === 25 || eventList.length >= 200;
    while (more && generation === historyGeneration) more = await loadEvents(true);
    if (generation === historyGeneration) $('event-context').textContent = (taskId || 'Entire run') + ' | complete structured history: ' + eventList.length + ' records (snapshot; Latest events resumes live updates)';
  } finally { if (generation === historyGeneration) $('older-events').hidden = true; }
}
function renderEvents() {
  const filter = $('event-filter').value.toLowerCase(); $('events').replaceChildren();
  const groups = new Map(), roles = new Map(), steps = new Map();
  const records = focusedEvent && !eventList.some(e => e.id === focusedEvent.id) ? [focusedEvent, ...eventList] : eventList;
  if (!taskId) $('events').append(button('Filter by work item using the status table above', () => { document.querySelector('#overview details:last-child')?.setAttribute('open', ''); $('overview').scrollIntoView(); }));
  else $('events').append(button('All work items', () => protect(() => filterTask(null))));
  for (const event of records.filter(e => (e.kind + ' ' + e.message).toLowerCase().includes(filter))) {
    const groupKey = event.runId + ':' + (event.taskId || 'orchestrator');
    if (!groups.has(groupKey)) { const group = collapsible('log-task:' + groupKey, event.taskId || 'Orchestrator / run lifecycle', true); groups.set(groupKey, group); $('events').append(group); }
    const roleKey = groupKey + ':' + (event.assignmentId || 'run') + ':' + (event.role || 'Orchestrator');
    if (!roles.has(roleKey)) { const group = collapsible('log-role:' + roleKey, (event.role || 'Orchestrator') + (event.attempt ? ' | attempt ' + event.attempt : ''), true); roles.set(roleKey, group); groups.get(groupKey).append(group); }
    const stepKey = roleKey + ':' + (event.planId || '') + ':' + (event.stepId || 'lifecycle');
    if (!steps.has(stepKey)) { const group = collapsible('log-step:' + stepKey, event.stepId ? 'Plan step: ' + event.stepId : 'Activity / checkpoints', true); steps.set(stepKey, group); roles.get(roleKey).append(group); }
    const entry = collapsible('event:' + event.id, event.kind + ' | ' + new Date(event.at).toLocaleString() + ' | ' + event.message.slice(0, 100));
    entry.id = 'event-' + event.id; entry.classList.add('log-event'); entry.append(node('pre', event.message));
    if (event.planId) entry.append(button('Open step plan', () => protect(() => showDocument(event.planId))));
    if (event.documentId) entry.append(button('Drill into evidence', () => protect(() => showDocument(event.documentId))));
    steps.get(stepKey).append(entry);
  }
  if (!records.length) $('events').append(node('p', 'No log entries for this selection yet.'));
}
$('refresh').addEventListener('click', () => protect(async () => { runOffset = 0; revision = -1; await loadRuns(); await loadRun(); await loadEvents(); }));
$('more-runs').addEventListener('click', () => protect(async () => { runOffset += 100; await loadRuns(); }));
$('expand-history').addEventListener('click', () => protect(completeHistory));
$('older-events').addEventListener('click', () => { historyGeneration++; $('older-events').hidden = true; $('event-context').textContent += ' | loading stopped; click Expand complete structured history to continue'; });
$('latest-events').addEventListener('click', () => protect(async () => { historyGeneration++; older = false; focusedEvent = null; $('older-events').hidden = true; await loadEvents(); }));
$('event-filter').addEventListener('input', renderEvents);
$('settings').addEventListener('click', () => protect(async () => { documentId = null; $('detail').replaceChildren(tree(await api('config'), 'Effective host configuration')); }));
protect(loadRuns);
let refreshing = false;
setInterval(() => {
  if (refreshing) return;
  refreshing = true;
  protect(async () => { await loadRun(); if (!older) await loadEvents(); }, false).finally(() => { refreshing = false; });
}, 2500);
