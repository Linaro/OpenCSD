#!/usr/bin/env python3
"""
Cross-platform packet decode test runner for OpenCSD.

This replaces the shell-only packet decode test scripts with a single Python
entry point that runs on Linux, macOS, and Windows.
"""

from __future__ import annotations

import argparse
import difflib
import os
import platform
import re
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Iterable, Sequence


STANDARD_DECODE_TESTS = (
    "a57_single_step",
    "armv8_1m_branches",
    "bugfix-exact-match",
    "itm_only_csformat",
    "itm_only_raw",
    "juno_r1_1",
    "juno-ret-stck",
    "juno-uname-001",
    "juno-uname-002",
    "Snowball",
    "stm-issue-27",
    "stm_only",
    "stm_only-2",
    "stm_only-juno",
    "TC2",
    "tc2-ptm-rstk-t32",
    "test-file-mem-offsets",
    "trace_cov_a15",
)

ETE_DECODE_TESTS = (
    "001-ack_test",
    "002-ack_test_scr",
    "ete-bc-instr",
    "ete_ip",
    "ete-ite-instr",
    "ete_mem",
    "ete_spec_1",
    "ete_spec_2",
    "ete_spec_3",
    "ete-wfet",
    "event_test",
    "feat_cmpbr",
    "infrastructure",
    "maxspec0_commopt1",
    "maxspec78_commopt0",
    "pauth_lr",
    "pauth_lr_Rm",
    "q_elem",
    "rme_test",
    "s_9001",
    "src_addr",
    "ss_ib_el1ns",
    "texit-poe2",
    "tme_simple",
    "tme_tcancel",
    "tme_test",
    "trace_file_cid_vmid",
    "trace_file_vmid",
    "ts_bit64_set",
    "ts_marker",
)

ETE_SRC_ADDR_N_TESTS = (
    "002-ack_test_scr",
    "ete_ip",
    "src_addr",
)

ETE_MULTI_SESSION_TESTS = (
    "ss_ib_el1ns",
    "ete-ite-instr",
    "pauth_lr",
    "pauth_lr_Rm",
    "q_elem",
    "rme_test",
    "s_9001",
)


@dataclass(frozen=True)
class SuiteConfig:
    name: str
    out_dir: str
    snapshot_dir: str


SUITES = {
    "standard": SuiteConfig("standard", "results", "snapshots"),
    "ete": SuiteConfig("ete", "results-ete", "snapshots-ete"),
}


class TestFailure(RuntimeError):
    pass


@dataclass(frozen=True)
class CompareResultsSummary:
    diff_counts_by_test: dict[str, int]
    issue_count: int


@dataclass(frozen=True)
class CompareDirResolution:
    current_out_dir: Path | None
    baseline_dir: Path | None
    issue: str | None


def is_windows() -> bool:
    return os.name == "nt"


def exe_name(program: str) -> str:
    return f"{program}.exe" if is_windows() else program


def prepend_env_path(env: dict[str, str], key: str, value: str) -> None:
    current = env.get(key, "")
    env[key] = value if not current else os.pathsep.join((value, current))


def platform_candidate_dirs(tests_dir: Path) -> list[Path]:
    machine = platform.machine().lower()
    bin_root = tests_dir / "bin"

    candidates: list[Path] = []
    if sys.platform.startswith("linux"):
        if machine in {"aarch64", "arm64"}:
            candidates.extend((bin_root / "linux-arm64" / "rel", bin_root / "builddir"))
        elif machine in {"arm", "armv7l", "armv8l", "aarch32"}:
            candidates.extend((bin_root / "linux-arm" / "rel", bin_root / "builddir"))
        elif machine in {"x86", "i386", "i686"}:
            candidates.extend((bin_root / "linux32" / "rel", bin_root / "builddir"))
        else:
            candidates.extend((bin_root / "linux64" / "rel", bin_root / "builddir"))
    elif sys.platform == "darwin":
        if machine in {"arm64", "aarch64"}:
            candidates.extend((bin_root / "darwin-arm64" / "rel", bin_root / "builddir"))
        else:
            candidates.extend((bin_root / "darwin64" / "rel", bin_root / "builddir"))
    elif is_windows():
        if machine in {"arm64", "aarch64"}:
            candidates.append(bin_root / "winarm64" / "rel")
        elif machine in {"x86", "i386", "i686"}:
            candidates.append(bin_root / "win32" / "rel")
        else:
            candidates.append(bin_root / "win64" / "rel")

    candidates.extend(sorted(bin_root.glob("*/rel")))
    candidates.append(bin_root / "builddir")

    deduped: list[Path] = []
    seen: set[Path] = set()
    for candidate in candidates:
        resolved = candidate.resolve()
        if resolved not in seen:
            deduped.append(candidate)
            seen.add(resolved)
    return deduped


def resolve_bin_dir(tests_dir: Path, explicit: str | None, use_installed: bool) -> Path | None:
    if use_installed:
        return None

    if explicit:
        bin_dir = Path(explicit).expanduser()
        if not bin_dir.is_absolute():
            bin_dir = (Path.cwd() / bin_dir).resolve()
        if not (bin_dir / exe_name("trc_pkt_lister")).exists():
            raise FileNotFoundError(
                f"Binary directory does not contain {exe_name('trc_pkt_lister')}: {bin_dir}"
            )
        return bin_dir

    for candidate in platform_candidate_dirs(tests_dir):
        if (candidate / exe_name("trc_pkt_lister")).exists():
            return candidate

    searched = "\n".join(str(path) for path in platform_candidate_dirs(tests_dir))
    raise FileNotFoundError(
        "Could not locate trc_pkt_lister in a default test bin directory.\n"
        f"Searched:\n{searched}\n"
        "Use --bin-dir to specify the binary directory."
    )


def suite_output_dir(tests_dir: Path, suite: SuiteConfig, suffix: str | None) -> Path:
    out_dir = suite.out_dir
    if suffix:
        out_dir = f"{out_dir}-{suffix}"
    return tests_dir / out_dir


def default_results_suffix() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def iter_result_files(results_dir: Path) -> Iterable[Path]:
    return (
        path for path in sorted(results_dir.rglob("*")) if path.is_file() and path.suffix != ".diff"
    )


def relative_display_path(root_dir: Path, path: Path) -> str:
    try:
        return str(path.relative_to(root_dir))
    except ValueError:
        return str(path)


SNAPSHOT_PATH_RE = re.compile(r"(snapshots(?:-ete)?/.*)$")
SNAPSHOT_READ_PATH_RE = re.compile(
    r"^(Trace Packet Lister : reading snapshot from path )(.+)$"
)
SNAPSHOT_NOT_FOUND_PATH_RE = re.compile(
    r"^(Trace Packet Lister : Snapshot path)(.+?)( not found)$"
)
FILENAME_PATH_RE = re.compile(r"^(Filename=)(.+)$")
SNAPSHOT_RELATIVE_PREFIX_RE = re.compile(r"(?<!\w)\./(?=snapshots(?:-ete)?/)")
SNAPSHOT_ABSOLUTE_PREFIX_RE = re.compile(
    r"(?:[A-Za-z]:)?(?:[^ \t:;=]+/)+(?=snapshots(?:-ete)?/)"
)


def normalize_snapshot_path_prefixes(text: str) -> str:
    normalized = text.replace("\\", "/")
    normalized = SNAPSHOT_RELATIVE_PREFIX_RE.sub("", normalized)
    return SNAPSHOT_ABSOLUTE_PREFIX_RE.sub("", normalized)


def normalize_result_path(path_text: str) -> str:
    normalized = normalize_snapshot_path_prefixes(path_text.strip())
    match = SNAPSHOT_PATH_RE.search(normalized)
    if match is not None:
        return match.group(1)

    trimmed = normalized.rstrip("/")
    if "/" in trimmed:
        return trimmed.rsplit("/", 1)[-1]
    return normalized


def normalize_result_line(line: str) -> str:
    if "/" not in line and "\\" not in line:
        return line

    line = normalize_snapshot_path_prefixes(line)

    match = SNAPSHOT_READ_PATH_RE.match(line)
    if match is not None:
        return f"{match.group(1)}{normalize_result_path(match.group(2))}"

    match = SNAPSHOT_NOT_FOUND_PATH_RE.match(line)
    if match is not None:
        return (
            f"{match.group(1)}{normalize_result_path(match.group(2))}{match.group(3)}"
        )

    match = FILENAME_PATH_RE.match(line)
    if match is not None:
        return f"{match.group(1)}{normalize_result_path(match.group(2))}"

    return line


def filter_result_lines(path: Path) -> list[str]:
    filtered: list[str] = []
    skip_next = False
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if skip_next:
            skip_next = False
            continue
        if "Version" in line:
            continue
        filtered.append(normalize_result_line(line))
        if "Test Command Line" in line:
            skip_next = True
    return filtered


def is_suite_results_dir_name(candidate_name: str, suite: SuiteConfig) -> bool:
    matched_out_dir: str | None = None
    matched_length = -1

    for candidate_suite in SUITES.values():
        prefix = f"{candidate_suite.out_dir}-"
        if candidate_name != candidate_suite.out_dir and not candidate_name.startswith(prefix):
            continue
        if len(candidate_suite.out_dir) > matched_length:
            matched_out_dir = candidate_suite.out_dir
            matched_length = len(candidate_suite.out_dir)

    return matched_out_dir == suite.out_dir


def iter_suite_result_dirs(tests_dir: Path, suite: SuiteConfig) -> Iterable[Path]:
    for candidate in tests_dir.iterdir():
        if not candidate.is_dir():
            continue
        if is_suite_results_dir_name(candidate.name, suite):
            yield candidate


def find_latest_results_dir(
    tests_dir: Path,
    suite: SuiteConfig,
    exclude_dir: Path | None = None,
) -> Path | None:
    exclude_resolved = exclude_dir.resolve() if exclude_dir is not None else None
    candidates = [
        candidate
        for candidate in iter_suite_result_dirs(tests_dir, suite)
        if exclude_resolved is None or candidate.resolve() != exclude_resolved
    ]

    if not candidates:
        return None

    return max(candidates, key=lambda path: (path.stat().st_mtime_ns, path.name))


def find_previous_results_dir(
    tests_dir: Path,
    suite: SuiteConfig,
    current_out_dir: Path,
) -> Path | None:
    return find_latest_results_dir(tests_dir, suite, exclude_dir=current_out_dir)


def resolve_compare_dirs(
    tests_dir: Path,
    suite: SuiteConfig,
    results_suffix: str | None,
    diff_results_suffixes: Sequence[str],
    diff_only: bool,
) -> CompareDirResolution:
    if diff_only:
        if len(diff_results_suffixes) == 1:
            current_out_dir = find_latest_results_dir(tests_dir, suite)
            if current_out_dir is None:
                return CompareDirResolution(
                    None,
                    None,
                    f"no results directory found for suite {suite.name}",
                )
            baseline_dir = suite_output_dir(tests_dir, suite, diff_results_suffixes[0])
        else:
            current_out_dir = suite_output_dir(tests_dir, suite, diff_results_suffixes[0])
            baseline_dir = suite_output_dir(tests_dir, suite, diff_results_suffixes[1])
    else:
        current_out_dir = suite_output_dir(tests_dir, suite, results_suffix)
        if diff_results_suffixes:
            baseline_dir = suite_output_dir(tests_dir, suite, diff_results_suffixes[0])
        else:
            baseline_dir = find_previous_results_dir(tests_dir, suite, current_out_dir)
            if baseline_dir is None:
                return CompareDirResolution(
                    current_out_dir,
                    None,
                    f"no previous results directory found for {current_out_dir.name}",
                )

    if current_out_dir.resolve() == baseline_dir.resolve():
        return CompareDirResolution(
            current_out_dir,
            baseline_dir,
            f"comparison results directory matches current results directory ({current_out_dir.name})",
        )

    if not baseline_dir.is_dir():
        return CompareDirResolution(
            current_out_dir,
            baseline_dir,
            f"comparison results directory does not exist: {baseline_dir}",
        )

    if diff_only and not current_out_dir.is_dir():
        return CompareDirResolution(
            current_out_dir,
            baseline_dir,
            f"current results directory does not exist: {current_out_dir}",
        )

    return CompareDirResolution(current_out_dir, baseline_dir, None)


def compare_results(
    tests_dir: Path,
    selected_suites: Sequence[str],
    results_suffix: str | None,
    diff_results_suffixes: Sequence[str],
    diff_verbose: bool,
    diff_only: bool,
) -> CompareResultsSummary:
    diff_counts_by_test: dict[str, int] = {}
    issue_count = 0

    for suite_name in selected_suites:
        suite = SUITES[suite_name]
        resolution = resolve_compare_dirs(
            tests_dir,
            suite,
            results_suffix,
            diff_results_suffixes,
            diff_only,
        )
        if resolution.issue is not None:
            issue_count += 1
            print(f"\nComparison issue for {suite_name}: {resolution.issue}")
            continue

        current_out_dir = resolution.current_out_dir
        baseline_dir = resolution.baseline_dir
        assert current_out_dir is not None
        assert baseline_dir is not None

        print(f"\nComparing {suite_name} results:")
        print(f"  Current : {current_out_dir}")
        print(f"  Against : {baseline_dir}")

        current_files = list(iter_result_files(current_out_dir))
        if not current_files:
            issue_count += 1
            print(f"  Comparison issue: no result files found in {current_out_dir}")
            continue

        suite_differences = 0
        for current_file in current_files:
            relative_path = current_file.relative_to(current_out_dir)
            baseline_file = baseline_dir / relative_path
            diff_path = current_file.with_suffix(current_file.suffix + ".diff")
            test_name = current_file.stem
            summary_name = f"{suite_name}:{test_name}"

            if not baseline_file.is_file():
                suite_differences += 1
                diff_counts_by_test[summary_name] = diff_counts_by_test.get(summary_name, 0) + 1
                print(
                    "  Missing comparison file for "
                    f"{relative_path}: {relative_display_path(tests_dir, baseline_file)}"
                )
                continue

            baseline_lines = filter_result_lines(baseline_file)
            current_lines = filter_result_lines(current_file)
            if baseline_lines == current_lines:
                if diff_path.exists():
                    try:
                        diff_path.unlink()
                    except OSError:
                        pass
                continue

            suite_differences += 1
            diff_group_count = sum(
                1
                for tag, _, _, _, _ in difflib.SequenceMatcher(
                    None, baseline_lines, current_lines
                ).get_opcodes()
                if tag != "equal"
            )
            diff_counts_by_test[summary_name] = (
                diff_counts_by_test.get(summary_name, 0) + diff_group_count
            )
            diff_lines = list(
                difflib.unified_diff(
                    baseline_lines,
                    current_lines,
                    fromfile=relative_display_path(tests_dir, baseline_file),
                    tofile=relative_display_path(tests_dir, current_file),
                    lineterm="",
                )
            )
            diff_output = "\n".join(diff_lines)
            diff_path.write_text(diff_output + "\n", encoding="utf-8")
            print(f"  Wrote diff file: {relative_display_path(tests_dir, diff_path)}")
            if diff_verbose:
                print("\n" + ("=" * 40))
                print(f"Diff for {suite_name}:{relative_path}")
                print("=" * 40)
                print()
                print(diff_output)
                print()
                print("=" * 40)
                print()

        if suite_differences == 0:
            print("  No filtered differences found.")

    return CompareResultsSummary(diff_counts_by_test=diff_counts_by_test, issue_count=issue_count)


def list_available_tests(selected_suites: Sequence[str]) -> None:
    for suite_name in selected_suites:
        print(f"{suite_name} suite tests:")
        test_names = STANDARD_DECODE_TESTS if suite_name == "standard" else ETE_DECODE_TESTS
        for test_name in test_names:
            print(f"  {test_name}")


def resolve_selected_suite(test_name: str, requested_suite: str) -> str:
    in_standard = test_name in STANDARD_DECODE_TESTS
    in_ete = test_name in ETE_DECODE_TESTS

    if not in_standard and not in_ete:
        raise ValueError(
            f"Unknown test '{test_name}'. "
            "Use a test name from STANDARD_DECODE_TESTS or ETE_DECODE_TESTS."
        )

    if requested_suite == "both":
        if in_standard and not in_ete:
            return "standard"
        if in_ete and not in_standard:
            return "ete"
        raise ValueError(f"Test '{test_name}' is ambiguous across suites.")

    if requested_suite == "standard" and not in_standard:
        raise ValueError(f"Test '{test_name}' is not part of the standard suite.")

    if requested_suite == "ete" and not in_ete:
        raise ValueError(f"Test '{test_name}' is not part of the ETE suite.")

    return requested_suite


def base_env(bin_dir: Path | None, memacc_req_trace: bool = False) -> dict[str, str]:
    env = os.environ.copy()
    if memacc_req_trace:
        env["OPENCSD_MEMACC_REQ_TRACE"] = "1"

    if not bin_dir:
        return env

    bin_path = str(bin_dir)
    if is_windows():
        prepend_env_path(env, "PATH", bin_path)
    elif sys.platform == "darwin":
        prepend_env_path(env, "DYLD_LIBRARY_PATH", bin_path)
    else:
        prepend_env_path(env, "LD_LIBRARY_PATH", bin_path)
    return env


def program_path(program: str, bin_dir: Path | None) -> str:
    name = exe_name(program)
    if bin_dir is None:
        return name
    return str((bin_dir / name).resolve())


def remove_if_exists(path: Path) -> None:
    if path.exists():
        path.unlink()


def move_output(src: Path, dst: Path) -> None:
    if not src.exists():
        raise TestFailure(f"Expected output file was not created: {src}")
    dst.parent.mkdir(parents=True, exist_ok=True)
    if dst.exists():
        dst.unlink()
    shutil.move(str(src), str(dst))


def run_command(
    description: str,
    command: Sequence[str],
    cwd: Path,
    env: dict[str, str],
    quiet_stdout: bool = False,
) -> int:
    print(description)
    print("  " + " ".join(command))
    stdout = subprocess.DEVNULL if quiet_stdout else None
    completed = subprocess.run(command, cwd=str(cwd), env=env, stdout=stdout, check=False)
    print(f"Done : Return {completed.returncode}")
    return completed.returncode


def run_optional_command(
    tool_name: str,
    description: str,
    command: Sequence[str],
    cwd: Path,
    env: dict[str, str],
    quiet_stdout: bool = False,
) -> int | None:
    try:
        return run_command(description, command, cwd, env, quiet_stdout=quiet_stdout)
    except OSError as exc:
        print(f"Warning: skipping {tool_name}: failed to start program ({exc})")
        return None


def run_standard_suite(
    tests_dir: Path,
    bin_dir: Path | None,
    use_installed: bool,
    results_suffix: str | None,
    selected_test: str | None,
    lister_args: Sequence[str],
    memacc_req_trace: bool,
    failures: list[str],
) -> None:
    suite = SUITES["standard"]
    out_dir = suite_output_dir(tests_dir, suite, results_suffix)
    snapshot_dir = tests_dir / suite.snapshot_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    env = base_env(bin_dir, memacc_req_trace)
    trc_pkt_lister = program_path("trc_pkt_lister", bin_dir)

    tests_to_run = (selected_test,) if selected_test else STANDARD_DECODE_TESTS

    for test_dir in tests_to_run:
        log_name = out_dir / f"{test_dir}.ppl"
        cmd = [
            trc_pkt_lister,
            "-ss_dir",
            str(snapshot_dir / test_dir),
            *lister_args,
            "-decode",
            "-no_time_print",
            "-logfilename",
            str(log_name),
        ]
        if run_command(f"Testing {test_dir}...", cmd, tests_dir, env) != 0:
            failures.append(test_dir)

    if selected_test:
        return

    env_range = dict(env)
    env_range["OPENCSD_INSTR_RANGE_LIMIT"] = "100"
    log_name = out_dir / "juno_r1_1_rangelimit.ppl"
    cmd = [
        trc_pkt_lister,
        "-ss_dir",
        str(snapshot_dir / "juno_r1_1"),
        *lister_args,
        "-decode",
        "-no_time_print",
        "-logfilename",
        str(log_name),
    ]
    if run_command("Test with run limit on...", cmd, tests_dir, env_range) != 0:
        failures.append("juno_r1_1_rangelimit")

    env_bad_opcode = dict(env)
    env_bad_opcode["OPENCSD_ERR_ON_AA64_BAD_OPCODE"] = "1"
    log_name = out_dir / "juno_r1_1_badopcode.ppl"
    cmd = [
        trc_pkt_lister,
        "-ss_dir",
        str(snapshot_dir / "juno_r1_1"),
        *lister_args,
        "-decode",
        "-no_time_print",
        "-logfilename",
        str(log_name),
    ]
    if run_command("Test with bad opcode detect on using env var...", cmd, tests_dir, env_bad_opcode) != 0:
        failures.append("juno_r1_1_badopcode")

    log_name = out_dir / "juno_r1_1_badopcode_flag.ppl"
    cmd = [
        trc_pkt_lister,
        "-ss_dir",
        str(snapshot_dir / "juno_r1_1"),
        *lister_args,
        "-decode",
        "-no_time_print",
        "-aa64_opcode_chk",
        "-logfilename",
        str(log_name),
    ]
    if run_command("Test with bad opcode detect on using flag...", cmd, tests_dir, env) != 0:
        failures.append("juno_r1_1_badopcode_flag")

    log_name = out_dir / "init-short-addr.ppl"
    cmd = [
        trc_pkt_lister,
        "-ss_dir",
        str(snapshot_dir / "init-short-addr"),
        *lister_args,
        "-pkt_mon",
        "-no_time_print",
        "-logfilename",
        str(log_name),
    ]
    if run_command("Testing init-short-addr...", cmd, tests_dir, env) != 0:
        failures.append("init-short-addr")

    log_name = out_dir / "a55-test-tpiu.ppl"
    cmd = [
        trc_pkt_lister,
        "-ss_dir",
        str(snapshot_dir / "a55-test-tpiu"),
        *lister_args,
        "-dstream_format",
        "-no_time_print",
        "-o_raw_packed",
        "-o_raw_unpacked",
        "-logfilename",
        str(log_name),
    ]
    if run_command("Testing a55-test-tpiu...", cmd, tests_dir, env) != 0:
        failures.append("a55-test-tpiu")

    if use_installed:
        return

    c_api_log = tests_dir / "c_api_test.log"
    remove_if_exists(c_api_log)
    cmd = [
        program_path("c_api_pkt_print_test", bin_dir),
        "-ss_path",
        str(snapshot_dir),
        "-decode",
    ]
    rc = run_optional_command(
        "c_api_pkt_print_test",
        "Testing C-API library...",
        cmd,
        tests_dir,
        env,
        quiet_stdout=True,
    )
    if rc is None:
        pass
    elif rc != 0:
        failures.append("c_api_pkt_print_test")
    else:
        move_output(c_api_log, out_dir / "c_api_test.ppl")

    frame_log = tests_dir / "frame_demux_test.ppl"
    remove_if_exists(frame_log)
    cmd = [program_path("frame-demux-test", bin_dir)]
    rc = run_optional_command(
        "frame-demux-test",
        "Running Frame demux test...",
        cmd,
        tests_dir,
        env,
        quiet_stdout=True,
    )
    if rc is None:
        pass
    elif rc != 0:
        failures.append("frame-demux-test")
    else:
        move_output(frame_log, out_dir / "frame_demux_test.ppl")

    for leftover in tests_dir.glob("mem_buff_demo*.ppl"):
        leftover.unlink()

    cmd = [
        program_path("mem-buffer-eg", bin_dir),
        "-logfile",
        "-ss_path",
        str(snapshot_dir),
        "-noprint",
    ]
    rc = run_optional_command(
        "mem-buffer-eg",
        "Running mem-buffer-eg with memory buffer...",
        cmd,
        tests_dir,
        env,
    )
    if rc is None:
        pass
    elif rc != 0:
        failures.append("mem-buffer-eg")

    cmd = [
        program_path("mem-buffer-eg", bin_dir),
        "-logfile",
        "-ss_path",
        str(snapshot_dir),
        "-noprint",
        "-callback",
    ]
    rc = run_optional_command(
        "mem-buffer-eg",
        "Running mem-buffer-eg with callback function...",
        cmd,
        tests_dir,
        env,
    )
    if rc is None:
        pass
    elif rc != 0:
        failures.append("mem-buffer-eg-callback")

    for produced in sorted(tests_dir.glob("mem_buff_demo*.ppl")):
        move_output(produced, out_dir / produced.name)

    itm_log = out_dir / "itm-decode-test.ppl"
    cmd = [
        program_path("itm-decode-test", bin_dir),
        "-logfilename",
        str(itm_log),
    ]
    rc = run_optional_command(
        "itm-decode-test",
        "Running ITM decoder test...",
        cmd,
        tests_dir,
        env,
    )
    if rc is None:
        pass
    elif rc != 0:
        failures.append("itm-decode-test")


def run_ete_suite(
    tests_dir: Path,
    bin_dir: Path | None,
    results_suffix: str | None,
    selected_test: str | None,
    lister_args: Sequence[str],
    memacc_req_trace: bool,
    failures: list[str],
) -> None:
    suite = SUITES["ete"]
    out_dir = suite_output_dir(tests_dir, suite, results_suffix)
    snapshot_dir = tests_dir / suite.snapshot_dir
    out_dir.mkdir(parents=True, exist_ok=True)

    env = base_env(bin_dir, memacc_req_trace)
    trc_pkt_lister = program_path("trc_pkt_lister", bin_dir)

    tests_to_run = (selected_test,) if selected_test else ETE_DECODE_TESTS

    for test_dir in tests_to_run:
        log_name = out_dir / f"{test_dir}.ppl"
        cmd = [
            trc_pkt_lister,
            "-ss_dir",
            str(snapshot_dir / test_dir),
            *lister_args,
            "-decode",
            "-no_time_print",
            "-logfilename",
            str(log_name),
        ]
        if run_command(f"Testing {test_dir}...", cmd, tests_dir, env) != 0:
            failures.append(f"ete:{test_dir}")

    if selected_test:
        return

    for test_dir in ETE_SRC_ADDR_N_TESTS:
        log_name = out_dir / f"{test_dir}_src_addr_N.ppl"
        cmd = [
            trc_pkt_lister,
            "-ss_dir",
            str(snapshot_dir / test_dir),
            *lister_args,
            "-decode",
            "-no_time_print",
            "-src_addr_n",
            "-logfilename",
            str(log_name),
        ]
        if run_command(f"Testing with -src_addr_n {test_dir}...", cmd, tests_dir, env) != 0:
            failures.append(f"ete:{test_dir}_src_addr_N")

    for test_dir in ETE_MULTI_SESSION_TESTS:
        log_name = out_dir / f"{test_dir}_multi_sess.ppl"
        cmd = [
            trc_pkt_lister,
            "-ss_dir",
            str(snapshot_dir / test_dir),
            *lister_args,
            "-decode",
            "-no_time_print",
            "-multi_session",
            "-logfilename",
            str(log_name),
        ]
        if run_command(f"Testing with -multi_session {test_dir}...", cmd, tests_dir, env) != 0:
            failures.append(f"ete:{test_dir}_multi_sess")


def parse_args(argv: Sequence[str]) -> tuple[argparse.Namespace, list[str]]:
    parser = argparse.ArgumentParser(
        description="Run OpenCSD packet decode regression tests on Linux, macOS, or Windows.",
        epilog="Pass additional trc_pkt_lister arguments only after '--'.",
    )
    parser.add_argument(
        "--suite",
        choices=("standard", "ete", "both"),
        default="standard",
        help="Select which test suite to run.",
    )
    parser.add_argument(
        "--use-installed",
        action="store_true",
        help="Use trc_pkt_lister from PATH instead of a repository test binary directory.",
    )
    parser.add_argument(
        "--bin-dir",
        help="Directory containing trc_pkt_lister and related test binaries.",
    )
    parser.add_argument(
        "--list-only",
        action="store_true",
        help="Print the resolved binary directory and selected suites without running tests.",
    )
    parser.add_argument(
        "--list-tests",
        action="store_true",
        help="List the available tests in the selected suite or suites and exit.",
    )
    parser.add_argument(
        "--results-suffix",
        help=(
            "Append '-<suffix>' to each suite's results directory name. "
            "Defaults to a timestamp in YYYYMMDD_HHMMSS format."
        ),
    )
    parser.add_argument(
        "--diff-previous",
        action="store_true",
        help=(
            "After the requested suites finish, compare each current results file against "
            "the matching file from the most recent previous results directory."
        ),
    )
    parser.add_argument(
        "--diff-results-suffix",
        action="append",
        help=(
            "Specify a results directory suffix to use for comparison. May be supplied once "
            "for normal post-run compare, or once / twice with --diff-only."
        ),
    )
    parser.add_argument(
        "--diff-only",
        action="store_true",
        help="Run only the result comparison step without running any tests.",
    )
    parser.add_argument(
        "--diff-verbose",
        action="store_true",
        help="Print unified diffs to stdout as well as writing .diff files in the current results directory.",
    )
    parser.add_argument(
        "--test",
        help="Run a single named test from the standard or ETE decode test lists.",
    )
    parser.add_argument(
        "--memacc-req-trace",
        action="store_true",
        help="Set OPENCSD_MEMACC_REQ_TRACE=1 for every test process started by this runner.",
    )

    passthrough: list[str] = []
    parseable_argv = list(argv)
    if "--" in parseable_argv:
        separator_index = parseable_argv.index("--")
        passthrough = parseable_argv[separator_index + 1 :]
        parseable_argv = parseable_argv[:separator_index]

    namespace = parser.parse_args(parseable_argv)

    diff_results_suffixes = namespace.diff_results_suffix or []
    if namespace.diff_previous and diff_results_suffixes:
        parser.error("--diff-previous cannot be used with --diff-results-suffix")
    if len(diff_results_suffixes) > 2:
        parser.error("--diff-results-suffix may be specified at most twice")
    if namespace.diff_only:
        if namespace.diff_previous:
            parser.error("--diff-only cannot be used with --diff-previous")
        if len(diff_results_suffixes) not in {1, 2}:
            parser.error("--diff-only requires --diff-results-suffix to be used 1 or 2 times")
        if len(diff_results_suffixes) == 2 and diff_results_suffixes[0] == diff_results_suffixes[1]:
            parser.error("the two --diff-results-suffix values must be different")
    elif len(diff_results_suffixes) > 1:
        parser.error("without --diff-only, --diff-results-suffix may be specified only once")

    return namespace, passthrough


def main(argv: Sequence[str]) -> int:
    args, lister_args = parse_args(argv)
    tests_dir = Path(__file__).resolve().parent
    diff_results_suffixes = args.diff_results_suffix or []
    results_suffix = None if args.diff_only else (args.results_suffix or default_results_suffix())
    diff_requested = args.diff_only or args.diff_previous or bool(diff_results_suffixes)
    selected_suite = args.suite
    selected_suites = ("standard", "ete") if selected_suite == "both" else (selected_suite,)

    if args.list_tests:
        list_available_tests(selected_suites)
        return 0

    try:
        if args.test:
            selected_suite = resolve_selected_suite(args.test, args.suite)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    bin_dir: Path | None = None
    if not args.diff_only:
        try:
            bin_dir = resolve_bin_dir(tests_dir, args.bin_dir, args.use_installed)
        except FileNotFoundError as exc:
            print(str(exc), file=sys.stderr)
            return 2

        print("Running trc_pkt_lister on snapshot directories.")
        if bin_dir is None:
            print("Tests using installed binaries.")
        else:
            print(f"Tests using BIN_DIR = {bin_dir}")
    else:
        print("Running diff-only comparison on existing results directories.")

    selected_suites = ("standard", "ete") if selected_suite == "both" else (selected_suite,)
    print(f"Selected suites: {', '.join(selected_suites)}")
    if args.test:
        print(f"Selected test: {args.test}")
    if results_suffix is not None:
        print(f"Results suffix: {results_suffix}")
    if diff_requested:
        if args.diff_only:
            if len(diff_results_suffixes) == 1:
                print(
                    "Diff-only mode: comparing the most recent results directory for each selected "
                    "suite against the supplied suffix."
                )
                print(f"Diff-only baseline suffix: {diff_results_suffixes[0]}")
            else:
                print(f"Diff-only current suffix: {diff_results_suffixes[0]}")
                print(f"Diff-only baseline suffix: {diff_results_suffixes[1]}")
        elif not diff_results_suffixes:
            print("Post-run diff: comparing against the most recent previous results directory.")
        else:
            print(f"Post-run diff suffix: {diff_results_suffixes[0]}")
        if args.diff_verbose:
            print("Post-run diff verbosity: enabled")
    if args.memacc_req_trace:
        print("Environment override: OPENCSD_MEMACC_REQ_TRACE=1")
    for suite_name in selected_suites:
        suite = SUITES[suite_name]
        if results_suffix is not None:
            print(f"Results dir for {suite_name}: {suite_output_dir(tests_dir, suite, results_suffix)}")
        if diff_requested:
            resolution = resolve_compare_dirs(
                tests_dir,
                suite,
                results_suffix,
                diff_results_suffixes,
                args.diff_only,
            )
            if resolution.issue is None:
                print(f"Compare current dir for {suite_name}: {resolution.current_out_dir}")
                print(f"Compare baseline dir for {suite_name}: {resolution.baseline_dir}")
            else:
                print(f"Compare setup issue for {suite_name}: {resolution.issue}")
    if lister_args:
        print(f"Additional trc_pkt_lister args: {' '.join(lister_args)}")

    if args.list_only:
        return 0

    failures: list[str] = []

    if not args.diff_only:
        try:
            for suite_name in selected_suites:
                if suite_name == "standard":
                    run_standard_suite(
                        tests_dir,
                        bin_dir,
                        args.use_installed,
                        results_suffix,
                        args.test,
                        lister_args,
                        args.memacc_req_trace,
                        failures,
                    )
                else:
                    run_ete_suite(
                        tests_dir,
                        bin_dir,
                        results_suffix,
                        args.test,
                        lister_args,
                        args.memacc_req_trace,
                        failures,
                    )
        except TestFailure as exc:
            print(str(exc), file=sys.stderr)
            return 1
        except OSError as exc:
            print(f"Failed to start a test program: {exc}", file=sys.stderr)
            return 1

    comparison_summary = CompareResultsSummary(diff_counts_by_test={}, issue_count=0)
    if diff_requested:
        comparison_summary = compare_results(
            tests_dir,
            selected_suites,
            results_suffix,
            diff_results_suffixes,
            args.diff_verbose,
            args.diff_only,
        )

    if failures:
        print("\nFailures:")
        for failure in failures:
            print(f"  {failure}")
    if comparison_summary.diff_counts_by_test or comparison_summary.issue_count:
        print("\nResult comparison issues:")
        if comparison_summary.diff_counts_by_test:
            for test_name, diff_count in sorted(comparison_summary.diff_counts_by_test.items()):
                if diff_count == 1:
                    print(f"  {test_name}: {diff_count} difference")
                else:
                    print(f"  {test_name}: {diff_count} differences")
        if comparison_summary.issue_count:
            print(f"  Comparison setup issues: {comparison_summary.issue_count}")
    if failures or comparison_summary.diff_counts_by_test or comparison_summary.issue_count:
        return 1

    print("\nAll requested packet decode tests completed successfully.")
    return 0


if __name__ == "__main__":
    start_time = time.perf_counter()
    try:
        raise SystemExit(main(sys.argv[1:]))
    finally:
        elapsed = time.perf_counter() - start_time
        print(f"\nTotal execution time: {elapsed:.2f} seconds")
