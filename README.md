A project built from the ground up: developing custom libraries, writing peripheral drivers, and applying advanced programming techniques—such as the embedded structure patterns found in the Linux kernel.

Object-oriented programming implemented in C++.

The APIs have been verified on actual hardware; this repository is currently built for the STM32F401RET6 microcontroller.
Implementation of advanced communication techniques, including DMA and interrupts.

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
