"""Background serial reader with auto-reconnect.

The reader runs in a daemon thread. It (re)opens the port whenever needed
and feeds bytes through the protocol :class:`FrameDecoder`. Decoded
:class:`Frame` instances (plus raw bytes and lifecycle events) are pushed
into a queue. The TUI consumes that queue from its own event loop.

Auto-reconnect: when the underlying read fails (typical after the USB
cable is unplugged on Windows, which yields ``PermissionError(13)``), the
reader closes the stale handle, waits a second, and tries to reopen the
port with the same name. The user does not have to restart the TUI.
"""

from __future__ import annotations

import queue
import threading
import time
from dataclasses import dataclass
from typing import Optional

import serial

from .protocol import Frame, FrameDecoder, encode_frame


@dataclass
class LinkEvent:
    """Anything that happens on the link, surfaced to the UI."""
    kind: str           # "frame" | "raw" | "open" | "close" | "error" | "reconnecting"
    frame: Optional[Frame] = None
    raw: bytes = b""
    message: str = ""


class SerialLink:
    """Owns the serial port and a background reader thread.

    Thread safety: ``send_frame`` / ``send_raw`` are called from the UI
    thread, the reader thread only reads. Pyserial's ``Serial`` is
    documented as thread-safe for one reader and one writer simultaneously
    on the same handle, which is exactly our pattern.
    """

    RECONNECT_DELAY_S = 1.0

    def __init__(self, port: str, baud: int = 115200) -> None:
        self.port_name = port
        self.baud = baud
        self.events: "queue.Queue[LinkEvent]" = queue.Queue()
        self._ser: Optional[serial.Serial] = None
        self._reader: Optional[threading.Thread] = None
        self._stop = threading.Event()
        self._ser_lock = threading.Lock()

    # ---- lifecycle --------------------------------------------------------

    def open(self) -> None:
        """Start the reader thread. The initial port open is attempted
        inside the thread so a missing device at startup does not crash
        the TUI."""
        self._stop.clear()
        self._reader = threading.Thread(target=self._reader_loop, daemon=True)
        self._reader.start()

    def close(self) -> None:
        self._stop.set()
        if self._reader is not None:
            self._reader.join(timeout=2.0)
            self._reader = None
        with self._ser_lock:
            if self._ser is not None:
                try:
                    self._ser.close()
                except Exception:
                    pass
                self._ser = None
        self.events.put(LinkEvent(kind="close",
                                  message=f"closed {self.port_name}"))

    # ---- TX ---------------------------------------------------------------

    def send_frame(self, msg_type: int, payload: bytes = b"") -> None:
        with self._ser_lock:
            ser = self._ser
            if ser is None:
                raise RuntimeError("serial port not open")
            ser.write(encode_frame(msg_type, payload))

    def send_raw(self, data: bytes) -> None:
        with self._ser_lock:
            ser = self._ser
            if ser is None:
                raise RuntimeError("serial port not open")
            ser.write(data)

    # ---- reader thread ----------------------------------------------------

    def _open_port(self) -> bool:
        """Try to (re)open the serial port. Returns True on success."""
        try:
            ser = serial.Serial(
                self.port_name,
                self.baud,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                timeout=0.1,
            )
        except Exception as exc:
            self.events.put(LinkEvent(
                kind="error",
                message=f"cannot open {self.port_name}: {exc}"))
            return False
        with self._ser_lock:
            self._ser = ser
        self.events.put(LinkEvent(
            kind="open",
            message=f"opened {self.port_name} @ {self.baud}"))
        return True

    def _drop_port(self) -> None:
        """Close the current handle (if any) and clear it."""
        with self._ser_lock:
            ser = self._ser
            self._ser = None
        if ser is not None:
            try:
                ser.close()
            except Exception:
                pass

    def _reader_loop(self) -> None:
        decoder = FrameDecoder()
        last_error: Optional[str] = None
        while not self._stop.is_set():
            # (Re)connect phase: hold here until the port opens.
            if self._ser is None:
                if not self._open_port():
                    if last_error != "open_fail":
                        self.events.put(LinkEvent(
                            kind="reconnecting",
                            message=f"retrying {self.port_name} in "
                                    f"{self.RECONNECT_DELAY_S:.0f} s ..."))
                        last_error = "open_fail"
                    self._sleep_interruptible(self.RECONNECT_DELAY_S)
                    continue
                # Fresh open succeeded — reset decoder so we don't carry a
                # half-parsed frame across the disconnect.
                decoder.reset()
                last_error = None

            # Read phase.
            try:
                chunk = self._ser.read(256)   # type: ignore[union-attr]
            except (serial.SerialException, OSError, PermissionError) as exc:
                if last_error != "read_fail":
                    self.events.put(LinkEvent(
                        kind="error",
                        message=f"link lost ({exc.__class__.__name__}); "
                                f"will auto-reconnect"))
                    last_error = "read_fail"
                self._drop_port()
                self._sleep_interruptible(self.RECONNECT_DELAY_S)
                continue
            except Exception as exc:
                # Anything unexpected — log once, retry.
                if last_error != "unknown_fail":
                    self.events.put(LinkEvent(
                        kind="error", message=f"unexpected read error: {exc}"))
                    last_error = "unknown_fail"
                self._drop_port()
                self._sleep_interruptible(self.RECONNECT_DELAY_S)
                continue

            if not chunk:
                continue

            # Healthy read.
            last_error = None
            self.events.put(LinkEvent(kind="raw", raw=chunk))
            for byte in chunk:
                frame = decoder.feed(byte)
                if frame is not None:
                    self.events.put(LinkEvent(kind="frame", frame=frame))

    def _sleep_interruptible(self, seconds: float) -> None:
        """Sleep but wake up early if ``stop`` is set."""
        self._stop.wait(timeout=seconds)
