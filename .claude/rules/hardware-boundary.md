---
description: Board/hardware abstraction rules: pins, buses and chip drivers only under src/board.
paths:
  - "src/board/**"
  - "src/hardware/**"
  - "boards/**"
  - "partitions.csv"
  - "sdkconfig.cache64"
  - "tests/*_mock_backend.h"
  - "tests/test_*_contract.cpp"
  - "tests/test_t5_*.cpp"
---

# Hardware boundary

- `src/board/` is the only place for GPIO numbers, I2C/SPI bus setup, PMIC registers, GT911, SX1262 electrical setup, e-paper controller APIs and framebuffer packing.
- `src/hardware/*.h` are generic contracts selected at compile time via `MESHINK_*_BACKEND_HEADER` macros. Keep them free of `T5_`, `t5_` and `P_LORA_` tokens.
- Changing a contract in `src/hardware/` means updating the matching mock in `tests/*_mock_backend.h` and its `tests/test_*_contract.cpp`.
- The H752-01/V2 T5 implementation is the field-tested reference. Changes to power sequencing, radio TCXO/RF-switch init (`target.cpp`), touch or display timing carry high risk and need a hardware test entry in `docs/agent/HARDWARE_TESTS.md`.
- Don't change `partitions.csv` or `sdkconfig.cache64` without being asked. CI greps specific sdkconfig values (cache line 64B, Bluedroid, octal SPIRAM, FATFS fastseek, log levels). A partition change also breaks the `update.bin` path for existing users.
- Hardware safety: respect ESP32-S3 strapping pins and never drive a pin whose board use is unknown.
