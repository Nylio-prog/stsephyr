#!/usr/bin/env python3
"""Build, flash, and run every STSEphyr sample on attached hardware."""

from __future__ import annotations

import argparse
import codecs
import re
import subprocess
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence


DEFAULT_BOARD = "nucleo_l452re"
DEFAULT_SHIELD = "x_nucleo_ese01a1"
DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 30.0

ANSI_ESCAPE = re.compile(r"\x1b\[[0-?]*[ -/]*[@-~]")
FAILURE_MARKERS = (
    "FAIL:",
    "FATAL ERROR",
    "ASSERTION FAIL",
    "***** HARD FAULT *****",
)


@dataclass(frozen=True)
class Sample:
    name: str
    source_dir: Path
    pass_marker: str
    failure_markers: tuple[str, ...] = FAILURE_MARKERS


@dataclass
class Result:
    name: str
    status: str
    build_seconds: float
    flash_seconds: float | None
    run_seconds: float | None
    total_seconds: float
    reason: str = ""


class SerialCapture:
    """Continuously drain a serial port while retaining and optionally echoing it."""

    def __init__(self, port, pass_marker: str, failure_markers: Sequence[str]):
        self._port = port
        self._pass_marker = pass_marker
        self._failure_markers = failure_markers
        self._decoder = codecs.getincrementaldecoder("utf-8")(errors="replace")
        self._parts: list[str] = []
        self._lock = threading.Lock()
        self._stop = threading.Event()
        self.completed = threading.Event()
        self.outcome: str | None = None
        self.matched_marker: str | None = None
        self.first_rx_at: float | None = None
        self.completed_at: float | None = None
        self._echo = False
        self._thread = threading.Thread(target=self._read_loop, daemon=True)

    def start(self) -> None:
        self._thread.start()

    def enable_echo(self) -> None:
        """Print buffered output, then print newly received output live."""
        with self._lock:
            if not self._echo:
                sys.stdout.write("".join(self._parts))
                sys.stdout.flush()
                self._echo = True

    def stop(self) -> None:
        self._stop.set()
        self._thread.join(timeout=2.0)

        tail = self._decoder.decode(b"", final=True)
        if tail:
            self._consume(tail)

    def text(self) -> str:
        with self._lock:
            return "".join(self._parts)

    def _read_loop(self) -> None:
        while not self._stop.is_set():
            try:
                data = self._port.read(4096)
            except Exception as exc:  # The main thread reports this as a test failure.
                self._complete("FAIL", f"serial read error: {exc}")
                return

            if not data:
                continue

            if self.first_rx_at is None:
                self.first_rx_at = time.perf_counter()
            decoded = self._decoder.decode(data)
            if decoded:
                self._consume(decoded)

    def _consume(self, decoded: str) -> None:
        with self._lock:
            self._parts.append(decoded)
            if self._echo:
                sys.stdout.write(decoded)
                sys.stdout.flush()
            normalized = ANSI_ESCAPE.sub("", "".join(self._parts))

        if self.outcome is not None:
            return

        for marker in self._failure_markers:
            if marker in normalized:
                self._complete("FAIL", marker)
                return

        if self._pass_marker in normalized:
            self._complete("PASS", self._pass_marker)

    def _complete(self, outcome: str, marker: str) -> None:
        if self.outcome is None:
            self.outcome = outcome
            self.matched_marker = marker
            self.completed_at = time.perf_counter()
            self.completed.set()


def parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build, flash, and run STSEphyr samples as a hardware integration test."
    )
    parser.add_argument(
        "samples",
        nargs="*",
        metavar="SAMPLE",
        help="sample names to run (default: every application under samples/)",
    )
    parser.add_argument("--port", help="serial port, for example COM6 or /dev/ttyACM0")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD)
    parser.add_argument("--board", default=DEFAULT_BOARD)
    parser.add_argument("--shield", default=DEFAULT_SHIELD)
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT,
        help="seconds to wait for each sample's completion marker (default: %(default)s)",
    )
    parser.add_argument(
        "--build-root",
        type=Path,
        help="build directory root (default: <west workspace>/build/stsephyr-integration)",
    )
    parser.add_argument("--runner", help="Zephyr flash runner passed to west flash")
    parser.add_argument(
        "--flash-arg",
        action="append",
        default=[],
        help="additional argument passed to west flash; repeat as needed",
    )
    parser.add_argument(
        "--list-samples", action="store_true", help="list discovered samples and exit"
    )
    parser.add_argument(
        "--list-ports", action="store_true", help="list available serial ports and exit"
    )
    return parser.parse_args(argv)


def discover_samples(repo_root: Path) -> list[Sample]:
    samples_dir = repo_root / "samples"
    discovered: list[Sample] = []

    for source_dir in sorted(samples_dir.iterdir(), key=lambda path: path.name):
        if not source_dir.is_dir() or not (source_dir / "CMakeLists.txt").is_file():
            continue

        name = source_dir.name
        if name == "basic":
            discovered.append(
                Sample(
                    name=name,
                    source_dir=source_dir,
                    pass_marker="STSAFE-A120 echo successful",
                    failure_markers=FAILURE_MARKERS + ("<err> stsephyr_sample:",),
                )
            )
        else:
            discovered.append(
                Sample(name=name, source_dir=source_dir, pass_marker=f"PASS: {name}")
            )

    return discovered


def choose_samples(discovered: Sequence[Sample], requested: Sequence[str]) -> list[Sample]:
    if not requested:
        return list(discovered)

    by_name = {sample.name: sample for sample in discovered}
    unknown = sorted(set(requested) - by_name.keys())
    if unknown:
        raise ValueError(f"unknown sample(s): {', '.join(unknown)}")
    return [by_name[name] for name in requested]


def import_serial():
    try:
        import serial
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError(
            "pyserial is required; activate the Zephyr virtual environment or run "
            "'python -m pip install pyserial'"
        ) from exc
    return serial, list_ports


def describe_ports(list_ports) -> list[str]:
    ports = sorted(list_ports.comports(), key=lambda item: item.device)
    return [
        f"{item.device}: {item.description or 'unknown device'}"
        + (f" (VID:PID={item.vid:04X}:{item.pid:04X})" if item.vid and item.pid else "")
        for item in ports
    ]


def auto_detect_port(list_ports) -> str:
    ports = list(list_ports.comports())
    hints = ("st-link", "stlink", "nucleo", "stmicroelectronics", "virtual com port")
    candidates = [
        item
        for item in ports
        if item.vid == 0x0483
        or any(
            hint in " ".join(
                filter(None, (item.description, item.manufacturer, item.product, item.interface))
            ).lower()
            for hint in hints
        )
    ]

    if len(candidates) == 1:
        return candidates[0].device
    if not candidates and len(ports) == 1:
        return ports[0].device

    available = "\n".join(f"  {line}" for line in describe_ports(list_ports)) or "  (none)"
    if candidates:
        devices = ", ".join(item.device for item in candidates)
        raise RuntimeError(
            f"multiple possible ST-LINK serial ports found ({devices}); use --port\n{available}"
        )
    raise RuntimeError(f"could not find an ST-LINK serial port; use --port\n{available}")


def west_topdir(west_command: Sequence[str], repo_root: Path) -> Path:
    try:
        result = subprocess.run(
            [*west_command, "topdir"],
            cwd=repo_root,
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as exc:
        raise RuntimeError(f"cannot execute west: {exc}") from exc

    if result.returncode != 0:
        detail = (result.stderr or result.stdout).strip()
        raise RuntimeError(f"cannot locate the west workspace: {detail}")
    return Path(result.stdout.strip()).resolve()


def print_command(command: Sequence[str]) -> None:
    print(f"$ {subprocess.list2cmdline(list(command))}", flush=True)


def run_command(command: Sequence[str], cwd: Path) -> tuple[int, float]:
    print_command(command)
    started = time.perf_counter()
    try:
        completed = subprocess.run(command, cwd=cwd, check=False)
        return completed.returncode, time.perf_counter() - started
    except OSError as exc:
        print(f"ERROR: could not execute {command[0]}: {exc}", file=sys.stderr)
        return 127, time.perf_counter() - started


def run_sample(
    sample: Sample,
    *,
    west_command: Sequence[str],
    workspace_root: Path,
    build_root: Path,
    board: str,
    shield: str,
    port_name: str,
    baud: int,
    timeout: float,
    runner: str | None,
    flash_args: Sequence[str],
    serial_module,
) -> Result:
    overall_started = time.perf_counter()
    build_dir = build_root / sample.name
    build_command = [
        *west_command,
        "build",
        "-p",
        "always",
        "-d",
        str(build_dir),
        "-b",
        board,
        "--shield",
        shield,
        str(sample.source_dir),
    ]

    print(f"\n{'=' * 78}\n[{sample.name}] BUILD\n{'=' * 78}")
    build_status, build_seconds = run_command(build_command, workspace_root)
    if build_status != 0:
        return Result(
            sample.name,
            "FAIL",
            build_seconds,
            None,
            None,
            time.perf_counter() - overall_started,
            f"build exited with status {build_status}",
        )

    flash_command = [*west_command, "flash", "-d", str(build_dir)]
    if runner:
        flash_command.extend(("--runner", runner))
    flash_command.extend(flash_args)

    print(f"\n[{sample.name}] FLASH")
    print_command(flash_command)
    try:
        serial_port = serial_module.Serial(
            port=port_name,
            baudrate=baud,
            bytesize=serial_module.EIGHTBITS,
            parity=serial_module.PARITY_NONE,
            stopbits=serial_module.STOPBITS_ONE,
            timeout=0.05,
            write_timeout=1.0,
        )
    except Exception as exc:
        return Result(
            sample.name,
            "FAIL",
            build_seconds,
            None,
            None,
            time.perf_counter() - overall_started,
            f"cannot open {port_name}: {exc}",
        )

    capture = SerialCapture(serial_port, sample.pass_marker, sample.failure_markers)
    flash_started = time.perf_counter()
    try:
        serial_port.reset_input_buffer()
        capture.start()
        try:
            flashed = subprocess.run(flash_command, cwd=workspace_root, check=False)
            flash_status = flashed.returncode
        except OSError as exc:
            print(f"ERROR: could not execute west: {exc}", file=sys.stderr)
            flash_status = 127
        flash_seconds = time.perf_counter() - flash_started

        print(f"\n[{sample.name}] SERIAL ({port_name}, {baud} 8N1)")
        capture.enable_echo()

        if flash_status != 0:
            reason = f"flash exited with status {flash_status}"
            status = "FAIL"
        else:
            capture.completed.wait(timeout)
            if capture.outcome is None:
                reason = f"timed out after {timeout:g}s waiting for {sample.pass_marker!r}"
                status = "FAIL"
            elif capture.outcome == "FAIL":
                reason = f"device output contained {capture.matched_marker!r}"
                status = "FAIL"
            else:
                reason = ""
                status = "PASS"

        # Give the UART a short drain window for text immediately after the marker.
        time.sleep(0.2)
    finally:
        capture.stop()
        serial_port.close()

    serial_log = build_dir / "serial.log"
    try:
        serial_log.write_text(capture.text(), encoding="utf-8")
    except OSError as exc:
        print(f"WARNING: could not write {serial_log}: {exc}", file=sys.stderr)

    if capture.first_rx_at is not None and capture.completed_at is not None:
        run_seconds = max(0.0, capture.completed_at - capture.first_rx_at)
    else:
        run_seconds = None

    return Result(
        sample.name,
        status,
        build_seconds,
        flash_seconds,
        run_seconds,
        time.perf_counter() - overall_started,
        reason,
    )


def duration(value: float | None) -> str:
    return "--" if value is None else f"{value:.2f}s"


def print_summary(results: Sequence[Result]) -> None:
    headers = ("Sample", "Result", "Build", "Flash", "Run", "Total", "Details")
    rows = [
        (
            result.name,
            result.status,
            duration(result.build_seconds),
            duration(result.flash_seconds),
            duration(result.run_seconds),
            duration(result.total_seconds),
            result.reason,
        )
        for result in results
    ]
    widths = [
        max(len(headers[index]), *(len(row[index]) for row in rows))
        for index in range(len(headers))
    ]

    print(f"\n{'=' * 78}\nINTEGRATION TEST SUMMARY\n{'=' * 78}")
    print("  ".join(headers[index].ljust(widths[index]) for index in range(len(headers))))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print("  ".join(row[index].ljust(widths[index]) for index in range(len(row))))

    passed = sum(result.status == "PASS" for result in results)
    failed = len(results) - passed
    print(f"\n{passed} passed, {failed} failed, {len(results)} total")


def main(argv: Sequence[str] | None = None) -> int:
    args = parse_args(argv if argv is not None else sys.argv[1:])
    repo_root = Path(__file__).resolve().parents[1]
    discovered = discover_samples(repo_root)

    if args.list_samples:
        for sample in discovered:
            print(sample.name)
        return 0

    try:
        samples = choose_samples(discovered, args.samples)
    except ValueError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    try:
        serial_module, list_ports = import_serial()
        if args.list_ports:
            descriptions = describe_ports(list_ports)
            print("\n".join(descriptions) if descriptions else "No serial ports found.")
            return 0

        port_name = args.port or auto_detect_port(list_ports)
        if args.port is None:
            print(f"Auto-detected ST-LINK serial port: {port_name}")

        # Running West as a module keeps it on the same Python environment as
        # this script, including Zephyr's required Python packages.
        west_command = (sys.executable, "-m", "west")
        workspace_root = west_topdir(west_command, repo_root)
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 2

    build_root = (
        args.build_root.resolve()
        if args.build_root
        else workspace_root / "build" / "stsephyr-integration"
    )
    print(f"Running {len(samples)} sample(s) on {args.board} with shield {args.shield}")
    print(f"Build root: {build_root}")

    results: list[Result] = []
    try:
        for sample in samples:
            result = run_sample(
                sample,
                west_command=west_command,
                workspace_root=workspace_root,
                build_root=build_root,
                board=args.board,
                shield=args.shield,
                port_name=port_name,
                baud=args.baud,
                timeout=args.timeout,
                runner=args.runner,
                flash_args=args.flash_arg,
                serial_module=serial_module,
            )
            results.append(result)
            print(f"\n[{sample.name}] {result.status}: {result.reason or 'completion marker received'}")
    except KeyboardInterrupt:
        print("\nInterrupted by user.", file=sys.stderr)

    if results:
        print_summary(results)
    return 0 if len(results) == len(samples) and all(r.status == "PASS" for r in results) else 1


if __name__ == "__main__":
    raise SystemExit(main())
