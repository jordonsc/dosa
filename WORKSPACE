workspace(name = "DOSA")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "io_bazel_rules_docker",
    sha256 = "95d39fd84ff4474babaf190450ee034d958202043e366b9fc38f438c9e6c3334",
    strip_prefix = "rules_docker-0.16.0",
    urls = ["https://github.com/bazelbuild/rules_docker/archive/v0.16.0.tar.gz"],
)

http_archive(
    name = "rules_pkg",
    sha256 = "dff10e80f2d58d4ce8434ef794e5f9ec0856f3a355ae41c6056259b65e1ad11a",
    strip_prefix = "rules_pkg-0.4.0/pkg",
    urls = [
        "https://github.com/bazelbuild/rules_pkg/archive/0.4.0.tar.gz",
    ],
)

load("@rules_pkg//:deps.bzl", "rules_pkg_dependencies")
load("@io_bazel_rules_docker//repositories:repositories.bzl", container_repositories = "repositories")

rules_pkg_dependencies()

load("@io_bazel_rules_docker//container:container.bzl", "container_pull")

container_repositories()

container_pull(
    name = "img-zookeeper",
    #digest = "sha256:c41e8d2a4ca9cddb4398bf08c99548b9c20d238f575870ae4d3216bc55ef3ca7",
    registry = "index.docker.io",
    repository = "library/zookeeper",
    tag = "3.7",
)

load("@io_bazel_rules_docker//repositories:deps.bzl", container_deps = "deps")

container_deps()

http_archive(
    name = "gtest",
    sha256 = "b4870bf121ff7795ba20d20bcdd8627b8e088f2d1dab299a031c1034eddc93d5",
    strip_prefix = "googletest-release-1.11.0",
    urls = [
        "https://github.com/google/googletest/archive/refs/tags/release-1.11.0.tar.gz",
    ],
)
