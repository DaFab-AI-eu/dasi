# -----------------------------------------------------------------------------
# Base image with build tools and libraries
# -----------------------------------------------------------------------------
FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates pkg-config \
    python3 python3-dev \
    libssl-dev libbz2-dev liblz4-dev \
    && rm -rf /var/lib/apt/lists/*

ENV INSTALL_PREFIX=/opt/ecmwf
ENV CMAKE_PREFIX_PATH=${INSTALL_PREFIX}
ENV LD_LIBRARY_PATH=${INSTALL_PREFIX}/lib
ENV PATH=${INSTALL_PREFIX}/bin:${PATH}

# Install ecbuild
ARG ECBUILD_VERSION=3.12.0
RUN git clone --depth 1 --branch ${ECBUILD_VERSION} https://github.com/ecmwf/ecbuild.git /opt/ecbuild
ENV PATH=/opt/ecbuild/bin:${PATH}

# -----------------------------------------------------------------------------
# Image with DASI dependencies
# -----------------------------------------------------------------------------
FROM base AS deps

WORKDIR /src

# Copy the project source
COPY . /src/dasi

# Build via the bundle
RUN mkdir -p /src/dasi/bundle/build && cd /src/dasi/bundle/build \
    && ecbuild --prefix=${INSTALL_PREFIX} \
        -DCMAKE_BUILD_TYPE=Release \
        .. \
    && make -j$(nproc) \
    && make install \
    && rm -rf /src

# -----------------------------------------------------------------------------
# Image with build tools and dev environment
# -----------------------------------------------------------------------------
FROM deps AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    gdb valgrind clang-format clang-tidy vim less sudo \
    python3-venv python3-pip \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

RUN groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME}

RUN python3 -m venv /opt/venv && chown -R ${USERNAME}:${USERNAME} /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"
ENV VIRTUAL_ENV="/opt/venv"
ENV DASI_DIR=${INSTALL_PREFIX}

RUN pip install --no-cache-dir \
    pytest pytest-env pycparser cffi pyyaml packaging build \
    black isort flake8 mypy ipython debugpy

WORKDIR /workspace
USER ${USERNAME}
CMD ["/bin/bash"]

# -----------------------------------------------------------------------------
# DASI runtime image
# -----------------------------------------------------------------------------
FROM ubuntu:22.04 AS dasi

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 python3-pip python3-venv \
    libssl3 libbz2-1.0 liblz4-1 \
    && rm -rf /var/lib/apt/lists/*

# Copy installed DASI and dependencies from deps stage
COPY --from=deps /opt/ecmwf /opt/ecmwf

ENV LD_LIBRARY_PATH=/opt/ecmwf/lib
ENV PATH=/opt/ecmwf/bin:${PATH}
ENV DASI_DIR=/opt/ecmwf

# Install pydasi
COPY pydasi /tmp/pydasi
RUN mkdir -p /tmp/pydasi/src/backend/libs/Linux \
    && cp /opt/ecmwf/lib/libdasi.so* /tmp/pydasi/src/backend/libs/Linux/ \
    && pip install --no-cache-dir /tmp/pydasi \
    && rm -rf /tmp/pydasi

WORKDIR /app
CMD ["/bin/bash"]
