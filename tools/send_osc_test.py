#!/usr/bin/env python3
"""Send a simple OSC message to the local XR receiver.

This helper uses only Python's standard library so it can run without
installing third-party packages.
"""

from __future__ import annotations

import argparse
import socket
import struct
import time


def _pad_osc_string(value: str) -> bytes:
    encoded = value.encode("utf-8") + b"\x00"
    padding = (4 - (len(encoded) % 4)) % 4
    return encoded + (b"\x00" * padding)


def _encode_argument(argument_type: str, value: str) -> tuple[str, bytes]:
    if argument_type == "int":
        return "i", struct.pack(">i", int(value))
    if argument_type == "float":
        return "f", struct.pack(">f", float(value))
    if argument_type == "string":
        return "s", _pad_osc_string(value)
    raise ValueError(f"Unsupported argument type: {argument_type}")


def build_osc_message(address: str, argument_type: str, value: str) -> bytes:
    if not address.startswith("/"):
        raise ValueError("OSC address must start with '/'.")

    type_tag, encoded_argument = _encode_argument(argument_type, value)
    return (
        _pad_osc_string(address)
        + _pad_osc_string(f",{type_tag}")
        + encoded_argument
    )


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Send a single OSC message to an XRSceneTriggerController receiver."
    )
    parser.add_argument(
        "value",
        nargs="?",
        default="3",
        help="Argument value to send. Defaults to 3.",
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="Target host. Defaults to 127.0.0.1.",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=7000,
        help="Target UDP port. Defaults to 7000.",
    )
    parser.add_argument(
        "--address",
        default="/xr/trigger",
        help="OSC address path. Defaults to /xr/trigger.",
    )
    parser.add_argument(
        "--type",
        choices=("int", "float", "string"),
        default="int",
        help="OSC argument type. Defaults to int.",
    )
    parser.add_argument(
        "--repeat",
        type=int,
        default=1,
        help="How many times to send the message. Defaults to 1.",
    )
    parser.add_argument(
        "--interval",
        type=float,
        default=0.25,
        help="Seconds to wait between repeated sends. Defaults to 0.25.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    payload = build_osc_message(args.address, args.type, args.value)

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as udp_socket:
        for index in range(args.repeat):
            udp_socket.sendto(payload, (args.host, args.port))
            print(
                f"[{index + 1}/{args.repeat}] sent {args.address} "
                f"{args.type}={args.value} to {args.host}:{args.port}"
            )
            if index + 1 < args.repeat:
                time.sleep(args.interval)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
