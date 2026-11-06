// Validates .github/area-owners.yml against the repository tree.
//
//   node .github/scripts/validate-area-owners.mjs
//       Full check of the working map against tracked files (git index). Fails on
//       schema errors, globs that match nothing, and files no area covers. Use this
//       locally after editing the map.
//
//   node .github/scripts/validate-area-owners.mjs --base <rev> [--head <rev>]
//       PR check. Compares the map and tree at <head> (default HEAD) with those at
//       <base> and fails only on problems the change *introduces*: new files that no
//       area covers, globs it leaves matching nothing, or schema errors. Drift that
//       already exists on the base branch is reported but left to the weekly sync.

import {
  MAP_PATH,
  UNASSIGNED,
  analyze,
  annotate,
  code,
  compileAreas,
  createLog,
  groupFiles,
  listFiles,
  parseMap,
  readMapText,
  readMapTextAt,
  schemaErrors,
} from './area-owners-lib.mjs';

const { log, flush } = createLog();

function option(name) {
  const i = process.argv.indexOf(name);
  return i === -1 ? undefined : process.argv[i + 1];
}

// Parses and analyzes one (map, tree) snapshot. `mapText` null means "no map yet".
function snapshot(mapText, files, label) {
  if (mapText === null) return null;
  const data = parseMap(mapText, `${MAP_PATH} (${label})`).toJS();
  const errors = schemaErrors(data);
  if (errors.length > 0) return { data, errors };
  const compiled = compileAreas(data.areas);
  return { data, errors, compiled, ...analyze(compiled, files) };
}

function describeGroups(data, files, limit = 15) {
  const groups = groupFiles(data.areas, files);
  for (const g of groups.slice(0, limit)) {
    log(`- ${code(g.glob)}${g.files.length > 1 ? ` (${g.files.length} files)` : ''}`);
  }
  if (groups.length > limit) log(`- ...and ${groups.length - limit} more`);
  return groups;
}

function main() {
  const baseRev = option('--base');
  const headRev = option('--head') ?? (baseRev ? 'HEAD' : undefined);

  const head = baseRev
    ? snapshot(readMapTextAt(headRev), listFiles(headRev), headRev)
    : snapshot(readMapText(), listFiles(), 'working tree');
  if (head === null) {
    log(`${MAP_PATH} does not exist at ${headRev}; nothing to validate.`);
    return 0;
  }

  log(`### Area owners check${baseRev ? ` (${baseRev}..${headRev})` : ''}`);
  log('');

  if (head.errors.length > 0) {
    log(`**${MAP_PATH} is invalid:**`);
    for (const e of head.errors) {
      log(`- ${e}`);
      annotate('error', e, MAP_PATH);
    }
    return 1;
  }

  let dead = head.dead;
  let uncovered = head.uncovered;
  let preexisting = null;

  if (baseRev) {
    let base = null;
    try {
      base = snapshot(readMapTextAt(baseRev), listFiles(baseRev), baseRev);
    } catch (err) {
      log(`> Base map could not be read (${err.message}); treating every finding as new.`);
    }
    if (base && base.errors.length === 0) {
      const oldDead = new Set(base.dead.map((d) => `${d.area}\u0000${d.glob}`));
      const oldUncovered = new Set(base.uncovered);
      dead = head.dead.filter((d) => !oldDead.has(`${d.area}\u0000${d.glob}`));
      uncovered = head.uncovered.filter((f) => !oldUncovered.has(f));
      preexisting = {
        dead: head.dead.length - dead.length,
        uncovered: head.uncovered.length - uncovered.length,
      };
    }
  }

  let failed = false;

  if (uncovered.length > 0) {
    failed = true;
    log(`**${uncovered.length} file(s) are not covered by any area.** Add a glob for them to the`);
    log(`right area in \`${MAP_PATH}\` (or to \`${UNASSIGNED}\` if the owner is not known yet):`);
    log('');
    for (const g of describeGroups(head.data, uncovered)) {
      annotate('error', `Not covered by any area in ${MAP_PATH}: ${g.glob}`, g.files[0]);
    }
    log('');
  }

  if (dead.length > 0) {
    failed = true;
    log(`**${dead.length} glob(s) match no file.** Fix or remove them in \`${MAP_PATH}\`:`);
    log('');
    for (const d of dead) {
      log(`- ${code(d.area)}: ${code(d.glob)}`);
      annotate('error', `${d.area}: glob '${d.glob}' matches no file`, MAP_PATH);
    }
    log('');
  }

  if (head.unassigned.length > 0) {
    log(`${head.unassigned.length} file(s) are parked in \`${UNASSIGNED}\` and still need an owning area.`);
    log('');
  }

  if (preexisting && (preexisting.dead > 0 || preexisting.uncovered > 0)) {
    log(
      `_Already on the base branch (not caused by this change, picked up by the weekly sync): ` +
        `${preexisting.uncovered} uncovered file(s), ${preexisting.dead} dead glob(s)._`,
    );
    log('');
  }

  if (!failed) {
    log(`OK - ${head.compiled.length} areas, every ${baseRev ? 'changed ' : ''}file is covered and every glob matches.`);
  }
  return failed ? 1 : 0;
}

try {
  process.exitCode = main();
} catch (err) {
  log(`**Area owners check failed:** ${err.message}`);
  annotate('error', err.message, MAP_PATH);
  process.exitCode = 1;
} finally {
  flush();
}
