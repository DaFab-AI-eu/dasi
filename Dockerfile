# syntax=docker/dockerfile:1.7
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
    python -m pip install -q --upgrade pip setuptools wheel gcovr && \
    # Configure module loading for MPI and GCC toolset
    echo "module load mpi" >> /etc/profile.d/dev-env.sh && \
    echo 'export FI_PROVIDER="^psm3"' >> /etc/profile.d/dev-env.sh && \
    echo "source /opt/rh/gcc-toolset-14/enable" >> /etc/profile.d/dev-env.sh

# Environment configuration
ENV INSTALL_PREFIX=/opt/ecmwf
ENV CMAKE_PREFIX_PATH=${INSTALL_PREFIX}:${INSTALL_PREFIX}/lib64/cmake:/usr/local:/usr/local/lib64/cmake
ENV LD_LIBRARY_PATH=${INSTALL_PREFIX}/lib64:/usr/local/lib64
ENV PATH=${INSTALL_PREFIX}/bin:${PATH}

# Install ecbuild
ADD --keep-git-dir=true https://github.com/ecmwf/ecbuild.git#3.12.0 /tmp/ecbuild

RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /tmp/ecbuild -B /tmp/ecbuild/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} && \
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

# =============================================================================
# Development environment for devcontainer
# =============================================================================
FROM build-dependencies AS dev-env

ARG DEV_USERNAME=developer
ARG DOCKER_UID=1000
ARG DOCKER_GID=${DOCKER_UID}

# Install development and debugging tools
RUN set -ex; \
    dnf install -y \
        # Debugging and profiling
        gdb valgrind systemtap ltrace strace perf papi lcov \
        llvm-toolset clang-tools-extra \
        # Editor and utilities
        vim-enhanced less sudo && \
    dnf debuginfo-install -y gcc glibc libgcc libstdc++ && \
    dnf clean all && \
    rm -rf /var/cache/dnf /var/log/* /var/tmp/* ~/.cache/*

# Install AWS CLI v2
RUN set -ex; \
    curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip" -o /tmp/awscliv2.zip && \
    unzip /tmp/awscliv2.zip -d /tmp && \
    /tmp/aws/install --bin-dir /usr/local/bin --install-dir /usr/local/aws-cli --update && \
    rm -rf /tmp/awscliv2.zip /tmp/aws

# Install MinIO Client
RUN curl -o /usr/local/bin/mc https://dl.min.io/client/mc/release/linux-amd64/mc && \
    chmod +x /usr/local/bin/mc

# Create non-root user for development
RUN groupadd --gid ${DOCKER_GID} ${DEV_USERNAME} && \
    useradd --uid ${DOCKER_UID} --gid ${DOCKER_GID} -m ${DEV_USERNAME} && \
    echo "${DEV_USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${DEV_USERNAME} && \
    chmod 0440 /etc/sudoers.d/${DEV_USERNAME}

# Set up Python virtual environment
RUN python -m venv /opt/venv && chown -R ${DEV_USERNAME}:${DEV_USERNAME} /opt/venv
ENV PATH="/opt/venv/bin:${PATH}"
ENV VIRTUAL_ENV="/opt/venv"
ENV DASI_DIR=${INSTALL_PREFIX}

# Install Python development tools
RUN pip install --no-cache-dir \
    pytest pytest-env pycparser cffi pyyaml packaging build \
    black isort flake8 mypy ipython debugpy

WORKDIR /workspace
USER ${DEV_USERNAME}
CMD ["/bin/bash", "-l"]

# =============================================================================
# Builds DASI and dependencies
# =============================================================================
FROM build-dependencies AS dasi-builder

WORKDIR /src

COPY . /src/dasi

# Build via the bundle (flags loaded from Linux.cmake)
RUN set -ex; \
    source /opt/rh/gcc-toolset-14/enable && \
    cmake -S /src/dasi/bundle -B /src/dasi/bundle/build \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="/usr/local;/usr/local/lib64/cmake" \
        -DAWSSDK_ROOT=/usr/local \
        -DAWSSDK_DIR=/usr/local/lib64/cmake/AWSSDK \
        -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX} && \
    cmake --build /src/dasi/bundle/build --parallel $(nproc) && \
    cmake --install /src/dasi/bundle/build && \
    cmake --build /src/dasi/bundle/build --target pydasi_package && \
    # Keep wheel for runtime, clean up source
    cp /src/dasi/pydasi/dist/pydasi-*.whl /tmp/ && \
    rm -rf /src

# =============================================================================
# Runtime stage
# =============================================================================
FROM rockylinux/rockylinux:9.6-minimal AS dasi-runtime

# Install only runtime dependencies
RUN microdnf install -y \
        python3.11 \
        ncurses openssl lz4-libs bzip2-libs zlib libuuid libcurl \
    && microdnf clean all && \
    rm -rf /var/cache/* /var/log/* /var/tmp/* && \
    ln -sf /usr/bin/python3.11 /usr/bin/python3 && \
    ln -sf /usr/bin/python3.11 /usr/bin/python && \
    python -m ensurepip --upgrade && \
    python -m pip install --upgrade pip

# Copy installed DASI and dependencies
COPY --from=dasi-builder /opt/ecmwf /opt/ecmwf

# Copy AWS SDK and libaec runtime libraries
COPY --from=dasi-builder /usr/local/lib64/libaws* /usr/local/lib64/
COPY --from=dasi-builder /usr/local/lib64/libaec* /usr/local/lib64/
COPY --from=dasi-builder /usr/local/lib64/libsz* /usr/local/lib64/

ENV LD_LIBRARY_PATH=/opt/ecmwf/lib64:/usr/local/lib64
ENV PATH=/opt/ecmwf/bin:${PATH}
ENV DASI_DIR=/opt/ecmwf

# Install pydasi wheel
COPY --from=dasi-builder /tmp/pydasi-*.whl /tmp/
RUN pip install --no-cache-dir /tmp/pydasi-*.whl && \
    rm -rf /tmp/pydasi-*.whl

WORKDIR /app
CMD ["/bin/bash"]
