Import("env")
import os
import subprocess

root = env.subst("$PROJECT_DIR")
desktop = env.GetProjectOption("custom_sim_desktop")
build = os.path.join(root, "build", env.subst("$PIOENV"))

def compile_sim(*args, **kwargs):
    subprocess.check_call(["cmake", "-S", os.path.join(root, "simulator"), "-B", build,
                           "-DTWATCH_DESKTOP=" + desktop, "-DCMAKE_BUILD_TYPE=Release"])
    subprocess.check_call(["cmake", "--build", build, "--config", "Release"])

def test_sim(*args, **kwargs):
    compile_sim()
    subprocess.check_call(["ctest", "--test-dir", build, "-C", "Release", "--output-on-failure"])

def run_sim(*args, **kwargs):
    if desktop.upper() != "ON":
        raise RuntimeError("sim_run requires the desktop environment: pio run -e sim -t sim_run")
    compile_sim()
    suffix = ".exe" if os.name == "nt" else ""
    candidates = [os.path.join(build, "Release", "twatch_desktop" + suffix),
                  os.path.join(build, "twatch_desktop" + suffix)]
    executable = next((path for path in candidates if os.path.isfile(path)), None)
    if executable is None:
        raise RuntimeError("Desktop build completed but twatch_desktop was not found")
    subprocess.check_call([executable], cwd=root)

env.AddCustomTarget("sim_build", None, compile_sim, title="Build UI simulator", always_build=True)
env.AddCustomTarget("sim_check", None, test_sim, title="Build and test UI simulator", always_build=True)
env.AddCustomTarget("sim_run", None, run_sim, title="Build and launch UI simulator", always_build=True)
