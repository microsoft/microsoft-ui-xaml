// Shared helpers for the area-owners map: loading, glob matching and validation.
// Used by area-reviewers.mjs, validate-area-owners.mjs and sync-area-owners.mjs so
// that all three agree on what a glob matches.

import fs from 'node:fs';
import { execFileSync } from 'node:child_process';
import YAML from 'yaml';

export const MAP_PATH = '.github/area-owners.yml';

// Holding area for paths nobody owns yet. It requests no reviewers (those files fall
// through to CODEOWNERS) and counts as "covered" so drift checks stay quiet, but it is
// reported as a warning until a human moves each path into a real area.
export const UNASSIGNED = 'UNASSIGNED';

const HANDLE = /^[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})$/;

export function parseMap(text, source = MAP_PATH) {
  const doc = YAML.parseDocument(text);
  if (doc.errors.length > 0) {
    throw new Error(`${source}: ${doc.errors[0].message}`);
  }
  return doc;
}

export function readMapText(path = MAP_PATH) {
  return fs.readFileSync(path, 'utf8');
}

// Serializes an edited map so that untouched lines stay byte-for-byte identical.
export function stringifyMap(doc, originalText) {
  const text = doc.toString({ lineWidth: 0, flowCollectionPadding: false });
  return originalText.includes('\r\n') ? text.replace(/\r?\n/g, '\r\n') : text;
}

// Returns the map as committed at `rev`, or null when the file does not exist there.
// Any other git failure (e.g. a lazy blob fetch in a partial clone) is thrown.
export function readMapTextAt(rev) {
  if (git(['ls-tree', '--name-only', rev, '--', MAP_PATH]).trim() === '') return null;
  return git(['show', `${rev}:${MAP_PATH}`]);
}

export function git(args) {
  return execFileSync('git', args, {
    encoding: 'utf8',
    maxBuffer: 1 << 30,
    stdio: ['ignore', 'pipe', 'pipe'],
  });
}

// Tracked files at `rev`, or in the index when `rev` is omitted (so local, uncommitted
// additions are validated too).
export function listFiles(rev) {
  const out = rev ? git(['ls-tree', '-r', '-z', '--name-only', rev]) : git(['ls-files', '-z']);
  return out.split('\0').filter(Boolean);
}

// Glob -> regex. `**` crosses directories, `*` and `?` do not, everything else is literal.
// Matching is case-insensitive against repository-relative paths.
export function toRegExp(glob) {
  const source = glob
    .replace(/[.+^${}()|[\]\\]/g, '\\$&')
    .replace(/\*\*\//g, '\u0000')
    .replace(/\*\*/g, '\u0001')
    .replace(/\*/g, '[^/]*')
    .replace(/\?/g, '[^/]')
    .replace(/\u0000/g, '(?:.*/)?')
    .replace(/\u0001/g, '.*');
  return new RegExp(`^${source}$`, 'i');
}

// Structural checks. Returns a list of error strings; an empty list means the map is
// safe to compile.
export function schemaErrors(data) {
  const errors = [];
  if (!data || !Array.isArray(data.areas)) {
    return ['top-level `areas` list is missing'];
  }
  const ids = new Set();
  data.areas.forEach((area, i) => {
    const where = `areas[${i}]${area?.id ? ` (${area.id})` : ''}`;
    if (!area || typeof area !== 'object') {
      errors.push(`${where}: must be a mapping`);
      return;
    }
    if (typeof area.id !== 'string' || area.id.length === 0) {
      errors.push(`${where}: \`id\` is required`);
    } else if (ids.has(area.id.toLowerCase())) {
      errors.push(`${where}: duplicate id`);
    } else {
      ids.add(area.id.toLowerCase());
    }
    if (typeof area.name !== 'string' || area.name.length === 0) {
      errors.push(`${where}: \`name\` is required`);
    }
    const unassigned = area.id === UNASSIGNED;
    if (!Array.isArray(area.reviewers) || (!unassigned && area.reviewers.length === 0)) {
      errors.push(`${where}: \`reviewers\` must be a non-empty list`);
    } else {
      for (const r of area.reviewers) {
        if (typeof r !== 'string' || !HANDLE.test(r)) {
          errors.push(`${where}: \`${r}\` is not a valid GitHub handle`);
        }
      }
      if (unassigned && area.reviewers.length > 0) {
        errors.push(`${where}: the ${UNASSIGNED} area must not list reviewers`);
      }
    }
    if (!Array.isArray(area.paths) || (!unassigned && area.paths.length === 0)) {
      errors.push(`${where}: \`paths\` must be a non-empty list`);
    } else {
      for (const p of area.paths) {
        if (typeof p !== 'string' || p.length === 0 || p.startsWith('/')) {
          errors.push(`${where}: \`${p}\` must be a repository-relative glob`);
        }
      }
    }
  });
  return errors;
}

export function compileAreas(areas) {
  return areas.map((area) => ({
    ...area,
    matchers: area.paths.map((glob) => ({ glob, re: toRegExp(glob) })),
  }));
}

export function areasForFile(compiled, file) {
  return compiled.filter((area) => area.matchers.some((m) => m.re.test(file)));
}

// Runs every file against every glob once and reports what the map gets wrong.
//   dead:       globs that match no file
//   uncovered:  files no area (not even UNASSIGNED) matches
//   unassigned: files whose only match is the UNASSIGNED holding area
export function analyze(compiled, files) {
  const hits = new Map();
  for (const area of compiled) {
    for (const m of area.matchers) hits.set(`${area.id}\u0000${m.glob}`, 0);
  }
  const uncovered = [];
  const unassigned = [];
  for (const file of files) {
    let owned = false;
    let parked = false;
    for (const area of compiled) {
      let matched = false;
      for (const m of area.matchers) {
        if (m.re.test(file)) {
          const key = `${area.id}\u0000${m.glob}`;
          hits.set(key, hits.get(key) + 1);
          matched = true;
        }
      }
      if (matched) {
        if (area.id === UNASSIGNED) parked = true;
        else owned = true;
      }
    }
    if (!owned && !parked) uncovered.push(file);
    else if (!owned) unassigned.push(file);
  }
  const dead = [];
  for (const [key, count] of hits) {
    if (count === 0) {
      const [area, glob] = key.split('\u0000');
      dead.push({ area, glob });
    }
  }
  return { dead, uncovered, unassigned };
}

// Directories that already host one-glob-per-component entries such as
// `controls/dev/Repeater/**` -> `controls/dev/`. Used to group stray files into the
// component directory a human would add as a single glob.
function componentParents(areas) {
  const parents = new Map();
  for (const area of areas) {
    for (const glob of area.paths) {
      const m = glob.match(/^((?:[^*?/]+\/)*)([^*?/]+)\/\*\*$/);
      if (!m) continue;
      const parent = m[1];
      if (!parents.has(parent)) parents.set(parent, []);
      parents.get(parent).push({ area: area.id, name: m[2], glob });
    }
  }
  return parents;
}

// Groups files into the smallest directory that fits the map's existing layout, e.g.
// every file under `controls/dev/TableView/` becomes one `controls/dev/TableView/**`
// group. Files that sit directly in such a parent are returned as their own group.
export function groupFiles(areas, files) {
  const parents = componentParents(areas);
  const sorted = [...parents.keys()].sort((a, b) => b.length - a.length);
  const groups = new Map();
  for (const file of files) {
    const parent = sorted.find((p) => file.startsWith(p) && file.slice(p.length).includes('/'));
    let key;
    let glob;
    let siblings = [];
    if (parent !== undefined) {
      const name = file.slice(parent.length).split('/')[0];
      key = `${parent}${name}/`;
      glob = `${parent}${name}/**`;
      siblings = parents.get(parent);
    } else {
      key = file;
      glob = file;
    }
    if (!groups.has(key)) groups.set(key, { dir: key, glob, files: [], siblings });
    groups.get(key).files.push(file);
  }
  return [...groups.values()].sort((a, b) => b.files.length - a.files.length);
}

// The GitHub handles that should look after the map itself: whoever owns the area
// that covers MAP_PATH.
export function mapMaintainers(compiled) {
  return [
    ...new Set(
      areasForFile(compiled, MAP_PATH)
        .filter((a) => a.id !== UNASSIGNED)
        .flatMap((a) => a.reviewers),
    ),
  ];
}

export function uniqueHandles(handles) {
  const seen = new Map();
  for (const h of handles) {
    const key = h.toLowerCase();
    if (!seen.has(key)) seen.set(key, h);
  }
  return [...seen.values()];
}

// Minimal GitHub REST client: returns parsed JSON, throws on non-2xx unless the
// caller opts into handling the status itself.
export function github(token) {
  async function request(route, { method = 'GET', body, allow = [] } = {}) {
    const res = await fetch(`https://api.github.com${route}`, {
      method,
      headers: {
        accept: 'application/vnd.github+json',
        authorization: `Bearer ${token}`,
        'content-type': 'application/json',
        'x-github-api-version': '2022-11-28',
      },
      body: body === undefined ? undefined : JSON.stringify(body),
    });
    const data = res.status === 204 ? null : await res.json().catch(() => null);
    if (!res.ok && !allow.includes(res.status)) {
      const detail = data?.message ? `: ${data.message}` : '';
      throw new Error(`${method} ${route} failed with HTTP ${res.status}${detail}`);
    }
    return { ok: res.ok, status: res.status, data };
  }

  async function paginate(route, limit = Infinity) {
    const items = [];
    const sep = route.includes('?') ? '&' : '?';
    for (let page = 1; items.length < limit; page++) {
      const { data } = await request(`${route}${sep}per_page=100&page=${page}`);
      if (!Array.isArray(data)) {
        throw new Error(`GET ${route} returned an unexpected payload`);
      }
      items.push(...data);
      if (data.length < 100) break;
    }
    return items;
  }

  return { request, paginate };
}

// Appends markdown to the job summary when running in Actions; always echoes to stdout.
export function createLog() {
  const lines = [];
  const log = (line = '') => {
    console.log(line);
    lines.push(line);
  };
  const flush = () => {
    if (process.env.GITHUB_STEP_SUMMARY) {
      fs.appendFileSync(process.env.GITHUB_STEP_SUMMARY, `${lines.join('\n')}\n`);
    }
  };
  return { log, flush };
}

// GitHub Actions workflow command (shows up as an annotation on the run / PR).
export function annotate(level, message, file) {
  if (!process.env.GITHUB_ACTIONS) return;
  const escapeData = (s) => String(s).replace(/%/g, '%25').replace(/\r/g, '%0D').replace(/\n/g, '%0A');
  const escapeProperty = (s) => escapeData(s).replace(/:/g, '%3A').replace(/,/g, '%2C');
  const where = file ? ` file=${escapeProperty(file)}` : '';
  console.log(`::${level}${where}::${escapeData(message)}`);
}

// Renders a path (possibly from an untrusted PR) as inline code on a single line, so a
// crafted file name cannot break out into a workflow command or the markdown summary.
export function code(path) {
  return `\`${String(path).replace(/[\u0000-\u001f\u007f`]/g, '?')}\``;
}

// Requests reviewers on a PR. One unknown or non-collaborator handle makes GitHub
// reject the whole batch, so on 422 it retries one handle at a time and reports the
// ones GitHub refuses. Returns the handles that were requested.
export async function requestReviewers(gh, repo, number, reviewers, log) {
  const route = `/repos/${repo}/pulls/${number}/requested_reviewers`;
  const all = await gh.request(route, { method: 'POST', body: { reviewers }, allow: [422] });
  if (all.ok) {
    log(`Requested: ${reviewers.map((r) => `@${r}`).join(', ')}`);
    return reviewers;
  }
  log(`Batch request rejected (${all.data?.message ?? 'HTTP 422'}); retrying one by one.`);
  const requested = [];
  for (const reviewer of reviewers) {
    const one = await gh.request(route, { method: 'POST', body: { reviewers: [reviewer] }, allow: [422] });
    if (one.ok) {
      requested.push(reviewer);
      log(`- Requested @${reviewer}`);
    } else {
      const reason = one.data?.message ?? 'HTTP 422';
      log(`- Could not request @${reviewer}: ${reason}`);
      annotate('warning', `Could not request @${reviewer} (${reason}). Check the handle in ${MAP_PATH}.`);
    }
  }
  return requested;
}
