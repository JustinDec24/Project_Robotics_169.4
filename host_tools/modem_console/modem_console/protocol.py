"""UART protocol codec — mirror of firmware/modem/protocol/protocol.[ch].

Frame format on the wire:

    [0xAA][0x55][LEN][TYPE][PAYLOAD...][CRC8]

* LEN  = 1 (TYPE byte) + payload bytes count
* CRC8 = polynomial 0x07, init 0x00, computed over [LEN, TYPE, PAYLOAD...]
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import Optional, Tuple


# ---- Frame constants ------------------------------------------------------

SYNC1 = 0xAA
SYNC2 = 0x55
MAX_PAYLOAD = 96   # MAX_UART_PAYLOAD in firmware/modem/config.h


# ---- Message types --------------------------------------------------------

class MsgType(IntEnum):
    # PC -> Modem
    SCAN_START   = 0x01
    SCAN_STOP    = 0x02
    CONNECT      = 0x03
    DISCONNECT   = 0x04
    GET_STATS    = 0x05
    DATA_TX      = 0x10

    # Modem -> PC
    SCAN_RESULT  = 0x81
    CONNECTED    = 0x82
    DISCONNECTED = 0x83
    STATS        = 0x84
    TX_ACK       = 0x85
    DATA_RX      = 0x90
    LOG          = 0x9F


# ---- CRC-8 (polynomial 0x07, init 0x00) ----------------------------------

def crc8_update(crc: int, byte_val: int) -> int:
    crc ^= byte_val
    for _ in range(8):
        if crc & 0x80:
            crc = ((crc << 1) ^ 0x07) & 0xFF
        else:
            crc = (crc << 1) & 0xFF
    return crc


def crc8(data: bytes) -> int:
    crc = 0
    for b in data:
        crc = crc8_update(crc, b)
    return crc


# ---- Frame encoding -------------------------------------------------------

def encode_frame(msg_type: int, payload: bytes = b"") -> bytes:
    """Build a complete protocol frame ready to write to the serial port."""
    if len(payload) > MAX_PAYLOAD:
        raise ValueError(f"payload too long ({len(payload)} > {MAX_PAYLOAD})")

    len_field = 1 + len(payload)            # TYPE + payload bytes
    crc_input = bytes([len_field, msg_type]) + payload
    return bytes([SYNC1, SYNC2]) + crc_input + bytes([crc8(crc_input)])


# ---- Frame decoding (byte-by-byte state machine) -------------------------

class _State(IntEnum):
    SYNC1 = 0
    SYNC2 = 1
    LEN   = 2
    DATA  = 3
    CRC   = 4


@dataclass
class Frame:
    msg_type: int
    payload: bytes


class FrameDecoder:
    """Feed received bytes one by one; yields a Frame when one is complete."""

    def __init__(self) -> None:
        self.reset()

    def reset(self) -> None:
        self._state = _State.SYNC1
        self._len_field = 0
        self._buf = bytearray()
        self._idx = 0

    def feed(self, byte_in: int) -> Optional[Frame]:
        b = byte_in & 0xFF

        if self._state == _State.SYNC1:
            if b == SYNC1:
                self._state = _State.SYNC2

        elif self._state == _State.SYNC2:
            if b == SYNC2:
                self._state = _State.LEN
            elif b == SYNC1:
                # stay in SYNC2 (we just saw another candidate first byte)
                pass
            else:
                self._state = _State.SYNC1

        elif self._state == _State.LEN:
            if b == 0 or b > MAX_PAYLOAD + 1:
                self._state = _State.SYNC1
            else:
                self._len_field = b
                self._buf = bytearray()
                self._idx = 0
                self._state = _State.DATA

        elif self._state == _State.DATA:
            self._buf.append(b)
            self._idx += 1
            if self._idx >= self._len_field:
                self._state = _State.CRC

        elif self._state == _State.CRC:
            crc = crc8_update(0, self._len_field)
            for byte in self._buf:
                crc = crc8_update(crc, byte)
            self._state = _State.SYNC1
            if crc == b:
                msg_type = self._buf[0]
                payload = bytes(self._buf[1:])
                return Frame(msg_type=msg_type, payload=payload)
            # CRC mismatch: drop silently, resync on next SYNC1

        return None

    def feed_many(self, data: bytes) -> Tuple[Frame, ...]:
        out = []
        for byte in data:
            frame = self.feed(byte)
            if frame is not None:
                out.append(frame)
        return tuple(out)
