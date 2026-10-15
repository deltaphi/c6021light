# Working in this repository

## Build and test

- The build project is the nested `c6021light/` directory. Commands below run there; install PlatformIO with `python -m pip install -r ../requirements.txt`.
- Firmware: `platformio run -e bluepill`. This targets STM32F103C8 with libopencm3 and FreeRTOS, not Arduino.
- Desktop tests use CMake/GoogleTest, despite the PlatformIO boilerplate in `test/README`. Fetch dependencies **before** configuring CMake: it reads libraries directly from `.pio/libdeps/bluepill/`.

```sh
platformio pkg install -e bluepill
cmake --preset Linux
cmake --build --preset Linux
../../c6021light-build-linux/c6021lightTest
```

- Presets put build directories beside the repository, not inside it. Windows uses `cmake --preset Windows`, `cmake --build --preset Windows`, and `../../c6021light-build-windows/Debug/c6021lightTest.exe` (Visual Studio 2022).
- Focused tests: `../../c6021light-build-linux/c6021lightTest --gtest_filter='*TurnoutRoutingFixture*'`; use `--gtest_list_tests` to discover names. CTest registers the entire executable as one test, so use GoogleTest filters for individual cases.
- Initial setup needs network access: PlatformIO fetches commit-pinned libraries; CMake downloads its pinned GoogleTest during configuration (`CMakeLists.txt.in`).
- `CMakeLists.txt` explicitly lists application and test `.cpp` files for `c6021lightTest`; add new host-tested sources there. Host tests compile C++14 and replace hardware/OS dependencies through `test/mocks` include precedence.
- Optional static analysis: configure with `cmake --preset Linux -DENABLE_CLANG_TIDY=ON`, then rebuild (requires `clang-tidy` on PATH).

## Execution and constraints

Paths in this section are relative to `c6021light/`.

- `src/main.cpp` wires the HAL, persisted `DataModel`, callbacks, timers, and two FreeRTOS tasks. `src/tasks/RoutingTask/RoutingTask.cpp` is the routing hub: I2C and LocoNet messages convert to `RR32Can::CanFrame` before forwarding and local handling. The root README's overall-routing discussion includes proposals; consult this code for current behavior.
- `RoutingTask::TaskMain()` waits for notifications, then calls `loop()`. Host integration tests drive `loop()` directly using `test/mocks/RoutingTaskFixture.h` and `SequenceMaker.h`; they do not exercise real interrupts or the scheduler.
- Preserve the explicit message-pointer `reset()` calls in CAN/I2C receive loops: the HAL needs the previous buffer released before returning another message.
- FreeRTOS dynamic allocation is disabled in `include/FreeRTOSConfig.h`; use the static task/timer infrastructure in `lib/freertossupport/`.
- `stm32f103c8.ld` limits firmware ROM to 62 KiB and reserves the final 2 KiB for FlashFairyPP configuration storage. Its `_flashFairyPage0/1` symbols are consumed by `src/hal/stm32eepromEmulation.cpp`; preserve that reservation when changing the linker layout.
- Hardware validation is documented in `test/TestManual.md` (MS2 over CAN and Keyboard over I2C); it records that operation without CAN is unreliable. Host integration tests do not replace this check for hardware changes.

## Formatting

- Use `c6021light/.clang-format` for changed C/C++ files. `apply-clang.sh` hardcodes `clang-format.exe` and Windows LLVM paths; on Linux run `clang-format -i <changed-files>` from `c6021light/`. The script's scope is `src`, `test`, `lib/freertossupport`, and `lib/cliSupport`, excluding vendored FreeRTOS and microrl.
