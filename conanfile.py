from conans import ConanFile, CMake, tools


class RosePngConan(ConanFile):
    name = "rose-png"
    version = "0.1.0"
    license = "MIT OR Apache-2.0"
    url = "https://github.com/markushedvall/rose-png"
    description = "Easy loading of PNG images into bitmap structures"
    exports_sources = "include/*"
    no_copy_source = True
    requires = (
        "rose-bitmap/0.2.0@markushedvall/stable",
        "rose-result/0.1.0@markushedvall/stable",
        "libpng/1.6.34@bincrafters/stable",
    )

    def package(self):
        self.copy("*.hpp")

    def package_id(self):
        self.info.header_only()
