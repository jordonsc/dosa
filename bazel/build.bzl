load("@io_bazel_rules_docker//container:image.bzl", "container_image")
load("@io_bazel_rules_docker//container:push.bzl", "container_push")

def app_image(tag, base = "@img-ubuntu//image", cmd = [], directory = "/app", entrypoint = [], files = [], ports = ["9000/tcp"], tars = [], layers = []):
    container_image(
        name = tag,
        base = base,
        cmd = cmd,
        directory = directory,
        entrypoint = entrypoint,
        files = files,
        ports = ports,
        repository = "dosa",
        tars = tars,
        visibility = ["//visibility:public"],
        layers = layers,
    )

def app_push(name, tag):
    container_push(
        name = tag,
        format = "Docker",
        image = "//pkg/" + name + ":" + tag,
        registry = "asia.gcr.io",
        repository = "prj-dosa/" + name,
        tag = tag,
    )
