from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain, cmake_layout


class MikuDanceStudioConan(ConanFile):
    """MikuDanceStudio - 1:1 open-source restoration of MikuMikuDance v932.

    Package management via Conan 2.  The only third-party link-time
    dependency of the original binary is Bullet Physics 2.75 (statically
    linked); it is provided by the local recipe in ``recipes/bullet275``.
    DirectX 9 (d3d9 / d3dx9_32) and the system Win32 APIs are consumed from
    the Microsoft DirectX SDK (June 2010) / the Windows SDK, not from Conan.
    """

    name = "mikudancestudio"
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"

    requires = (
        "bullet275/2.75",
    )

    # Sources stay in the source folder; nothing is packaged from here yet.
    exports_sources = (
        "CMakeLists.txt",
        "src/*",
        "include/*",
        "exports/*",
        "docs/*",
    )

    generators = ("CMakeDeps", "CMakeToolchain")

    def layout(self):
        cmake_layout(self)

    def validate(self):
        # The restoration targets the exact 32-bit layout of the original
        # binary; the state-class offsets in docs/FIELD_MAP.md only describe
        # the x86 original.
        if self.info.settings.arch != "x86":
            self.output.warning(
                "MikuDanceStudio reproduces the x86 (32-bit) original; "
                "consider building with an x86 profile."
            )

    def build(self):
        # Build is driven by CMake directly (conan install + cmake --preset).
        # Keeping conanfile build() empty lets CMake presets own the build
        # graph while Conan owns dependency provisioning.
        pass
