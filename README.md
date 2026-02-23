# Project_Robotics_169.4
Project about radio communication between pc and robot

## Firmware — Remote Node

The `firmware/remote/` directory contains the Remote node firmware skeleton
(PC-connected controller) for a **169.4 MHz RF link** using a
**TI CC1120** transceiver controlled by a **PIC microcontroller**.

### Data flow

```
PC <-> UART <-> PIC <-> SPI <-> CC1120 <-> RF <-> Robot
```

### File tree

```
firmware/remote/
├── main.c                    Entry point + super-loop
├── config.h                  Build-time constants (NET_ID, timing, sizes)
├── app/
│   ├── app.h                 State machine declarations
│   └── app.c                 Remote-node state machine + bring-up flow
├── board/
│   ├── board.h               Hardware abstraction API (pin mapping lives here)
│   └── board.c               PIC-specific stubs — the ONLY file to edit for porting
├── drivers/
│   ├── uart.h / uart.c       Interrupt-driven UART (RX ring buffer)
│   ├── spi.h  / spi.c        SPI master driver
│   └── timer.h / timer.c     1 ms tick + deadline helpers
├── radio/
│   ├── cc1120_regs.h         CC1120 register addresses + command strobes
│   ├── cc1120.h / cc1120.c   Low-level SPI commands, FIFO, reset sequence
│   └── radio_link.h / .c     Packet format, TX/RX, ACK, duty-cycle limiter
├── protocol/
│   ├── protocol.h / .c       UART binary framing (PC <-> Remote)
└── util/
    ├── ringbuf.h             Generic ring buffer (header-only)
    └── crc8.h / crc8.c       CRC-8 for UART framing
```

### Getting started

1. Create an MPLAB X project, select your PIC, and add all `.c` files as sources
   and all `.h` files to the include path (`firmware/remote/`).
2. Fill in `board/board.c` with your PIC's register accesses (pin init, UART,
   SPI, timer, ISR wiring). See the TODO comments.
3. Export a register table from **SmartRF Studio** for 169.4 MHz and paste the
   values into `cc1120_init_minimal()` in `radio/cc1120.c`.
4. Build, flash, connect a serial terminal at 9600 baud.  You should see
   `REMOTE: boot` / `REMOTE: UART ok` / `REMOTE: RF init ok` log messages.
