#!/usr/bin/env python3

import os
import sys
import argparse
import json
import subprocess
import time
import csv

N = 10


def input_file(benchmark):
    pre, _ = os.path.splitext(benchmark)
    return pre + ".in"


def bench(passes: "list[str]"):
    passes = " ".join("-p {}".format(p) for p in passes)
    output_dir = "compiled_benchmarks"
    benchmarks = [f for f in os.listdir("benchmarks") if f.endswith(".lisp")]

    # First, compile all benchmarks
    for benchmark in benchmarks:
        compile_cmd = "dune exec --root . bin/compile.exe -- {} {} {}".format(
            passes,
            os.path.join(os.path.relpath("benchmarks", start=".."), benchmark),
            os.path.relpath(output_dir, start=".."),
        )
        
        subprocess.run(
            compile_cmd,
            shell=True,
            cwd="..",
            check=True,
        )

    # Then, run each benchmark N times and record the results
    results = {}
    for benchmark in benchmarks:
        runs = []
        run_cmd = os.path.join("compiled_benchmarks", "{}.exe".format(benchmark))
        stdin = ""
        input_path = os.path.join("benchmarks", input_file(benchmark))
        if os.path.exists(input_path):
            stdin = open(input_path).read()
        for i in range(N):
            start = time.perf_counter()
            subprocess.run(
                run_cmd,
                input=stdin,
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                universal_newlines=True,
            )
            end = time.perf_counter()
            runs.append(end - start)
        results[benchmark] = sum(runs) / len(runs)
    return results


def main():
    if not os.path.exists("benchmarks"):
        print(
            "Benchmark script should be run from the root of the benchmarks repository"
        )
        sys.exit(1)
    parser = argparse.ArgumentParser(
        description="CS 164 Homework 8 Benchmark Script"
    )
    subprocess.run(
            "dune build",
            shell=True,
            cwd="..",
            check=True,
        )
    parser.add_argument("--config", type=argparse.FileType("r"), default="config.json")
    parser.add_argument("--output", type=argparse.FileType("w"), default="results.csv")
    args = parser.parse_args()
    config = None
    try:
        config = json.load(args.config)
    except json.decoder.JSONDecodeError as e:
        print("Error reading config file {}: {}".format(args.config.name, e))
        sys.exit(1)
    results = {}
    for name in config:
        results[name] = bench(config[name])
    writer = csv.writer(args.output)
    writer.writerow(("Benchmark name", "Configuration", "Time taken (s)"))
    for benchmark in sorted(results[name].keys()):
        for name in sorted(results.keys()):
            writer.writerow((benchmark, name, results[name][benchmark]))


if __name__ == "__main__":
    main()

