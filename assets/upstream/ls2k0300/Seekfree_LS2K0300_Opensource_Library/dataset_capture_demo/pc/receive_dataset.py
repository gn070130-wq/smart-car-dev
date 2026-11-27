#!/usr/bin/env python3
"""Receive lossless 64x64 dataset images from the LS2K0300 capture demo."""

from __future__ import annotations

import argparse
import csv
import datetime as dt
import os
import socket
import struct
from pathlib import Path


HEADER = struct.Struct("!4sIIiHH")
MAGIC = b"DS01"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"
VALID_LABELS = ("supplies", "vehicle", "weapon")


def disable_windows_quick_edit() -> None:
    if os.name != "nt":
        return

    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        input_handle = kernel32.GetStdHandle(-10)  # STD_INPUT_HANDLE
        console_mode = ctypes.c_uint()
        if kernel32.GetConsoleMode(input_handle, ctypes.byref(console_mode)):
            enable_extended_flags = 0x0080
            enable_quick_edit_mode = 0x0040
            new_mode = (console_mode.value | enable_extended_flags) & ~enable_quick_edit_mode
            kernel32.SetConsoleMode(input_handle, new_mode)
    except (AttributeError, OSError):
        pass


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Receive LS2K0300 dataset images into a class directory."
    )
    parser.add_argument("--label", required=True, choices=VALID_LABELS)
    parser.add_argument("--output", type=Path, default=Path("dataset"))
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=8087)
    parser.add_argument(
        "--count",
        type=int,
        default=0,
        help="stop after N saved images; 0 means no limit",
    )
    args = parser.parse_args()
    if not 1 <= args.port <= 65535:
        parser.error("--port must be between 1 and 65535")
    if args.count < 0:
        parser.error("--count cannot be negative")
    return args


def recv_exact(connection: socket.socket, size: int) -> bytes | None:
    chunks: list[bytes] = []
    received = 0
    while received < size:
        chunk = connection.recv(size - received)
        if not chunk:
            if received == 0:
                return None
            raise ConnectionError(
                f"connection closed after {received}/{size} bytes"
            )
        chunks.append(chunk)
        received += len(chunk)
    return b"".join(chunks)


def validate_png(payload: bytes, width: int, height: int) -> None:
    if len(payload) < 24 or payload[:8] != PNG_SIGNATURE:
        raise ValueError("payload is not a PNG image")
    if payload[12:16] != b"IHDR":
        raise ValueError("PNG has no IHDR as its first chunk")
    png_width, png_height = struct.unpack("!II", payload[16:24])
    if (png_width, png_height) != (width, height):
        raise ValueError(
            f"header is {width}x{height}, PNG is {png_width}x{png_height}"
        )
    if (width, height) != (64, 64):
        raise ValueError(f"expected a 64x64 model input, got {width}x{height}")


def make_filename(session: str, sequence: int, yaw_millidegrees: int) -> str:
    sign = "p" if yaw_millidegrees >= 0 else "m"
    return (
        f"{session}_{sequence:06d}_yaw_"
        f"{sign}{abs(yaw_millidegrees):05d}.png"
    )


def append_manifest(
    manifest_path: Path,
    filename: str,
    label: str,
    sequence: int,
    yaw_millidegrees: int,
    received_at: str,
) -> None:
    needs_header = not manifest_path.exists() or manifest_path.stat().st_size == 0
    with manifest_path.open("a", newline="", encoding="utf-8") as manifest_file:
        writer = csv.writer(manifest_file)
        if needs_header:
            writer.writerow(
                ["filename", "label", "sequence", "yaw_degrees", "received_at"]
            )
        writer.writerow(
            [
                filename,
                label,
                sequence,
                f"{yaw_millidegrees / 1000.0:.3f}",
                received_at,
            ]
        )


def receive_frames(args: argparse.Namespace) -> int:
    label_dir = args.output / args.label
    label_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = args.output / "manifest.csv"
    session = dt.datetime.now().strftime("%Y%m%d_%H%M%S_%f")

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((args.bind, args.port))
        server.listen(1)
        print(f"Listening on {args.bind}:{args.port}; label={args.label}")
        connection, address = server.accept()

        with connection:
            print(f"Board connected from {address[0]}:{address[1]}")
            saved = 0
            try:
                while args.count == 0 or saved < args.count:
                    header_data = recv_exact(connection, HEADER.size)
                    if header_data is None:
                        print("Board disconnected")
                        break

                    magic, payload_size, sequence, yaw_md, width, height = (
                        HEADER.unpack(header_data)
                    )
                    if magic != MAGIC:
                        raise ValueError(f"bad frame magic: {magic!r}")
                    payload = recv_exact(connection, payload_size)
                    if payload is None:
                        raise ConnectionError("connection closed before PNG payload")
                    validate_png(payload, width, height)

                    received_at = dt.datetime.now(dt.timezone.utc).isoformat()
                    filename = make_filename(session, sequence, yaw_md)
                    image_path = label_dir / filename
                    image_path.write_bytes(payload)
                    append_manifest(
                        manifest_path,
                        str(Path(args.label) / filename),
                        args.label,
                        sequence,
                        yaw_md,
                        received_at,
                    )
                    saved += 1
                    limit_text = str(args.count) if args.count else "unlimited"
                    print(
                        f"Saved {saved}/{limit_text}: {image_path} "
                        f"(yaw={yaw_md / 1000.0:.2f} deg)"
                    )
            finally:
                try:
                    connection.sendall(b"S")
                except OSError:
                    pass

    return saved


def main() -> int:
    disable_windows_quick_edit()
    args = parse_args()
    try:
        saved = receive_frames(args)
    except KeyboardInterrupt:
        print("\nStopped by user")
        return 130
    except (ConnectionError, OSError, ValueError) as error:
        print(f"Receiver error: {error}")
        return 1

    print(f"Finished; saved {saved} image(s) under {args.output / args.label}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
