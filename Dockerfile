FROM ubuntu:26.04 AS build

LABEL authors="Ernesto Casablanca"
LABEL workspace="lightrat"

# Initial setup
RUN apt-get update && \
    export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends curl software-properties-common gpg-agent && \
    apt-get autoremove -y && \
    apt-get clean -y

# Install bazel
ARG BAZELISK_VERSION=v1.30.0
ARG BAZELISK_DOWNLOAD_SHA=a248ae2b2b62b5baebb748f6fba7203bff1a27e5d01b55399a8406744cb539e8
RUN curl -fSsL -o /usr/local/bin/bazel https://github.com/bazelbuild/bazelisk/releases/download/${BAZELISK_VERSION}/bazelisk-linux-amd64 \
    && ([ "${BAZELISK_DOWNLOAD_SHA}" = "dev-mode" ] || echo "${BAZELISK_DOWNLOAD_SHA} /usr/local/bin/bazel" | sha256sum --check --status - ) \
    && chmod 0755 /usr/local/bin/bazel

# Install required packages
ARG APT_PACKAGES="git build-essential libnlopt-cxx-dev"
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get install -y --no-install-recommends ${APT_PACKAGES} && \
    apt-get autoremove -y && \
    apt-get clean -y

WORKDIR /app
COPY . .

RUN bazel run --config=opt //lightrat:install -- --dest_dir=/usr
RUN bazel build --config=opt //lightrat:lightrat_bin && mv bazel-bin/lightrat/lightrat_bin /sbin
RUN bazel clean --expunge

ENTRYPOINT [ "lightrat_bin" ]
