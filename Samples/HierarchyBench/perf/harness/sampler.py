#!/usr/bin/env python3
r"""
L1 external process sampler for the TableView perf framework.

Launches the dedicated benchmark app once for ONE matrix cell, samples per-process
metrics from the OUTSIDE (the app can't measure these about itself reliably),
waits for it to auto-quit, then merges the app's L2 in-app CSV into one unified
results row appended to results.csv.

Metrics captured here (L1):
  startup_ms      process-create -> first-render ready-file
  binary_kb       sum of exe + dlls in the app dir
  cpu_avg/max     psutil cpu_percent (per-process, % of one core)
  priv_mb avg/max psutil private/uss committed memory
  ws_mb avg/max   psutil working set (resident)
  bytes_per_row   ws_mb_avg * 1MB / rows
  handles/threads psutil num_handles / num_threads
  gdi_max/user_max Win32 GetGuiResources(GR_GDIOBJECTS/GR_USEROBJECTS)
  ctx_switches    psutil num_ctx_switches rate (/sec)
  gpu_util avg/max PDH \GPU Engine(pid_*engtype_3D) Utilization (best-effort)
  samples         number of samples collected

Usage:
  python sampler.py --exe <app.exe> --control DataGrid --scenario load \
      --rows 1000 --cols 10 --run 1 --out results.csv [--settle-ms 1500] [--gpu]
"""
import argparse, csv, os, subprocess, sys, tempfile, time, statistics, ctypes
from ctypes import wintypes

try:
    import psutil
except ImportError:
    print("psutil not installed: pip install psutil", file=sys.stderr); sys.exit(2)

# ---- Win32 GetGuiResources (GDI / USER object counts) ----
_PROCESS_QUERY_INFORMATION = 0x0400
_GR_GDIOBJECTS = 0
_GR_USEROBJECTS = 1
_k32 = ctypes.WinDLL("kernel32", use_last_error=True)
_u32 = ctypes.WinDLL("user32", use_last_error=True)
_k32.OpenProcess.restype = wintypes.HANDLE
_k32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
_k32.CloseHandle.argtypes = [wintypes.HANDLE]
_u32.GetGuiResources.restype = wintypes.DWORD
_u32.GetGuiResources.argtypes = [wintypes.HANDLE, wintypes.DWORD]

def gui_resources(pid):
    h = _k32.OpenProcess(_PROCESS_QUERY_INFORMATION, False, pid)
    if not h:
        return None, None
    try:
        gdi = _u32.GetGuiResources(h, _GR_GDIOBJECTS)
        usr = _u32.GetGuiResources(h, _GR_USEROBJECTS)
        return (gdi or None), (usr or None)
    finally:
        _k32.CloseHandle(h)

def gpu_util(pid):
    """Best-effort per-process GPU 3D-engine utilization via PDH. Returns float% or None."""
    ctr = r"\GPU Engine(pid_{}*engtype_3D)\Utilization Percentage".format(pid)
    cmd = ["powershell", "-NoProfile", "-Command",
           "(Get-Counter '{}' -EA SilentlyContinue).CounterSamples | "
           "Measure-Object -Property CookedValue -Sum | Select -Expand Sum".format(ctr)]
    try:
        out = subprocess.run(cmd, capture_output=True, text=True, timeout=8).stdout.strip()
        return float(out) if out else 0.0
    except Exception:
        return None

def binary_kb(exe):
    d = os.path.dirname(exe)
    total = 0
    for f in os.listdir(d):
        if f.lower().endswith((".exe", ".dll")):
            try: total += os.path.getsize(os.path.join(d, f))
            except OSError: pass
    return round(total / 1024.0, 1)

def agg(vals):
    vals = [v for v in vals if v is not None]
    if not vals: return ("", "")
    return (round(statistics.mean(vals), 2), round(max(vals), 2))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", required=True)
    ap.add_argument("--control", default="DataGrid")
    ap.add_argument("--scenario", default="load")
    ap.add_argument("--rows", type=int, default=1000)
    ap.add_argument("--cols", type=int, default=10)
    ap.add_argument("--run", type=int, default=1)
    ap.add_argument("--out", required=True)
    ap.add_argument("--settle-ms", type=int, default=1500)
    ap.add_argument("--interval-ms", type=int, default=100)
    ap.add_argument("--gpu", action="store_true")
    ap.add_argument("--shape", default=None, help="Hierarchy benches: tree shape, forwarded to the app.")
    a = ap.parse_args()

    tmp = tempfile.mkdtemp(prefix="tvperf_")
    l2 = os.path.join(tmp, "l2.csv")
    ready = os.path.join(tmp, "ready.txt")
    args = [a.exe, "--control", a.control, "--scenario", a.scenario,
            "--rows", str(a.rows), "--cols", str(a.cols),
            "--emit", l2, "--ready-file", ready, "--settle-ms", str(a.settle_ms)]
    if a.shape:
        args += ["--shape", a.shape]

    t0 = time.perf_counter()
    proc = subprocess.Popen(args)
    p = psutil.Process(proc.pid)

    startup_ms = None
    cpu, priv, ws, handles, threads = [], [], [], [], []
    gdi_vals, user_vals, gpu_vals = [], [], []
    ctx0 = ctx1 = None
    t_first = t_last = time.perf_counter()
    try:
        p.cpu_percent(None)  # prime
    except Exception:
        pass

    gpu_taken = 0
    while proc.poll() is None:
        try:
            cpu.append(p.cpu_percent(None))
            mi = p.memory_full_info()
            priv.append((getattr(mi, "uss", None) or getattr(mi, "private", mi.rss)) / (1024*1024))
            ws.append(mi.rss / (1024*1024))
            handles.append(getattr(p, "num_handles", lambda: None)())
            threads.append(p.num_threads())
            g, u = gui_resources(proc.pid)
            if g: gdi_vals.append(g)
            if u: user_vals.append(u)
            cs = p.num_ctx_switches()
            cur = (cs.voluntary + cs.involuntary)
            if ctx0 is None: ctx0 = cur
            ctx1 = cur
            t_last = time.perf_counter()
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            break
        if startup_ms is None and os.path.exists(ready):
            startup_ms = round((time.perf_counter() - t0) * 1000.0, 1)
        if a.gpu and gpu_taken < 3 and (time.perf_counter() - t_first) > 0.5:
            gv = gpu_util(proc.pid)
            if gv is not None: gpu_vals.append(gv)
            gpu_taken += 1
        time.sleep(a.interval_ms / 1000.0)

    proc.wait()
    if startup_ms is None and os.path.exists(ready):
        try: startup_ms = round(float(open(ready).read().strip()), 1)
        except Exception: pass

    dur = max(0.001, t_last - t_first)
    cpu_avg, cpu_max = agg(cpu)
    priv_avg, priv_max = agg(priv)
    ws_avg, ws_max = agg(ws)
    h_avg, h_max = agg(handles)
    th_avg, th_max = agg(threads)
    gdi_max = max(gdi_vals) if gdi_vals else ""
    user_max = max(user_vals) if user_vals else ""
    gpu_avg, gpu_max = agg(gpu_vals) if gpu_vals else ("", "")
    ctx_rate = round((ctx1 - ctx0) / dur, 0) if (ctx0 is not None and ctx1 is not None) else ""
    bytes_per_row = round(ws_avg * 1024 * 1024 / a.rows, 0) if ws_avg != "" and a.rows else ""

    # ---- merge app's L2 in-app metrics ----
    l2row = {}
    if os.path.exists(l2):
        with open(l2, newline="") as f:
            r = list(csv.DictReader(f))
            if r: l2row = r[0]

    def g2(k): return l2row.get(k, "")

    row = {
        "control": a.control, "scenario": a.scenario, "rows": a.rows, "cols": a.cols, "run": a.run,
        "startup_ms": startup_ms if startup_ms is not None else "",
        "binary_kb": binary_kb(a.exe),
        "cpu_avg": cpu_avg, "cpu_max": cpu_max,
        "priv_mb_avg": priv_avg, "priv_mb_max": priv_max,
        "ws_mb_avg": ws_avg, "ws_mb_max": ws_max,
        "bytes_per_row": bytes_per_row,
        "handles_avg": h_avg, "handles_max": h_max,
        "threads_avg": th_avg, "threads_max": th_max,
        "gdi_max": gdi_max, "user_max": user_max,
        "ctx_switches": ctx_rate,
        "gpu_util_avg": gpu_avg, "gpu_util_max": gpu_max,
        "samples": len(cpu),
        "creation_ms": g2("creation_ms"), "first_render_ms": g2("first_render_ms"),
        "fps_avg": g2("fps_avg"), "frame_p99_ms": g2("frame_p99_ms"),
        "jank_pct": g2("jank_pct"), "frame_drops": g2("frame_drops"),
        "realized_rows": g2("realized_rows"), "total_rows": g2("total_rows") or a.rows,
        "managed_heap_mb": g2("managed_heap_mb"),
        "gc_gen0": g2("gc_gen0"), "gc_gen1": g2("gc_gen1"), "gc_gen2": g2("gc_gen2"),
        "sort_ms": g2("sort_ms"), "filter_ms": g2("filter_ms"), "scenario_ms": g2("scenario_ms"),
        "select_ms": g2("select_ms"),
        "realized_cols": g2("realized_cols"), "realized_cells": g2("realized_cells"),
        "frame_p95_ms": g2("frame_p95_ms"), "frame_max_ms": g2("frame_max_ms"),
        "time_to_interactive_ms": g2("time_to_interactive_ms"),
        "data_gen_ms": g2("data_gen_ms"), "control_creation_ms": g2("control_creation_ms"),
        # Hierarchy benches (empty for the flat benches).
        "api": g2("api"), "shape": g2("shape") or (a.shape or ""), "op_ms": g2("op_ms"),
        "setup_expand_ms": g2("setup_expand_ms"), "visible_rows_before": g2("visible_rows_before"),
        "visible_rows_after": g2("visible_rows_after"), "error": g2("error"),
        "op_sync_ms": g2("op_sync_ms"), "layout_ms": g2("layout_ms"), "render_ms": g2("render_ms"),
        "exit_code": proc.returncode,
    }

    cols = ["control","scenario","rows","cols","run","startup_ms","binary_kb","cpu_avg","cpu_max",
            "priv_mb_avg","priv_mb_max","ws_mb_avg","ws_mb_max","bytes_per_row","handles_avg","handles_max",
            "threads_avg","threads_max","gdi_max","user_max","ctx_switches","gpu_util_avg","gpu_util_max",
            "samples","creation_ms","first_render_ms","fps_avg","frame_p99_ms","jank_pct","frame_drops",
            "realized_rows","total_rows","managed_heap_mb","gc_gen0","gc_gen1","gc_gen2","sort_ms","filter_ms","scenario_ms",
            "realized_cols","realized_cells","frame_p95_ms","frame_max_ms","time_to_interactive_ms","data_gen_ms","control_creation_ms","select_ms",
            "api","shape","op_ms","setup_expand_ms","visible_rows_before","visible_rows_after","error",
            "op_sync_ms","layout_ms","render_ms","exit_code"]
    new = not os.path.exists(a.out) or os.path.getsize(a.out) == 0
    with open(a.out, "a", newline="") as f:
        w = csv.DictWriter(f, fieldnames=cols)
        if new: w.writeheader()
        w.writerow(row)

    print("run done: {} {} rows={} cols={} -> startup={}ms ws_avg={}MB cpu_avg={}% realized={}/{} fps={}".format(
        a.control, a.scenario, a.rows, a.cols, row["startup_ms"], ws_avg, cpu_avg,
        row["realized_rows"], row["total_rows"], row["fps_avg"]))

if __name__ == "__main__":
    main()
