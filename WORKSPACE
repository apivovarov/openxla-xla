workspace(name = "xla")

# Initialize the XLA repository and all dependencies.
#
# The cascade of load() statements and xla_workspace?() calls works around the
# restriction that load() statements need to be at the top of .bzl files.
# E.g. we can not retrieve a new repository with http_archive and then load()
# a macro from that repository in the same file.
load(":workspace4.bzl", "xla_workspace4")

xla_workspace4()

load(":workspace3.bzl", "xla_workspace3")

xla_workspace3()

load(":workspace2.bzl", "xla_workspace2")

xla_workspace2()

load(":workspace1.bzl", "xla_workspace1")

xla_workspace1()

load(":workspace0.bzl", "xla_workspace0")

xla_workspace0()

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "nlohmann_json",
    url = "https://github.com/nlohmann/json/archive/v3.11.3.tar.gz",
    strip_prefix = "json-3.11.3"
)

http_archive(
    name = "oneTBB",
    url = "https://github.com/oneapi-src/oneTBB/archive/v2021.11.0.tar.gz",
    strip_prefix = "oneTBB-2021.11.0"
)

new_local_repository(
    name = "amazon",
    path = "/opt/amazon",
    build_file = "hilo/BUILD.amazon",
)
