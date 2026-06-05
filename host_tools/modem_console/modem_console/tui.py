"""Textual TUI for the modem console."""

from __future__ import annotations

import shlex
import time
from dataclasses import dataclass, field
from typing import Dict, Optional

from textual.app import App, ComposeResult
from textual.binding import Binding
from textual.containers import Horizontal, Vertical
from textual.widgets import (
    DataTable,
    Footer,
    Header,
    Input,
    RichLog,
    Static,
)

from .protocol import Frame, MsgType
from .serial_link import LinkEvent, SerialLink


# ---- Connection state ----------------------------------------------------

@dataclass
class LinkStats:
    rssi_avg_dbm: Optional[int] = None
    per_pct: Optional[int] = None
    rtt_ms: Optional[int] = None


@dataclass
class RobotState:
    connected: bool = False
    robot_id: Optional[int] = None
    last_tx_at: Optional[float] = None
    last_rx_at: Optional[float] = None
    tx_count: int = 0
    rx_count: int = 0
    stats: LinkStats = field(default_factory=LinkStats)


@dataclass
class DiscoveredRobot:
    robot_id: int
    rssi_dbm: int
    age_100ms: int
    first_seen: float


# ---- Widgets -------------------------------------------------------------

class StatusPanel(Static):
    """Left-top: connection state to the remote modem and the robot."""

    def __init__(self) -> None:
        super().__init__(id="status-panel")
        self.border_title = "Connection"

    def render_state(self,
                     port: str, baud: int,
                     state: RobotState) -> None:
        if state.connected:
            conn_line = f"[bold green]● CONNECTED[/]  robot 0x{state.robot_id:02X}"
        else:
            conn_line = "[bold red]○ disconnected[/]"
        rssi = (f"{state.stats.rssi_avg_dbm} dBm"
                if state.stats.rssi_avg_dbm is not None else "—")
        per = (f"{state.stats.per_pct} %"
               if state.stats.per_pct is not None else "—")
        rtt = (f"{state.stats.rtt_ms} ms"
               if state.stats.rtt_ms is not None else "—")
        last_tx = _fmt_age(state.last_tx_at)
        last_rx = _fmt_age(state.last_rx_at)
        text = (
            f"{conn_line}\n\n"
            f"[dim]RSSI avg[/]   {rssi}\n"
            f"[dim]PER[/]        {per}\n"
            f"[dim]RTT[/]        {rtt}\n"
            f"[dim]TX count[/]   {state.tx_count}    "
            f"[dim]RX count[/] {state.rx_count}\n"
            f"[dim]Last TX[/]    {last_tx}\n"
            f"[dim]Last RX[/]    {last_rx}\n\n"
            f"[dim]Port[/]       {port} @ {baud}"
        )
        self.update(text)


class DiscoveryPanel(DataTable):
    """Left-bottom: robots discovered through SCAN.

    Simple strategy: rebuild the whole table on each update. With
    MAX_DISCOVERED_ROBOTS == 8 in the firmware, the cost is negligible.
    """

    def __init__(self) -> None:
        super().__init__(id="discovery-panel")
        self.cursor_type = "row"
        self.zebra_stripes = True
        self.border_title = "Discovered robots"

    def on_mount(self) -> None:
        self.add_columns("ID", "RSSI (dBm)", "Age (s)")

    def refresh_robots(self, robots: Dict[int, DiscoveredRobot]) -> None:
        self.clear()
        for robot in sorted(robots.values(), key=lambda r: r.robot_id):
            self.add_row(
                f"0x{robot.robot_id:02X}",
                str(robot.rssi_dbm),
                f"{robot.age_100ms / 10.0:.1f}",
            )


class EventLog(RichLog):
    """Right: scrolling event/log feed."""

    def __init__(self) -> None:
        super().__init__(id="event-log",
                         highlight=False,
                         markup=True,
                         wrap=True,
                         auto_scroll=True)
        self.border_title = "Events"

    def add(self, tag: str, text: str, color: str = "white") -> None:
        ts = time.strftime("%H:%M:%S")
        self.write(f"[dim]{ts}[/] [{color}]{tag:<7}[/] {text}")


# ---- App -----------------------------------------------------------------

class ModemConsoleApp(App):
    CSS = """
    Screen {
        layout: vertical;
    }
    #top-row {
        height: 1fr;
    }
    #left-col {
        width: 40%;
        min-width: 36;
    }
    #status-panel {
        border: round $primary;
        padding: 1 2;
        height: auto;
        min-height: 12;
    }
    #discovery-panel {
        border: round $secondary;
        height: 1fr;
    }
    #event-log {
        border: round $accent;
        width: 1fr;
    }
    #cmd-input {
        dock: bottom;
        margin: 0 0 0 0;
        border: round $warning;
    }
    """

    BINDINGS = [
        Binding("ctrl+c", "quit", "Quit", priority=True),
        Binding("ctrl+l", "clear_log", "Clear log"),
    ]

    def __init__(self, link: SerialLink) -> None:
        super().__init__()
        self.link = link
        self.state = RobotState()
        self.discovered: Dict[int, DiscoveredRobot] = {}

    # ---- layout ---------------------------------------------------------

    def compose(self) -> ComposeResult:
        yield Header(show_clock=True)
        with Horizontal(id="top-row"):
            with Vertical(id="left-col"):
                yield StatusPanel()
                yield DiscoveryPanel()
            yield EventLog()
        yield Input(placeholder=("> command  "
                                 "(help / connect <id> / disconnect / "
                                 "send <text> / passthrough / clear / quit)"),
                    id="cmd-input")
        yield Footer()

    def on_mount(self) -> None:
        self.title = "RF169 Modem Console"
        self.sub_title = f"{self.link.port_name} @ {self.link.baud}"
        self._refresh_status()
        # Poll the link queue periodically. Textual's `set_interval`
        # keeps everything on the main UI thread, which is what we want.
        self.set_interval(0.05, self._drain_link_events)
        self._log_event("INFO", f"Connecting to {self.link.port_name} ...", "cyan")
        try:
            self.link.open()
        except Exception as exc:
            self._log_event("ERROR", f"Could not open port: {exc}", "red")

    def on_unmount(self) -> None:
        self.link.close()

    # ---- command dispatch ---------------------------------------------

    def on_input_submitted(self, event: Input.Submitted) -> None:
        line = event.value.strip()
        event.input.value = ""
        if not line:
            return
        self._log_event("CMD", f"> {line}", "yellow")
        try:
            parts = shlex.split(line)
        except ValueError as exc:
            self._log_event("ERROR", f"parse error: {exc}", "red")
            return
        if not parts:
            return
        cmd = parts[0].lower()
        args = parts[1:]
        handler = {
            "help":        self._cmd_help,
            "connect":     self._cmd_connect,
            "disconnect":  self._cmd_disconnect,
            "send":        self._cmd_send,
            "passthrough": self._cmd_passthrough,
            "clear":       self._cmd_clear,
            "quit":        self._cmd_quit,
            "exit":        self._cmd_quit,
        }.get(cmd)
        if handler is None:
            self._log_event("ERROR", f"unknown command '{cmd}' (try help)", "red")
            return
        try:
            handler(args)
        except Exception as exc:
            self._log_event("ERROR", f"{cmd}: {exc}", "red")

    def _cmd_help(self, _args) -> None:
        for line in [
            "connect <id>      - connect to robot id (e.g. 0x10 or 16)",
            "disconnect        - drop the current connection",
            'send "<text>"     - send a data frame to the connected robot',
            "passthrough       - switch the modem to raw shell-bridge mode",
            "clear             - clear the event log",
            "quit              - exit the console (Ctrl+C also works)",
        ]:
            self._log_event("HELP", line, "magenta")

    def _cmd_connect(self, args) -> None:
        if not args:
            raise ValueError("usage: connect <id>")
        robot_id = int(args[0], 0)
        if not 0 <= robot_id <= 0xFF:
            raise ValueError("id must be 0..255")
        self.link.send_frame(MsgType.CONNECT, bytes([robot_id]))
        self._log_event("TX", f"CONNECT id=0x{robot_id:02X}", "blue")

    def _cmd_disconnect(self, _args) -> None:
        self.link.send_frame(MsgType.DISCONNECT)
        self._log_event("TX", "DISCONNECT", "blue")

    def _cmd_send(self, args) -> None:
        if not args:
            raise ValueError('usage: send "<text>"')
        text = " ".join(args)
        payload = text.encode("utf-8")
        self.link.send_frame(MsgType.DATA_TX, payload)
        self.state.tx_count += 1
        self.state.last_tx_at = time.time()
        self._refresh_status()
        self._log_event("TX", f"DATA_TX  \"{text}\"  ({len(payload)} B)", "blue")

    def _cmd_passthrough(self, _args) -> None:
        # Hand the link to a raw terminal session. After this command the
        # modem stops framing UART traffic — every byte that arrives on the
        # serial port becomes RF DATA, and every RF DATA payload from the
        # robot is written back as raw bytes. The TUI is therefore useless
        # from here on; the user is expected to close it and open PuTTY /
        # minicom on the same COM port. Only a power-cycle / reset of the
        # modem exits passthrough mode.
        self.link.send_frame(MsgType.PASSTHROUGH)
        self._log_event("TX", "PASSTHROUGH", "blue")
        self._log_event(
            "INFO",
            "Modem is now in raw shell-bridge mode.", "bold yellow")
        self._log_event(
            "INFO",
            "Close this TUI and open PuTTY/minicom on the same port "
            f"({self.link.port_name} @ {self.link.baud}).",
            "bold yellow")
        self._log_event(
            "INFO",
            "Power-cycle / reset the modem to exit passthrough.",
            "bold yellow")

    def _cmd_clear(self, _args) -> None:
        self.query_one(EventLog).clear()

    def _cmd_quit(self, _args) -> None:
        self.exit()

    def action_clear_log(self) -> None:
        self.query_one(EventLog).clear()

    # ---- link events ---------------------------------------------------

    def _drain_link_events(self) -> None:
        while not self.link.events.empty():
            try:
                evt = self.link.events.get_nowait()
            except Exception:
                break
            self._handle_link_event(evt)

    def _handle_link_event(self, evt: LinkEvent) -> None:
        if evt.kind == "open":
            self._log_event("LINK", evt.message, "green")
        elif evt.kind == "close":
            self._log_event("LINK", evt.message, "yellow")
        elif evt.kind == "error":
            self._log_event("ERROR", evt.message, "red")
        elif evt.kind == "raw":
            # Heuristic: only show chunks that look like real text (boot
            # banners, debug prints). Skip chunks that are mostly binary
            # because they are probably the byte-level view of a framed
            # message that will also surface as a parsed "frame" event.
            if _looks_like_text(evt.raw):
                stripped = _safe_ascii(evt.raw).strip()
                if stripped:
                    self._log_event("UART", stripped, "grey50")
        elif evt.kind == "frame" and evt.frame is not None:
            self._handle_frame(evt.frame)

    def _handle_frame(self, frame: Frame) -> None:
        t = frame.msg_type
        p = frame.payload
        if t == MsgType.SCAN_RESULT and len(p) >= 3:
            robot_id, rssi_raw, age = p[0], p[1], p[2]
            rssi = _decode_rssi(rssi_raw)
            first_time = robot_id not in self.discovered
            self.discovered[robot_id] = DiscoveredRobot(
                robot_id=robot_id, rssi_dbm=rssi, age_100ms=age,
                first_seen=time.time())
            self.query_one(DiscoveryPanel).refresh_robots(self.discovered)
            # Beacons arrive every 500 ms — 2 SCAN_RESULTs per second.
            # The Discovered-robots panel already shows the live state, so
            # only log to the Events feed when we first hear from a robot.
            # Every subsequent beacon updates the panel silently.
            if first_time:
                self._log_event("RX",
                                f"discovered robot 0x{robot_id:02X} "
                                f"rssi={rssi} dBm",
                                "cyan")
        elif t == MsgType.CONNECTED and len(p) >= 1:
            self.state.connected = True
            self.state.robot_id = p[0]
            self._refresh_status()
            self._log_event("RX",
                            f"CONNECTED to robot 0x{p[0]:02X}",
                            "green")
        elif t == MsgType.DISCONNECTED and len(p) >= 1:
            self.state.connected = False
            self.state.robot_id = None
            self._refresh_status()
            self._log_event("RX",
                            f"DISCONNECTED reason=0x{p[0]:02X}",
                            "yellow")
        elif t == MsgType.STATS and len(p) >= 4:
            # The Connection status panel already shows RSSI / PER / RTT live —
            # don't echo every 500 ms STATS frame to the Events log.
            rssi_avg = _decode_rssi(p[0])
            per = p[1]
            rtt = p[2] | (p[3] << 8)
            self.state.stats = LinkStats(rssi_avg_dbm=rssi_avg,
                                         per_pct=per,
                                         rtt_ms=rtt)
            self._refresh_status()
        elif t == MsgType.TX_ACK:
            # Robot acknowledged our last DATA frame — surface a clean
            # "delivered" confirmation so the user knows the send went
            # through. No payload, the event itself is the signal.
            self._log_event("ACK", "robot received last send", "bold green")
        elif t == MsgType.DATA_RX:
            self.state.rx_count += 1
            self.state.last_rx_at = time.time()
            self._refresh_status()
            self._log_event("RX",
                            f"DATA \"{_safe_ascii(p)}\"  ({len(p)} B)",
                            "bold green")
        elif t == MsgType.LOG:
            self._log_event("LOG", _safe_ascii(p), "white")
        else:
            self._log_event("RX",
                            f"unknown type 0x{t:02X}  payload={p.hex(' ')}",
                            "magenta")

    # ---- helpers -------------------------------------------------------

    def _refresh_status(self) -> None:
        try:
            panel = self.query_one(StatusPanel)
        except Exception:
            return
        panel.render_state(self.link.port_name, self.link.baud, self.state)

    def _log_event(self, tag: str, text: str, color: str = "white") -> None:
        try:
            self.query_one(EventLog).add(tag, text, color)
        except Exception:
            pass


# ---- pure helpers --------------------------------------------------------

#: dBm offset applied to the chip's raw RSSI to get an absolute power
#: reading. From TI document TIDU512 (CC1120 169 MHz reference design):
#: "RSSI Offset of CC1120 = -102 dBm". Used both for instantaneous RSSI
#: in SCAN_RESULT and for the smoothed average in STATS.
CC1120_RSSI_OFFSET_DBM = -102


def _decode_rssi(raw: int) -> int:
    """Convert the CC1120's signed-8 raw RSSI to an absolute dBm value."""
    signed = raw - 256 if raw & 0x80 else raw
    return signed + CC1120_RSSI_OFFSET_DBM


def _safe_ascii(data: bytes) -> str:
    """Return a printable representation; non-printable bytes -> '.'."""
    return "".join(chr(b) if 0x20 <= b < 0x7F else "." for b in data)


def _looks_like_text(data: bytes) -> bool:
    """Heuristic: True if the buffer is most likely printable text rather
    than the binary view of a framed protocol message.

    Framed messages start with 0xAA 0x55 (SYNC1, SYNC2). Even when they
    contain a few accidentally printable bytes they will be majority
    non-printable. We require at least 75 % of bytes to be printable
    ASCII (or CR/LF) for the chunk to be surfaced as UART text."""
    if not data:
        return False
    if data[0] == 0xAA:
        return False
    printable = sum(1 for b in data
                    if 0x20 <= b < 0x7F or b in (0x0A, 0x0D, 0x09))
    return printable / len(data) >= 0.75


def _fmt_age(ts: Optional[float]) -> str:
    if ts is None:
        return "—"
    dt = time.time() - ts
    if dt < 60:
        return f"{dt:.1f} s ago"
    if dt < 3600:
        return f"{dt/60:.1f} min ago"
    return f"{dt/3600:.1f} h ago"
