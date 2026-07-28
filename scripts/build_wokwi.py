Import("env")

from os.path import join
from SCons.Script import COMMAND_LINE_TARGETS


def build_wokwi_after_release(source, target, env):
    # An upload must operate on the physical release environment only.
    if "upload" in COMMAND_LINE_TARGETS:
        print("Wokwi build skipped for release upload.")
        return

    platformio = join(env.subst("$PROJECT_PACKAGES_DIR"),
                      "..", "penv", "Scripts", "platformio.exe")
    project_dir = env.subst("$PROJECT_DIR")
    exit_code = env.Execute(
        f'"{platformio}" run --project-dir "{project_dir}" '
        f'--environment nano_328p_wokwi'
    )
    if exit_code:
        env.Exit(exit_code)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", build_wokwi_after_release)
