Import("env")
import os

BOARD_CONFIG = env.BoardConfig()

def merge_bin_action(source, target, env):
    build_dir  = env.subst("$BUILD_DIR")
    prog_name  = env.subst("$PROGNAME")
    flash_size = BOARD_CONFIG.get("upload.flash_size", "4MB")
    flash_mode = BOARD_CONFIG.get("build.flash_mode", "dio")
    flash_freq = BOARD_CONFIG.get("build.f_flash", "40000000L")

    # Normalise flash frequency (e.g. "40000000L" -> "40m")
    freq_hz = int(flash_freq.rstrip("L"))
    flash_freq_str = "{}m".format(freq_hz // 1000000)

    # boot_app0.bin from the Arduino-ESP32 framework package
    framework_dir = env.PioPlatform().get_package_dir("framework-arduinoespressif32")
    boot_app0     = os.path.join(framework_dir, "tools", "partitions", "boot_app0.bin")

    app_bin    = os.path.join(build_dir, prog_name + ".bin")
    merged_bin = os.path.join(build_dir, prog_name + "_merged.bin")
    partitions = os.path.join(build_dir, "partitions.bin")

    # Try named bootloader first, fall back to generic bootloader.bin
    bootloader = os.path.join(build_dir, "bootloader_{}_{}.bin".format(flash_mode, flash_freq_str))
    if not os.path.exists(bootloader):
        bootloader = os.path.join(build_dir, "bootloader.bin")

    cmd = " ".join([
        '"$PYTHONEXE"', '"$OBJCOPY"',
        "--chip", "esp32",
        "merge_bin",
        "--output", '"{}"'.format(merged_bin),
        "--flash_mode", flash_mode,
        "--flash_freq", flash_freq_str,
        "--flash_size", flash_size,
        "0x1000",  '"{}"'.format(bootloader),
        "0x8000",  '"{}"'.format(partitions),
        "0xe000",  '"{}"'.format(boot_app0),
        "0x10000", '"{}"'.format(app_bin),
    ])

    print("\n=============================================")
    print("  Merging firmware into single flashable .bin")
    print("  Output: {}".format(merged_bin))
    print("=============================================")
    env.Execute(env.subst(cmd))

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin_action)
