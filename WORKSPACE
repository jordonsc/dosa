workspace(name = "DOSA")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

http_archive(
    name = "gtest",
    sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
    strip_prefix = "googletest-release-1.11.0",
    urls = [
        "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
    ],
)

#http_archive(
#    name = "nanopb",
#    sha256 = "707e542c2960685f67be73afc3be3d1c15cad6dc803c42bbfde58089b315244d",
#    strip_prefix = "nanopb-0.3.9.9",
#    urls = [
#        "https://github.com/nanopb/nanopb/archive/refs/tags/0.3.9.9.tar.gz",
#    ],
#)

git_repository(
    name = "nanopb",
    commit = "3dde17ee6e122ab7116eb7e750ceda20ba655de1",
    remote = "https://github.com/nanopb/nanopb.git",
)
