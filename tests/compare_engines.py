#!/usr/bin/env python3
"""Compare this engine to a saved previous Solver.cpp/Solver.hpp at equal time."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--baseline-dir", type=Path, required=True,
                    help="Directory containing the previous Solver.cpp and Solver.hpp")
parser.add_argument("--time-ms", type=int, default=100)
parser.add_argument("--games", type=int, default=12)
args = parser.parse_args()
if args.time_ms < 1 or args.games < 2 or args.games % 2:
    parser.error("Use positive time and an even number of games (alternating colors)")
root = Path(__file__).resolve().parents[1]
header = (args.baseline_dir / "Solver.hpp").read_text()
source = (args.baseline_dir / "Solver.cpp").read_text()
header = header.replace("SOLVER_HPP", "BASELINE_SOLVER_HPP")
header = re.sub(r"TIME_LIMIT_MS = [0-9]+", f"TIME_LIMIT_MS = {args.time_ms}", header)
header = re.sub(r"\bSolver\b", "BaselineSolver", header)
source = re.sub(r"\bSolver\b", "BaselineSolver", source)
source = source.replace('#include "Gomoku.hpp"', '#include "Rules.hpp"').replace("Gomoku::", "Rules::")
with tempfile.TemporaryDirectory(prefix="gomoku-compare-") as temporary:
    work = Path(temporary)
    (work / "BaselineSolver.hpp").write_text(header)
    (work / "BaselineSolver.cpp").write_text(source)
    binary = work / "engine-match"
    subprocess.run(["c++", "-std=c++20", "-O3", "-DNDEBUG", f"-I{root / 'includes'}",
                    f"-I{work}", f"-DMATCH_TIME_MS={args.time_ms}", f"-DMATCH_GAMES={args.games}",
                    str(root / "tests/engine_match.cpp"), str(work / "BaselineSolver.cpp"),
                    str(root / "src/Solver.cpp"), str(root / "src/Rules.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
