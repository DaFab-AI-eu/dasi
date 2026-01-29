# =============================================================================
# Base image with compilers, tools, and pre-built dependencies (ecbuild, libaec, aws-sdk)
# =============================================================================
FROM rockylinux/rockylinux:9.6 AS build-dependencies

# Install build essentials and development libraries
RUN set -ex; \
    dnf install -y dnf-plugins-core epel-release && \
    dnf config-manager --set-enabled crb && /usr/bin/crb enable && \
    dnf config-manager --set-enabled devel && \
    dnf install -y \
    # Build tools
    git cmake ninja-build diffutils which unzip \
    # Compilers
    gcc gcc-c++ gcc-fortran \
    binutils glibc-devel bison flex \
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
    ln -s /usr/bin/python3.11 /usr/bin/python && \
    python -m pip install -q --upgrade pip setuptools wheel gcovr

# Install ecbuild
ADD --keep-git-dir=true https://github.com/ecmwf/ecbuild.git#3.12.0 /tmp/ecbuild

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /tmp/ecbuild -B /tmp/ecbuild/build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/ecbuild/build --target install && \
    rm -rf /tmp/ecbuild

# Install libaec from source
ADD --keep-git-dir=true https://gitlab.dkrz.de/k202009/libaec.git /tmp/libaec

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /tmp/libaec -B /tmp/libaec/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local && \
    cmake --build /tmp/libaec/build --target install && \
    rm -rf /tmp/libaec

# Install AWS SDK CPP (S3 only)
RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    git clone --depth 1 --recurse-submodules --shallow-submodules \
    https://github.com/aws/aws-sdk-cpp /tmp/aws-sdk-cpp && \
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
# Development environment for devcontainer
# =============================================================================
FROM build-dependencies AS dev-env

ARG DEV_USERNAME=developer
ARG DEV_UID=1000
ARG DEV_GID=$DEV_UID

# Configure shell environment for interactive use
RUN echo "source /opt/rh/gcc-toolset-14/enable" >> /etc/profile.d/dev-env.sh

# Install development and debugging tools
RUN set -ex; \
    dnf install -y \
    # Debugging and profiling
    gdb valgrind systemtap ltrace strace perf papi lcov \
    llvm-toolset clang-tools-extra \
    # Editor and utilities
    vim-enhanced less sudo && \
    dnf clean all && \
    rm -rf /var/cache/dnf /var/log/* /var/tmp/* ~/.cache/*

# Install Python development tools
RUN pip install --no-cache-dir \
    pytest pytest-env pycparser pyyaml packaging build \
    black isort flake8 mypy ipython debugpy

# Create non-root user for development
RUN groupadd --gid ${DEV_GID} ${DEV_USERNAME} && \
    useradd --uid ${DEV_UID} --gid ${DEV_GID} -m ${DEV_USERNAME} && \
    echo "${DEV_USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${DEV_USERNAME} && \
    chmod 0440 /etc/sudoers.d/${DEV_USERNAME}

USER ${DEV_USERNAME}

WORKDIR /workspace

COPY ./bundle/CMakeLists.txt .
COPY ./bundle/Linux.cmake .

RUN set -ex; \
    sed -i 's/\(PROJECT.*dasi.*GIT.*\)UPDATE/\1MANUAL/' CMakeLists.txt && \
    sed -i 's/ENABLE_TESTS.*OFF/ENABLE_TESTS ON/' Linux.cmake && \
    sed -i 's/BUILD_TESTING.*OFF/BUILD_TESTING ON/' Linux.cmake

# =============================================================================
# Builds DASI
# =============================================================================
FROM build-dependencies AS dasi-builder

ARG DASI_VERSION=0.2.8

RUN echo "Building DASI Version: ${DASI_VERSION}"

WORKDIR /workspace

COPY ./bundle/CMakeLists.txt .
COPY ./bundle/Linux.cmake .

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    sed -i "s/set(.DASI_VERSION.*)/set( DASI_VERSION ${DASI_VERSION} )/" CMakeLists.txt && \
    sed -i 's/ENABLE_TESTS.*ON/ENABLE_TESTS OFF/' Linux.cmake && \
    sed -i 's/BUILD_TESTING.*ON/BUILD_TESTING OFF/' Linux.cmake && \
    cmake -S . -B /tmp/build/dasi-bundle -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local/dasi && \
    cmake --build /tmp/build/dasi-bundle --target all install pydasi_package && \
    cp ./dasi/pydasi/dist/pydasi-*.whl /tmp/ && \
    rm -rf /tmp/build/dasi-bundle

# =============================================================================
# Tests DASI
# =============================================================================
FROM dasi-builder AS dasi-tester

WORKDIR /workspace

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    sed -i 's/ENABLE_TESTS.*OFF/ENABLE_TESTS ON/' Linux.cmake && \
    sed -i 's/BUILD_TESTING.*OFF/BUILD_TESTING ON/' Linux.cmake && \
    rm -rf /tmp/build/dasi-bundle && \
    cmake -S . -B /tmp/build/dasi-bundle -G Ninja -DCMAKE_BUILD_TYPE=Debug && \
    cmake --build /tmp/build/dasi-bundle --target all pydasi_develop

# =============================================================================
# Runtime stage
# =============================================================================
FROM rockylinux/rockylinux:9.6-minimal AS dasi-runtime

# Install only runtime dependencies
RUN set -ex; \
    microdnf install -y \
    libstdc++ \
    python3.11 \
    ncurses openssl lz4-libs bzip2-libs zlib libuuid libcurl && \
    microdnf clean all && \
    rm -rf /var/cache/* /var/log/* /var/tmp/* && \
    ln -sf /usr/bin/python3.11 /usr/bin/python3 && \
    ln -sf /usr/bin/python3.11 /usr/bin/python && \
    python -m ensurepip --upgrade  && \
    python -m pip install --upgrade pip

# Copy DASI installation from builder
COPY --from=dasi-builder /usr/local/dasi /usr/local/
COPY --from=dasi-builder /usr/local/lib64/libaws* /usr/local/lib64/
COPY --from=dasi-builder /usr/local/lib64/libs2n* /usr/local/lib64/
COPY --from=dasi-builder /usr/local/lib64/libaec* /usr/local/lib64/
COPY --from=dasi-builder /usr/local/lib64/libsz* /usr/local/lib64/
COPY --from=dasi-builder /opt/rh/gcc-toolset-14/root/usr/lib/gcc/x86_64-redhat-linux/14/libstdc++.so.6* /usr/local/lib64/

RUN echo "/usr/local/lib64" > /etc/ld.so.conf.d/dasi-libs.conf && ldconfig

# Install pydasi
COPY --from=dasi-builder /tmp/pydasi-*.whl /tmp/

RUN pip install --no-cache-dir /tmp/pydasi-*.whl && \
    rm -rf /tmp/pydasi-*.whl

WORKDIR /workspace
