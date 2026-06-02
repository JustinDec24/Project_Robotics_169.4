"""Entry point: ``python -m modem_console --port COM4``."""

from __future__ import annotations

import argparse
import sys

import serial.tools.list_ports

from .serial_link import SerialLink
from .tui import ModemConsoleApp


def _list_ports() -> None:
    ports = sorted(serial.tools.list_ports.comports(), key=lambda p: p.device)
    if not ports:
        print("No serial ports detected.")
        return
    print("Available serial ports:")
    for p in ports:
        desc = f" — {p.description}" if p.description else ""
        print(f"  {p.device}{desc}")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(
        prog="modem_console",
        description="TUI console for the RF169 modem (PC side).",
    )
    parser.add_argument("--port", "-p",
                        help="Serial port (e.g. COM4 on Windows, "
                             "/dev/ttyUSB0 on Linux).")
    parser.add_argument("--baud", "-b", type=int, default=115200,
                        help="Baud rate (default: 115200, must match firmware "
                             "UART_BAUD_DEFAULT).")
    parser.add_argument("--list", action="store_true",
                        help="List available serial ports and exit.")
    args = parser.parse_args(argv)

    if args.list:
        _list_ports()
        return 0

    if not args.port:
        print("error: --port is required (use --list to see available ports).",
              file=sys.stderr)
        return 2

    link = SerialLink(args.port, args.baud)
    app = ModemConsoleApp(link)
    app.run()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
