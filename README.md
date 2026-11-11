A project built from the ground up: developing custom libraries, writing peripheral drivers, and applying advanced programming techniques—such as the embedded structure patterns found in the Linux kernel.

Object-oriented programming implemented in C++.

The APIs have been verified on actual hardware; this repository is currently built for the STM32F401RET6 microcontroller.
Implementation of advanced communication techniques, including DMA and interrupts.

## Project chapters

This project uses a bare-metal, register-level design for the STM32F401RE. Peripheral registers are represented by C++ structs mapped to their memory addresses; the drivers configure those registers directly without HAL or Cube-generated startup code.

### Chapter 1 - Register-level foundation

[`Core/Inc/define.h`](Core/Inc/define.h) defines fixed-width types, peripheral base addresses, register layouts, bit/pin helpers, and UART/DMA/NVIC identifiers. The driver accesses RCC, GPIO, USART, DMA, SysTick, and NVIC registers through `volatile` pointers so reads and writes reach the hardware.

The project targets a 16 MHz core clock (`FREQ`). The linker memory map and bare-metal reset/startup flow are described in Chapter 9.

### Chapter 2 - SysTick time base

[`systick_init()`](Core/Src/Uart.cpp) configures SysTick to generate an interrupt every 1 ms when called with `16000000 / 1000`. [`SysTick_Handler()`](Core/Src/main.cpp) increments the shared `s_ticks` counter. The `timer_expired()` helper uses that tick count to schedule work; the current application checks for received UART data every 500 ms.

### Chapter 3 - GPIO and alternate functions

The `GPIO` helpers in [`Core/Inc/Uart.h`](Core/Inc/Uart.h) set a pin's mode, select an alternate-function number, and write its output using the GPIO `BSRR` register. UART setup selects the MCU pins and alternate functions required to connect them to each USART:

| USART | TX pin | RX pin | Alternate function |
|---|---|---|---|
| USART1 | PA9 | PA10 | AF7 |
| USART2 | PA2 | PA3 | AF7 |
| USART6 | PA11 | PA12 | AF8 |

### Chapter 4 - UART operation and polling helpers

[`DriverUART::uart_init()`](Core/Src/Uart.cpp) enables the selected USART clock, configures its TX/RX pins, calculates the baud-rate register value from the 16 MHz clock, and enables the USART, transmitter, and receiver. The application initializes USART1 and USART2 at 115200 baud.

The driver provides register-level status and byte helpers for checking RX-ready, TX-ready, transmission-complete, and UART IDLE conditions. `uart_write_byte_for_polling()` and `uart_write_buf()` provide blocking transmit helpers. These are driver APIs; the current application's main data path uses DMA rather than polling each byte.

### Chapter 5 - UART and NVIC interrupts

The driver contains helpers to enable/disable USART RXNE and TXE interrupts and to enable USART IRQs in the NVIC. The vector table in [`Core/Src/main.cpp`](Core/Src/main.cpp) maps USART1 and USART2 to their handlers.

In the current application, USART1's IDLE interrupt marks the end of a received burst and passes the newly received DMA data to the RX FIFO. A USART2 TXE handler is also present, but its byte source (`receivedByteDMA`) is not populated by the active application path. The earlier byte-at-a-time interrupt forwarding code is commented out, so this is not currently an active USART1-to-USART2 transfer.

### Chapter 6 - DMA reception and transmission

The active DMA path uses USART1 with DMA2:

- **RX: DMA2 Stream 5.** The driver configures circular, byte-wide peripheral-to-memory transfers into a 128-byte DMA buffer. Half-transfer, transfer-complete, and USART IDLE events move newly received bytes into the 1024-byte RX FIFO. The main loop reads available data from this FIFO.
- **TX: DMA2 Stream 7.** Application data is queued in a 1024-byte TX FIFO. The driver copies up to 128 bytes at a time into a DMA TX buffer and starts a memory-to-peripheral transfer to USART1. The transfer-complete handler clears the DMA status and starts the next queued chunk.

The DMA buffer sizes and FIFO sizes are defined in [`Core/Inc/Uart.h`](Core/Inc/Uart.h); configuration, ISR callbacks, and queue handling are implemented in [`Core/Src/Uart.cpp`](Core/Src/Uart.cpp).

### Chapter 7 - Application data flow and two USARTs

The current `main()` initializes USART1 and USART2 at 115200 baud, configures DMA RX/TX for USART1, and then repeatedly checks the RX FIFO. Every 500 ms, if data is available, the application queues the same bytes for USART1 transmission and starts DMA TX. In other words, the active demo is a **USART1 DMA echo**.

USART2 initialization and its interrupt handler demonstrate additional driver support, but the current application does not route received USART1 bytes to USART2 or receive bytes on USART2. A two-USART forwarding example would need to connect the USART1 RX data path to USART2's TX path and configure the required USART2 transmit mechanism; that end-to-end behavior is not implemented in the current `main()`.

### Chapter 8 - FIFO buffering and interrupt safety

[`FIFO`](Core/Inc/fifo.h) and [`Core/Src/fifo.cpp`](Core/Src/fifo.cpp) implement a fixed-size circular buffer with read/write pointers, occupied/free-size tracking, and optional lock/unlock callbacks. UART DMA callbacks use the FIFOs to decouple data arrival/transmission from application processing. The UART driver supplies critical-section callbacks that disable and re-enable CPU interrupts while updating FIFO data.

### Chapter 9 - Startup, vector table, and memory layout

The project supplies its own startup code in [`Core/Src/main.cpp`](Core/Src/main.cpp). The reset handler sets the stack pointer, zeros `.bss`, copies initialized `.data` from Flash to RAM, runs global C++ constructors, and calls `main()`. The vector table includes the initial stack pointer, core exceptions, and STM32F401RE peripheral IRQ handlers.

[`link.ld`](link.ld) places the vector table, code, read-only data, and C++ constructor list in Flash; places `.data`, `.bss`, and a reserved stack area in RAM; and defines the linker symbols used by startup. The GNU Make build compiles the sources without HAL, CMSIS startup, or Cube-generated support files.

## Building

This bare-metal build compiles application sources from `Core/Src` and uses headers from `Core/Inc`. HAL, CMSIS startup, and Cube-generated support files are not part of the build. The reset handler and STM32F401RETx interrupt vectors are defined in `Core/Src/main.cpp`.

Install GNU Make and the GNU Arm Embedded Toolchain, then run these commands from the repository root:

```sh
make
make build/bare_metal.hex
make size
```

The ELF, BIN, HEX, and map outputs are written to `build/`. To remove generated files, run `make clean`.

## Flashing

Connect the probe to the board before running a flash target.

### J-Link

Run `make flash` (or its alias, `make flash-jlink`) to program the BIN at `0x08000000`, reset the MCU, and start it. The J-Link Commander commands are in `flash.jlink`.

The Makefile defaults to the J-Link installation bundled with STM32CubeIDE on the development machine, device `STM32F401RE`, SWD interface, and a 1 MHz clock. If J-Link is installed elsewhere, override the executable path and, if needed, the device, interface, or speed:

```sh
make flash JLINK="C:/path/to/JLink.exe" JLINK_DEVICE=STM32F401RE JLINK_IF=SWD JLINK_SPEED=1000
```

### ST-LINK

Install `st-flash`, connect an ST-LINK probe, and run:

```sh
make flash-stlink
```

This target also programs the BIN at `0x08000000` and resets the MCU.

## Debugging in VS Code

The workspace includes a Cortex-Debug launch configuration for the STM32F401RE using a J-Link probe. Install the [Cortex-Debug extension](https://marketplace.visualstudio.com/items?itemName=marus25.cortex-debug), GNU Arm Embedded Toolchain, and SEGGER J-Link software. Connect the J-Link probe to the board over SWD.

Open the repository folder in VS Code, then:

1. Build the firmware with `make`, or let the debug configuration run the `Build firmware` task automatically.
2. Open a source file such as `Core/Src/main.cpp` and click beside a line number to set a breakpoint.
3. Open **Run and Debug** (`Ctrl+Shift+D`), select **Debug STM32F401RE (J-Link)**, and press `F5`.
4. The debugger loads `build/bare_metal.elf` and runs to `main()`. Press `F5` again to continue to your breakpoint.

While paused, use `F10` to step over, `F11` to step into, and the Variables, Watch, Call Stack, and Registers views to inspect program state.

The paths to `arm-none-eabi-gdb` and `JLinkGDBServerCL.exe` in `.vscode/launch.json` are specific to the development machine and may need updating if the toolchains are installed elsewhere.

![VS Code paused at a breakpoint while debugging the firmware](docs/images/vscode-debug.png)
