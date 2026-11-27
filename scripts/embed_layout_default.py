# PlatformIO pre-build: regenerate the same header CMake uses, in the build dir.
Import("env")  # noqa: F821
from pathlib import Path
import runpy

root = Path(env.subst("$PROJECT_DIR"))
generated = Path(env.subst("$BUILD_DIR")) / "layout_generated"
generator = runpy.run_path(str(root / "scripts" / "generate_layout_default.py"))
generator["generate"](root / "config" / "layout_default.txt",
                      generated / "layout_default_embedded.h")
env.Append(CPPPATH=[str(generated)])
