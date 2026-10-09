// Weekly sync for .github/area-owners.yml.
//
// Finds drift between the map and the repository tree and proposes fixes as a single
// pull request (branch `bot/area-owners-sync`) for humans to review. It only edits
// *paths* - it never adds, removes or changes reviewers.
//
//   glob matches nothing, its folder moved     -> rewrite the glob     (medium)
//   glob matches nothing                       -> remove the glob      (high)
//   new folder, name/co-change points to area  -> add it to that area  (medium)
//   new folder, no clear owner                 -> park in UNASSIGNED   (needs a human)
//   UNASSIGNED path now owned by a real area   -> drop it from UNASSIGNED
//
// Usage:
//   node .github/scripts/sync-area-owners.mjs            CI: dry run unless AREA_OWNERS_SYNC_DRY_RUN=false
//   node .github/scripts/sync-area-owners.mjs --write    local: apply the proposal to the working map

import fs from 'node:fs';
import {
  MAP_PATH,
  UNASSIGNED,
  analyze,
  annotate,
  areasForFile,
  compileAreas,
  createLog,
  git,
  github,
  groupFiles,
  listFiles,
  mapMaintainers,
  parseMap,
  requestReviewers,
  readMapText,
  schemaErrors,
  stringifyMap,
  toRegExp,
  uniqueHandles,
} from './area-owners-lib.mjs';

const BRANCH = 'bot/area-owners-sync';
const TITLE = 'Sync area-owners.yml with the repository tree';
const COMMIT_MARKER = '[area-owners-sync]';
const CO_CHANGE_COMMITS = 200;

const { log, flush } = createLog();
const writeLocal = process.argv.includes('--write');
const dryRun = !writeLocal && (process.env.AREA_OWNERS_SYNC_DRY_RUN ?? 'true').toLowerCase() !== 'false';

// ------------------------------------------------------------------ analysis

function directories(files) {
  const dirs = new Set();
  for (const f of files) {
    const parts = f.split('/');
    for (let i = 1; i < parts.length; i++) dirs.add(parts.slice(0, i).join('/'));
  }
  return dirs;
}

// A dead `some/path/Name/**` glob whose folder now lives somewhere else under the same
// name (exactly one uncovered candidate) was most likely moved.
function findMove(glob, dirs) {
  const m = glob.match(/^((?:[^*?/]+\/)*)([^*?/]+)\/\*\*$/);
  if (!m) return null;
  const name = m[2].toLowerCase();
  const candidates = [...dirs].filter((d) => d.split('/').pop().toLowerCase() === name);
  return candidates.length === 1 ? `${candidates[0]}/**` : null;
}

// `dll-tabular` next to an existing `dll/**` glob, `idl-tabular` next to `idl/**`, ...
function suggestByName(group) {
  const leaf = group.dir.replace(/\/$/, '').split('/').pop();
  const tokens = new Set(leaf.toLowerCase().split(/[-_.]/).filter((t) => t.length > 1));
  const hits = group.siblings.filter((s) => tokens.has(s.name.toLowerCase()));
  const areaIds = [...new Set(hits.map((h) => h.area))].filter((id) => id !== UNASSIGNED);
  if (areaIds.length !== 1) return null;
  return {
    area: areaIds[0],
    confidence: 'medium',
    reason: `name matches sibling \`${hits[0].glob}\``,
  };
}

// Which sibling areas were changed in the same commits as this folder? A clear
// majority (>= 60% of votes, >= 3 commits) is taken as the likely owner.
function suggestByCoChange(group, compiled) {
  const candidates = new Set(group.siblings.map((s) => s.area).filter((id) => id !== UNASSIGNED));
  if (candidates.size === 0) return { suggestion: null, votes: [] };
  // A shallow clone's oldest commit looks like a root commit, whose "diff" lists the
  // whole tree; log.showRoot=false keeps it from voting for every area.
  const out = git([
    '-c',
    'log.showRoot=false',
    'log',
    '--full-diff',
    '--no-renames',
    '--name-only',
    '--format=%x01',
    '-n',
    String(CO_CHANGE_COMMITS),
    '--',
    group.dir,
  ]);
  const votes = new Map();
  for (const commit of out.split('\u0001').slice(1)) {
    const hit = new Set();
    for (const file of commit.split('\n').filter(Boolean)) {
      if (file.startsWith(group.dir)) continue;
      for (const area of areasForFile(compiled, file)) {
        if (candidates.has(area.id)) hit.add(area.id);
      }
    }
    for (const id of hit) votes.set(id, (votes.get(id) ?? 0) + 1);
  }
  const ranked = [...votes.entries()].sort((a, b) => b[1] - a[1]);
  const total = ranked.reduce((sum, [, n]) => sum + n, 0);
  const tally = ranked.map(([id, n]) => `${id} ${n}`);
  if (ranked.length > 0 && ranked[0][1] >= 3 && ranked[0][1] / total >= 0.6) {
    return {
      suggestion: {
        area: ranked[0][0],
        confidence: 'medium',
        reason: `changed together with ${ranked[0][0]} in ${ranked[0][1]} of the commits touching it (${tally.join(', ')})`,
      },
      votes: tally,
    };
  }
  return { suggestion: null, votes: tally };
}

function plan(data, files) {
  const compiled = compileAreas(data.areas);
  const { dead, uncovered } = analyze(compiled, files);
  const dirs = directories(uncovered);
  const changes = [];
  const flags = [];

  const remaining = new Map(data.areas.map((a) => [a.id, a.paths.length]));
  let stray = uncovered;

  for (const d of dead) {
    if (d.area === UNASSIGNED) {
      changes.push({ kind: 'remove', area: d.area, glob: d.glob, confidence: 'high', reason: 'matches nothing' });
      continue;
    }
    // Only follow a move when the new location is not owned yet.
    const moved = findMove(d.glob, dirs);
    const movedRe = moved && toRegExp(moved);
    if (moved && stray.some((f) => movedRe.test(f))) {
      stray = stray.filter((f) => !movedRe.test(f));
      changes.push({
        kind: 'rewrite',
        area: d.area,
        glob: d.glob,
        to: moved,
        confidence: 'medium',
        reason: 'folder appears to have moved',
      });
    } else if (remaining.get(d.area) > 1) {
      remaining.set(d.area, remaining.get(d.area) - 1);
      changes.push({ kind: 'remove', area: d.area, glob: d.glob, confidence: 'high', reason: 'matches nothing' });
    } else {
      flags.push(`\`${d.area}\`: \`${d.glob}\` matches nothing but is the area's last path - retire or re-scope the area.`);
    }
  }

  // UNASSIGNED globs whose files a real area now covers are no longer needed.
  const parked = compiled.find((a) => a.id === UNASSIGNED);
  if (parked) {
    const owned = compiled.filter((a) => a.id !== UNASSIGNED);
    for (const m of parked.matchers) {
      const matches = files.filter((f) => m.re.test(f));
      if (matches.length > 0 && matches.every((f) => areasForFile(owned, f).length > 0)) {
        changes.push({
          kind: 'remove',
          area: UNASSIGNED,
          glob: m.glob,
          confidence: 'high',
          reason: 'now covered by an owning area',
        });
      }
    }
  }

  for (const group of groupFiles(data.areas, stray)) {
    const isDir = group.glob.endsWith('/**');
    let suggestion = isDir ? suggestByName(group) : null;
    let votes = [];
    if (!suggestion && isDir) ({ suggestion, votes } = suggestByCoChange(group, compiled));
    if (suggestion) {
      changes.push({ kind: 'add', glob: group.glob, files: group.files.length, ...suggestion });
    } else {
      changes.push({
        kind: 'add',
        area: UNASSIGNED,
        glob: group.glob,
        files: group.files.length,
        confidence: 'needs a human',
        reason: votes.length > 0 ? `no clear owner (co-change: ${votes.join(', ')})` : 'no clear owner',
      });
    }
  }

  return { changes, flags };
}

// ------------------------------------------------------------------ editing

function areaNode(doc, id) {
  return doc.get('areas', true).items.find((n) => n.get('id') === id);
}

function pathIndex(paths, glob) {
  return paths.items.findIndex((n) => (n?.value ?? n) === glob);
}

function scalar(doc, glob) {
  const node = doc.createNode(glob);
  if (/^[*&!|>'"%@`{[?#,-]/.test(glob)) node.type = 'QUOTE_SINGLE';
  return node;
}

function ensureUnassigned(doc) {
  let node = areaNode(doc, UNASSIGNED);
  if (node) return node;
  node = doc.createNode({
    id: UNASSIGNED,
    name: 'Unassigned - needs an owning area',
    tier: 'Holding area',
    reviewers: [],
    paths: [],
  });
  node.get('reviewers', true).flow = true;
  node.commentBefore =
    ' -------------------------------------------------------------- Holding area\n' +
    ` Paths the weekly sync could not attribute. ${UNASSIGNED} requests nobody (CODEOWNERS\n` +
    ' applies); move each path into the area that should own it.';
  node.spaceBefore = true;
  doc.get('areas', true).items.push(node);
  return node;
}

function apply(doc, text, changes) {
  for (const c of changes) {
    if (c.kind === 'add') {
      const area = c.area === UNASSIGNED ? ensureUnassigned(doc) : areaNode(doc, c.area);
      area.get('paths', true).items.push(scalar(doc, c.glob));
    } else {
      const paths = areaNode(doc, c.area).get('paths', true);
      const i = pathIndex(paths, c.glob);
      if (i === -1) continue;
      if (c.kind === 'remove') paths.items.splice(i, 1);
      else paths.items[i] = scalar(doc, c.to);
    }
  }
  return stringifyMap(doc, text);
}

// ------------------------------------------------------------------ reporting

function report(changes, flags, after) {
  const lines = [];
  const row = (c) => {
    const what =
      c.kind === 'rewrite'
        ? `\`${c.glob}\` -> \`${c.to}\``
        : `\`${c.glob}\`${c.files ? ` (${c.files} file${c.files === 1 ? '' : 's'})` : ''}`;
    return `| ${c.kind} | \`${c.area}\` | ${what} | ${c.confidence} | ${c.reason} |`;
  };
  lines.push(
    `This PR was generated by the weekly area-owners sync (\`.github/workflows/area-owners-sync.yml\`).`,
    'It only edits paths in `.github/area-owners.yml` - reviewers are never changed automatically.',
    '',
  );
  const human = changes.filter((c) => c.area === UNASSIGNED && c.kind === 'add');
  if (human.length > 0 || flags.length > 0) {
    lines.push('### Needs a decision', '');
    for (const c of human) {
      lines.push(`- \`${c.glob}\` has no owner yet (${c.reason}). It is parked in \`${UNASSIGNED}\`; move it to the right area.`);
    }
    for (const f of flags) lines.push(`- ${f}`);
    lines.push('');
  }
  if (changes.length > 0) {
    lines.push('### Proposed changes', '', '| Change | Area | Path | Confidence | Why |', '| --- | --- | --- | --- | --- |');
    lines.push(...changes.map(row), '');
  }
  lines.push(
    '### Validation of the resulting map',
    '',
    `- Files not covered by any area: ${after.uncovered.length}`,
    `- Globs matching nothing: ${after.dead.length}`,
    `- Files parked in \`${UNASSIGNED}\`: ${after.unassigned.length}`,
    '',
    `To adjust a proposal, push to \`${BRANCH}\` or edit the map in a separate PR - the next run will not overwrite a branch that has human commits on it.`,
  );
  return lines.join('\n');
}

// ------------------------------------------------------------------ GitHub

async function publish(newText, body, changes, reviewers) {
  const repo = process.env.GITHUB_REPOSITORY;
  const owner = repo.split('/')[0];
  const gh = github(process.env.GITHUB_TOKEN);
  const { data: repoInfo } = await gh.request(`/repos/${repo}`);
  const base = repoInfo.default_branch;
  const baseSha = git(['rev-parse', 'HEAD']).trim();

  const [existing] = await gh.paginate(`/repos/${repo}/pulls?state=open&head=${owner}:${BRANCH}`);

  // Never touch the branch once a person has pushed to it - whether or not a PR is open.
  const ref = await gh.request(`/repos/${repo}/git/ref/heads/${BRANCH}`, { allow: [404] });
  if (ref.ok) {
    const { data: tip } = await gh.request(`/repos/${repo}/git/commits/${ref.data.object.sha}`);
    if (!tip.message.includes(COMMIT_MARKER)) {
      log(`\`${BRANCH}\` has commits from a person; leaving it${existing ? ` and #${existing.number}` : ''} untouched.`);
      annotate('warning', `${BRANCH} has human commits; the sync did not update or close it.`);
      return;
    }
  }

  if (newText === null) {
    if (existing) {
      await gh.request(`/repos/${repo}/issues/${existing.number}/comments`, {
        method: 'POST',
        body: { body: 'Nothing left for the sync to change; closing.' },
      });
      await gh.request(`/repos/${repo}/pulls/${existing.number}`, { method: 'PATCH', body: { state: 'closed' } });
      log(`Closed #${existing.number} - nothing left to sync.`);
    }
    if (ref.ok) {
      await gh.request(`/repos/${repo}/git/refs/heads/${BRANCH}`, { method: 'DELETE', allow: [404, 422] });
    }
    return;
  }

  // Build the commit first and move the branch once, so a failure part-way through
  // leaves the branch (and any open PR) exactly as it was.
  const { data: baseCommit } = await gh.request(`/repos/${repo}/git/commits/${baseSha}`);
  const { data: tree } = await gh.request(`/repos/${repo}/git/trees`, {
    method: 'POST',
    body: {
      base_tree: baseCommit.tree.sha,
      tree: [{ path: MAP_PATH, mode: '100644', type: 'blob', content: newText }],
    },
  });
  const { data: commit } = await gh.request(`/repos/${repo}/git/commits`, {
    method: 'POST',
    body: {
      message: `${TITLE}\n\n${COMMIT_MARKER} ${changes.length} change(s) proposed by the weekly sync.`,
      tree: tree.sha,
      parents: [baseSha],
    },
  });
  if (ref.ok) {
    await gh.request(`/repos/${repo}/git/refs/heads/${BRANCH}`, {
      method: 'PATCH',
      body: { sha: commit.sha, force: true },
    });
  } else {
    await gh.request(`/repos/${repo}/git/refs`, {
      method: 'POST',
      body: { ref: `refs/heads/${BRANCH}`, sha: commit.sha },
    });
  }

  let pr = existing;
  if (pr) {
    await gh.request(`/repos/${repo}/pulls/${pr.number}`, { method: 'PATCH', body: { title: TITLE, body } });
    log(`Updated #${pr.number}.`);
  } else {
    ({ data: pr } = await gh.request(`/repos/${repo}/pulls`, {
      method: 'POST',
      body: { title: TITLE, head: BRANCH, base, body },
    }));
    log(`Opened #${pr.number}.`);
  }

  // Skip the PR's author (the token's identity) and anyone already asked or reviewing.
  const involved = new Set(
    [pr.user, ...(pr.requested_reviewers ?? [])].filter(Boolean).map((u) => u.login.toLowerCase()),
  );
  if (existing) {
    for (const r of await gh.paginate(`/repos/${repo}/pulls/${pr.number}/reviews`)) {
      if (r.user) involved.add(r.user.login.toLowerCase());
    }
  }
  const pending = reviewers.filter((r) => !involved.has(r.toLowerCase()));
  if (pending.length > 0) await requestReviewers(gh, repo, pr.number, pending, log);
}
// ------------------------------------------------------------------ main

async function main() {
  const text = readMapText();
  const doc = parseMap(text);
  const data = doc.toJS();
  const errors = schemaErrors(data);
  if (errors.length > 0) throw new Error(`${MAP_PATH} is invalid: ${errors.join('; ')}`);

  const files = listFiles('HEAD');
  if (fs.existsSync('.git/shallow')) {
    log(`> Shallow clone: co-change suggestions use the history that was fetched.`);
    log('');
  }

  const { changes, flags } = plan(data, files);
  log(`### Area owners sync${dryRun ? ' (dry run)' : ''}`);
  log('');

  for (const f of flags) annotate('warning', `Area owners sync: ${f.replace(/`/g, '')}`, MAP_PATH);

  if (changes.length === 0 && flags.length === 0) {
    log('The map is in sync with the repository tree.');
    const parked = analyze(compileAreas(data.areas), files).unassigned.length;
    if (parked > 0) log(`${parked} file(s) are still parked in \`${UNASSIGNED}\` waiting for an owning area.`);
    if (!dryRun && !writeLocal) await publish(null);
    return;
  }

  const newText = changes.length > 0 ? apply(doc, text, changes) : text;
  const newData = parseMap(newText).toJS();
  const newErrors = schemaErrors(newData);
  if (newErrors.length > 0) throw new Error(`proposed map is invalid: ${newErrors.join('; ')}`);
  const after = analyze(compileAreas(newData.areas), files);
  const body = report(changes, flags, after);
  log(body);

  if (writeLocal) {
    fs.writeFileSync(MAP_PATH, newText);
    log('');
    log(`Wrote ${MAP_PATH}.`);
    return;
  }
  if (dryRun) {
    log('');
    log('_Dry run - set the `AREA_OWNERS_SYNC_DRY_RUN` repository variable to `false` to open the PR._');
    return;
  }
  if (changes.length === 0) {
    log('Only items that need a human were found (see the warnings); no path changes to propose.');
    await publish(null);
    return;
  }

  const touched = new Set(changes.map((c) => c.area));
  const owners = compileAreas(newData.areas)
    .filter((a) => touched.has(a.id))
    .flatMap((a) => a.reviewers);
  await publish(newText, body, changes, uniqueHandles([...mapMaintainers(compileAreas(newData.areas)), ...owners]));
}

main()
  .catch((err) => {
    log(`**Area owners sync failed:** ${err.message}`);
    annotate('error', `Area owners sync failed: ${err.message}`);
    process.exitCode = 1;
  })
  .finally(flush);
