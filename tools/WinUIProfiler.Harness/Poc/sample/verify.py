import argparse
import json
from pathlib import Path
import statistics
import time

from workload import Catalog


LABEL_COUNT = 1_000
QUERY_COUNT = 50
ITERATION_COUNT = 10
MAX_LABEL_NORMALIZATIONS = LABEL_COUNT


class CountingLabel(str):
    def __new__(cls, value: str, counter: dict[str, int]):
        instance = super().__new__(cls, value)
        instance.counter = counter
        return instance

    def casefold(self) -> str:
        self.counter["calls"] += 1
        return super().casefold()


def run_validation() -> dict:
    iterations = []
    for iteration in range(1, ITERATION_COUNT + 1):
        counter = {"calls": 0}
        labels = [
            CountingLabel(f"Item-{index:04d}", counter)
            for index in range(LABEL_COUNT)
        ]
        start = time.perf_counter_ns()
        catalog = Catalog(labels)
        for query_number in range(QUERY_COUNT):
            expected_index = query_number * 7 % LABEL_COUNT
            actual = catalog.find(f"ITEM-{expected_index:04d}")
            if actual != (expected_index,):
                raise AssertionError(
                    f"Iteration {iteration}: query {query_number} returned "
                    f"{actual!r}, expected {(expected_index,)!r}"
                )
        elapsed_ms = (time.perf_counter_ns() - start) / 1_000_000
        iterations.append(
            {
                "iteration": iteration,
                "labelNormalizations": counter["calls"],
                "elapsedMs": round(elapsed_ms, 3),
                "withinBudget": counter["calls"] <= MAX_LABEL_NORMALIZATIONS,
            }
        )

    counts = [result["labelNormalizations"] for result in iterations]
    elapsed = [result["elapsedMs"] for result in iterations]
    passed = all(result["withinBudget"] for result in iterations)
    return {
        "schema": "winui-profiler-ade-poc/measurement-v1",
        "status": "Passed" if passed else "Failed",
        "reason": "WithinWorkBudget" if passed else "WorkBudgetExceeded",
        "measurementKind": "synthetic-label-normalization-work-proxy",
        "timingIsAcceptanceCriterion": False,
        "scope": "Catalog construction and 50 queries per iteration",
        "maxLabelNormalizationsPerIteration": MAX_LABEL_NORMALIZATIONS,
        "iterations": iterations,
        "summary": {
            "iterationCount": len(iterations),
            "meanLabelNormalizations": statistics.mean(counts),
            "meanElapsedMs": round(statistics.mean(elapsed), 3),
            "medianElapsedMs": round(statistics.median(elapsed), 3),
        },
        "limitations": [
            "No WinUI app or framework is measured.",
            "No ETW data is collected.",
            "Work-count acceptance does not prove wall-clock improvement.",
        ],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Check the synthetic work budget.")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = run_validation()
    text = json.dumps(result, indent=2)
    if args.output is not None:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text + "\n", encoding="utf-8")
    print(text)
    return 0 if result["status"] == "Passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
