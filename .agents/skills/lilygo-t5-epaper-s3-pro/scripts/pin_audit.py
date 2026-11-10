#!/usr/bin/env python3
"""Audit MeshInk's T5 E-Paper S3 Pro (H752-01) pin assignments.

Cross-checks every GPIO macro in src/board/board_profile.h (H752-01 block) and
the platformio.ini build flags against LILYGO's schematic-derived pin map, and
flags:
  * a macro whose GPIO differs from the schematic signal it names (ERROR)
  * two different board signals assigned the same GPIO (ERROR)
  * any assignment to USB / flash / octal-PSRAM pins (ERROR)
  * literal GPIO numbers passed to pin APIs outside board_profile.h (WARN)
  * use of strapping pins (INFO)

Usage (from anywhere inside the repo):
  python3 .claude/skills/lilygo-t5-epaper-s3-pro/scripts/pin_audit.py
  python3 .../pin_audit.py --gpio 38      # who owns GPIO38, is it usable?
  python3 .../pin_audit.py --map          # print the full ownership table

Standard library only. Exit status 1 if any ERROR was found.
"""
from __future__ import annotations

import argparse
import configparser
import re
import sys
from pathlib import Path

# Canonical H752-01 wiring: LILYGO docs/pinmap.md (schematic V1.0 24-12-24),
# Xinyuan-LilyGO/T5S3-4.7-e-paper-PRO @ 87436b5. signal -> (gpio, owner note)
SCHEMATIC: dict[str, tuple[int, str]] = {
    "BOOT": (0, "BOOT button; EXT0 deep-sleep wake"),
    "LORA_RST": (1, "SX1262 RESET"),
    "RTC_INT": (2, "PCF8563 INT (unused by MeshInk; RTC-capable)"),
    "TOUCH_INT": (3, "GT911 INT"),
    "EPD_CKH": (4, "EPDiy"),
    "EPD_D0": (5, "EPDiy"),
    "EPD_D1": (6, "EPDiy"),
    "EPD_D2": (7, "EPDiy"),
    "EPD_D7": (8, "EPDiy"),
    "TOUCH_RST": (9, "GT911 RST"),
    "LORA_IRQ": (10, "SX1262 DIO1; EXT1 deep-sleep wake"),
    "BL_EN": (11, "Frontlight PWM (PT4103 EN)"),
    "SD_CS": (12, "microSD CS"),
    "SPI_MOSI": (13, "Shared SPI (SX1262 + SD)"),
    "SPI_SCLK": (14, "Shared SPI (SX1262 + SD)"),
    "EPD_D3": (15, "EPDiy"),
    "EPD_D4": (16, "EPDiy"),
    "EPD_D5": (17, "EPDiy"),
    "EPD_D6": (18, "EPDiy"),
    "USB_DM": (19, "USB D- (console)"),
    "USB_DP": (20, "USB D+ (console)"),
    "SPI_MISO": (21, "Shared SPI (SX1262 + SD)"),
    "PCA9535_INT": (38, "PCA9535 INT (unused by MeshInk; not RTC-capable)"),
    "I2C_SDA": (39, "Shared I2C bus"),
    "I2C_SCL": (40, "Shared I2C bus"),
    "EPD_STH": (41, "EPDiy"),
    "EPD_LE": (42, "EPDiy"),
    "GPS_UART_TX": (43, "ESP TX -> GPS RX (UART0 pins)"),
    "GPS_UART_RX": (44, "ESP RX <- GPS TX (UART0 pins)"),
    "EPD_STV": (45, "EPDiy"),
    "LORA_CS": (46, "SX1262 NSS"),
    "LORA_BUSY": (47, "SX1262 BUSY"),
    "EPD_CKV": (48, "EPDiy"),
}

# ESP32-S3-WROOM-1 N16R8 (qio_opi): flash/PSRAM bus and octal data lines.
RESERVED = {g: "SPI flash/PSRAM bus" for g in range(26, 33)}
RESERVED.update({g: "octal PSRAM (qio_opi)" for g in range(33, 38)})
RESERVED.update({19: "USB D-", 20: "USB D+"})
STRAPPING = {0: "boot mode", 3: "JTAG source", 45: "VDD_SPI voltage", 46: "ROM log / boot mode"}

# Macro name -> schematic signal it is supposed to carry.
MACRO_SIGNAL = {
    # board_profile.h (H752-01 block)
    "T5_PIN_I2C_SDA": "I2C_SDA",
    "T5_PIN_I2C_SCL": "I2C_SCL",
    "T5_PIN_SPI_MISO": "SPI_MISO",
    "T5_PIN_SPI_MOSI": "SPI_MOSI",
    "T5_PIN_SPI_SCLK": "SPI_SCLK",
    "T5_PIN_TOUCH_INT": "TOUCH_INT",
    "T5_PIN_TOUCH_RST": "TOUCH_RST",
    "T5_PIN_FRONTLIGHT": "BL_EN",
    "T5_PIN_BOOT_BUTTON": "BOOT",
    "T5_PIN_SD_CS": "SD_CS",
    "T5_PIN_LORA_CS": "LORA_CS",
    "T5_PIN_LORA_IRQ": "LORA_IRQ",
    "T5_PIN_LORA_RST": "LORA_RST",
    "T5_PIN_LORA_BUSY": "LORA_BUSY",
    # platformio.ini build flags
    "P_LORA_NSS": "LORA_CS",
    "P_LORA_DIO_1": "LORA_IRQ",
    "P_LORA_RESET": "LORA_RST",
    "P_LORA_BUSY": "LORA_BUSY",
    "P_LORA_SCLK": "SPI_SCLK",
    "P_LORA_MISO": "SPI_MISO",
    "P_LORA_MOSI": "SPI_MOSI",
    "PIN_BOARD_SDA": "I2C_SDA",
    "PIN_BOARD_SCL": "I2C_SCL",
    # Named from the GPS module's side: PIN_GPS_TX is the GPS TX line = ESP RX.
    "PIN_GPS_TX": "GPS_UART_RX",
    "PIN_GPS_RX": "GPS_UART_TX",
}

PIN_API = re.compile(
    r"\b(pinMode|digitalWrite|digitalRead|gpio_set_level|gpio_set_direction|"
    r"gpio_hold_en|gpio_hold_dis|rtc_gpio_\w+|attachInterrupt|ledcAttachPin)"
    r"\s*\(\s*(?:\(\s*gpio_num_t\s*\)\s*)?(\d+)\b"
)


def find_root() -> Path:
    for start in (Path.cwd(), Path(__file__).resolve().parent):
        for p in (start, *start.parents):
            if (p / "platformio.ini").is_file() and (p / "src" / "board").is_dir():
                return p
    sys.exit("pin_audit: run inside the MeshInk repository (platformio.ini not found)")


def board_profile_pins(root: Path) -> dict[str, int]:
    text = (root / "src/board/board_profile.h").read_text(encoding="utf-8")
    m = re.search(r"#if\s+T5_BOARD_H752_01(.*?)#else", text, re.S)
    if not m:
        sys.exit("pin_audit: H752-01 block not found in board_profile.h")
    return {k: int(v) for k, v in re.findall(r"#define\s+(T5_PIN_\w+)\s+(\d+)\b", m.group(1))}


def platformio_pins(root: Path) -> dict[str, int]:
    cfg = configparser.ConfigParser(interpolation=None, strict=False)
    cfg.read(root / "platformio.ini", encoding="utf-8")
    pins: dict[str, int] = {}
    for env in ("env:t5-pro", "env:t5-unified"):
        if env in cfg and "build_flags" in cfg[env]:
            for k, v in re.findall(r"-D((?:P_LORA|PIN)_\w+)=(\d+)\b", cfg[env]["build_flags"]):
                pins[k] = int(v)
    return pins


def literal_pin_uses(root: Path) -> list[str]:
    hits = []
    for path in sorted((root / "src").rglob("*")):
        if path.suffix not in {".c", ".cpp", ".h"} or path.name == "board_profile.h":
            continue
        for n, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            stripped = line.split("//", 1)[0]
            for m in PIN_API.finditer(stripped):
                hits.append(f"{path.relative_to(root)}:{n}: {m.group(1)}({m.group(2)}) uses a literal GPIO")
    return hits


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--gpio", type=int, help="explain ownership/usability of one GPIO")
    ap.add_argument("--map", action="store_true", help="print the full GPIO ownership table")
    args = ap.parse_args()

    by_gpio = {g: (sig, note) for sig, (g, note) in SCHEMATIC.items()}

    if args.gpio is not None:
        g = args.gpio
        if not 0 <= g <= 48:
            print(f"GPIO{g}: not an ESP32-S3 GPIO")
            return 1
        if g in RESERVED:
            print(f"GPIO{g}: RESERVED ({RESERVED[g]}) - unusable")
        elif g in by_gpio:
            sig, note = by_gpio[g]
            print(f"GPIO{g}: {sig} - {note}")
        else:
            print(f"GPIO{g}: not routed to any board net on H752-01 (no pad available)")
        if g in STRAPPING:
            print(f"  strapping pin ({STRAPPING[g]}): never add pulls or drive it during reset")
        print("  expansion alternatives: shared I2C (39/40) or PCA9535 IO0_1..IO0_7 (NC; needs rework)")
        return 0

    if args.map:
        for g in range(49):
            if g in RESERVED:
                desc = f"RESERVED  {RESERVED[g]}"
            elif g in by_gpio:
                desc = f"{by_gpio[g][0]:<12} {by_gpio[g][1]}"
            else:
                desc = "-         (no board net / not on this ESP32-S3 package)"
            flag = f"  [strap: {STRAPPING[g]}]" if g in STRAPPING else ""
            print(f"GPIO{g:<3} {desc}{flag}")
        return 0

    root = find_root()
    errors: list[str] = []
    warnings: list[str] = []
    infos: list[str] = []

    assigned: dict[str, int] = {}
    assigned.update(board_profile_pins(root))
    assigned.update(platformio_pins(root))
    if not assigned:
        errors.append("no pin macros found - parser out of date with board_profile.h/platformio.ini?")

    signal_of_gpio: dict[int, set[str]] = {}
    for macro, gpio in sorted(assigned.items()):
        signal = MACRO_SIGNAL.get(macro)
        if gpio in RESERVED:
            errors.append(f"{macro}={gpio}: GPIO{gpio} is reserved ({RESERVED[gpio]})")
        if signal is None:
            warnings.append(f"{macro}={gpio}: unknown macro; add it to MACRO_SIGNAL in pin_audit.py")
            sig_here = by_gpio.get(gpio, ("?", ""))[0]
        else:
            expected = SCHEMATIC[signal][0]
            if gpio != expected:
                errors.append(f"{macro}={gpio}: schematic routes {signal} to GPIO{expected}")
            sig_here = signal
        signal_of_gpio.setdefault(gpio, set()).add(sig_here)
        if gpio in STRAPPING:
            infos.append(f"{macro}=GPIO{gpio} is a strapping pin ({STRAPPING[gpio]})")

    for gpio, sigs in sorted(signal_of_gpio.items()):
        if len(sigs) > 1:
            errors.append(f"GPIO{gpio} assigned to multiple signals: {sorted(sigs)}")

    warnings.extend(literal_pin_uses(root))

    for label, items in (("ERROR", errors), ("WARN", warnings), ("INFO", sorted(set(infos)))):
        for item in items:
            print(f"{label}: {item}")
    print(f"pin_audit: {len(assigned)} macros checked, {len(errors)} error(s), {len(warnings)} warning(s)")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
