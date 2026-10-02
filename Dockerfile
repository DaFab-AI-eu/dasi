# syntax=docker/dockerfile:1

# =============================================================================
# Base image with compilers, tools, and pre-built dependencies (ecbuild, libaec, aws-sdk)
# =============================================================================
FROM rockylinux/rockylinux:9.6 AS build-dependencies

ARG BUILD_JOBS=2
ENV CMAKE_BUILD_PARALLEL_LEVEL=${BUILD_JOBS}
LABEL org.opencontainers.image.title="DASI build dependencies" \
      org.opencontainers.image.description="Base image with compilers, build tools, and pre-built dependencies (ecbuild, libaec, AWS SDK for C++) used to build DASI." \
      org.opencontainers.image.source="https://github.com/ecmwf-projects/dasi" \
      org.opencontainers.image.licenses="Apache-2.0" \
      org.opencontainers.image.vendor="ECMWF"

# Install build essentials and development libraries
RUN set -ex; \
    dnf install -y dnf-plugins-core epel-release && \
    dnf config-manager --set-enabled crb && /usr/bin/crb enable && \
    dnf config-manager --set-enabled devel && \
    dnf install -y \
    tar redhat-rpm-config ca-certificates \
    # Build tools
    git cmake ninja-build diffutils which unzip \
    # Compilers
    gcc gcc-c++ gcc-fortran \
    binutils glibc-devel bison flex findutils libffi-devel \
    # GCC toolset 14
    gcc-toolset-14 gcc-toolset-14-binutils gcc-toolset-14-libstdc++-devel gcc-toolset-14-libasan-devel \
    # Development libraries
    ncurses-devel bzip2-devel openssl-devel lz4-devel libcurl-devel zlib-devel libuuid-devel \
    # MPI support
    openmpi openmpi-devel \
    # Python
    python3.11 python3.11-pip python3.11-devel && \
    dnf clean all && \
    rm -rf /var/cache/dnf /var/log/* /var/tmp/* ~/.cache/* && \
    # Python symlink and tools
    ln -sf /usr/bin/python3.11 /usr/bin/python3 && \
    ln -sf /usr/bin/python3.11 /usr/bin/python && \
    python -m pip install -q --upgrade pip setuptools wheel gcovr && \
    python -m pip install -q --no-cache-dir Cython boto3 "rucio-clients==40.2.0"

# Install ecbuild
ADD --keep-git-dir=true https://github.com/ecmwf/ecbuild.git#3.12.0 /tmp/ecbuild

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /tmp/ecbuild -B /tmp/ecbuild/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/ecbuild/build --target install && \
    rm -rf /tmp/ecbuild

# Install libaec from source
ARG LIBAEC_REF=b8fd98cfbba2fcc82f1c99813465d7c4806b9d4d
ADD --keep-git-dir=true https://gitlab.dkrz.de/k202009/libaec.git#${LIBAEC_REF} /tmp/libaec

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /tmp/libaec -B /tmp/libaec/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/libaec/build --target install && \
    rm -rf /tmp/libaec

# Install AWS SDK CPP (S3 only)
ARG AWS_SDK_REF=32b1058cbe6b99559aa7d0b899b60f16e86bb766
RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    git init /tmp/aws-sdk-cpp && \
    git -C /tmp/aws-sdk-cpp remote add origin https://github.com/aws/aws-sdk-cpp && \
    git -C /tmp/aws-sdk-cpp fetch --depth 1 origin "${AWS_SDK_REF}" && \
    git -C /tmp/aws-sdk-cpp checkout --detach FETCH_HEAD && \
    git -C /tmp/aws-sdk-cpp submodule update --init --recursive --depth 1 && \
    cmake -S /tmp/aws-sdk-cpp -B /tmp/aws-sdk-cpp/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_ONLY="s3" && \
    cmake --build /tmp/aws-sdk-cpp/build --target install && \
    rm -rf /tmp/aws-sdk-cpp

# Install AWS CLI v2
RUN set -ex; \
    curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o /tmp/awscliv2.zip && \
    unzip /tmp/awscliv2.zip -d /tmp && \
    /tmp/aws/install --bin-dir /usr/local/bin --install-dir /usr/local/aws-cli --update && \
    rm -rf /tmp/awscliv2.zip /tmp/aws

# =============================================================================
# Development environment for devcontainer and CI
# =============================================================================
FROM build-dependencies AS dev-env

# Configure shell environment for interactive use
RUN echo "source /opt/rh/gcc-toolset-14/enable" >> /etc/profile.d/dev-env.sh

# Install development and debugging tools
RUN set -ex; \
    dnf install -y \
    # Debugging and profiling
    gdb valgrind systemtap ltrace strace perf papi lcov \
    llvm-toolset clang-tools-extra \
    # Editor and utilities
    vim-enhanced less sudo ccache && \
    dnf clean all && \
    rm -rf /var/cache/dnf /var/log/* /var/tmp/* ~/.cache/* && \
    # Install Python development tools
    pip install -U --no-cache-dir \
    pytest pytest-env pycparser pyyaml packaging build \
    black isort flake8 mypy ipython debugpy

RUN useradd --create-home --uid 1000 vscode && \
    echo 'vscode ALL=(root) NOPASSWD:ALL' > /etc/sudoers.d/vscode && \
    chmod 0440 /etc/sudoers.d/vscode && \
    mkdir -p /workspace/dasi /workspace/bundle /workspace/.ccache /tmp/build /workspace/install && \
    chown -R vscode:vscode /workspace /tmp/build

ENV CCACHE_DIR=/workspace/.ccache CCACHE_MAXSIZE=1G
WORKDIR /workspace/dasi
USER vscode
CMD ["bash"]

# =============================================================================
# Runtime stage
# =============================================================================
FROM rockylinux/rockylinux:9.6-minimal AS dasi-runtime

ARG DASI_VERSION=latest

LABEL version="dasi:${DASI_VERSION}"

# Install only runtime dependencies
RUN set -ex; \
    microdnf install -y \
    libstdc++ \
    python3.11 python3.11-pip \
    ncurses openssl lz4-libs bzip2-libs zlib libuuid libcurl libgfortran ca-certificates && \
    microdnf clean all && \
    rm -rf /var/cache/* /var/log/* /var/tmp/* && \
    ln -sf /usr/bin/python3.11 /usr/bin/python3 && \
    ln -sf /usr/bin/python3.11 /usr/bin/python && \
    python -m pip install --upgrade pip && \
    python -m pip install -q --no-cache-dir boto3 "rucio-clients==40.2.0"

COPY .artifacts/install/ /opt/dasi/
COPY .artifacts/wheels/ /tmp/wheels/

# Update the dynamic linker cache
RUN printf '/opt/dasi/lib\n/opt/dasi/lib64\n' > /etc/ld.so.conf.d/dasi-libs.conf && ldconfig

# Install pydasi
RUN pip install --no-cache-dir /tmp/wheels/pydasi-*.whl && \
    rm -rf /tmp/wheels && \
    mkdir -p /workspace && chown 1000:1000 /workspace

ENV PATH=/opt/dasi/bin:$PATH
WORKDIR /workspace
USER 1000:1000
