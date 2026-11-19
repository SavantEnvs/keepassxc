# syntax=docker/dockerfile:1.7
#
# Commit image for KeePassXC: builds on the org C/C++ base (ghcr.io/savantenvs/base).
#
# Build (from the repo root):
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile \
#       --build-arg COMMIT=$(git rev-parse HEAD) \
#       [--build-arg SOURCE_DATE_EPOCH=$(date +%s)] \
#       -t keepassxc .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
# SOURCE_DATE_EPOCH stays an ARG (an empty ENV breaks clang); build.sh unsets it when empty.
ARG SOURCE_DATE_EPOCH
# No default: an unset arg lets the base ENV (ASan+UBSan, halting) flow through to build.sh.
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# KeePassXC's build dependencies, all from Debian (pinned by SOURCE_DATE_EPOCH when set):
# Qt 6 (Core/Gui/Widgets/Svg/Network/Concurrent/DBus/Test/LinguistTools), Botan 3, zlib, minizip,
# PC/SC, libusb, keyutils, qrencode, readline. X11 autotype and the docs are switched off in build.sh.
USER root
RUN [ -n "$SOURCE_DATE_EPOCH" ] && repro-sources-list.sh || true ; \
    apt-get update && apt-get install -yq --no-install-recommends \
        cmake ninja-build pkgconf \
        qt6-base-dev qt6-base-private-dev qt6-svg-dev qt6-tools-dev qt6-tools-dev-tools \
        qt6-l10n-tools libqt6svg6 \
        libbotan-3-dev zlib1g-dev libminizip-dev libpcsclite-dev libusb-1.0-0-dev \
        libkeyutils-dev libqrencode-dev libreadline-dev libgl-dev \
 && rm -rf /var/lib/apt/lists/*
USER mayhem

# Qt picks a platform plugin only for Gui code; the readers fuzzed here are headless.
ENV QT_QPA_PLATFORM=offscreen

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

# Compile-cache pre-fill (SPEC §6.2 item 18); a missing or stale pre-fill only costs a cold cache.
RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh — repo ships no functional tests"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
