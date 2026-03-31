from pathlib import Path
import os

Import("env")


def _first_existing(paths):
    for candidate in paths:
        if Path(candidate).exists():
            return str(Path(candidate))
    return None


if env["PIOPLATFORM"] == "native" and os.name == "nt":
    platformio_core_dir = Path(os.environ.get("PLATFORMIO_CORE_DIR", Path.home() / ".platformio"))
    local_app_data = Path(os.environ.get("LOCALAPPDATA", ""))
    mingw_bin = _first_existing(
        [
            local_app_data
            / "Microsoft"
            / "WinGet"
            / "Packages"
            / "BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe"
            / "mingw64"
            / "bin",
        ]
    )
    archive_tool = _first_existing(
        [
            platformio_core_dir / "packages" / "toolchain-rp2040-earlephilhower" / "arm-none-eabi" / "bin" / "ar.exe",
            Path.home() / ".platformio" / "packages" / "toolchain-rp2040-earlephilhower" / "arm-none-eabi" / "bin" / "ar.exe",
        ]
    )

    if mingw_bin:
        env.PrependENVPath("PATH", mingw_bin)
        env.Replace(
            CC="gcc",
            CXX="g++",
            AR=archive_tool or "gcc-ar",
            RANLIB="ranlib",
        )
