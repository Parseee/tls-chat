FROM --platform=linux/amd64 ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    wget \
    ca-certificates \
    git \
    make \
    openssl \
    cmake \
    python3 \
    gdb \
    ninja-build \
    gpg \
    gnupg \
    libc-ares-dev \
    libre2-dev \
    software-properties-common \
    libidn2-0-dev \
    idn2 \
    ccache \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Install clang-18 and set as default
RUN wget https://apt.llvm.org/llvm.sh && \
    chmod u+x llvm.sh && \
    ./llvm.sh 18 && \
    update-alternatives --install /usr/bin/clang clang /usr/bin/clang-18 160 && \
    update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-18 160 && \
    update-alternatives --install /usr/bin/llvm-config llvm-config /usr/bin/llvm-config-18 160

# build the project
RUN --mount=type=cache,target=/ccache,sharing=locked \
    git clone https://github.com/ydb-platform/ydb-cpp-sdk.git && cd ydb-cpp-sdk && \
    rm -rf build && cmake --preset release-clang \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DYDB_SDK_INSTALL=On \
    -DCMAKE_CXX_FLAGS="-fPIC" && \
    cmake --build --preset default && \
    cmake --install build --prefix=/usr/local/ && \
    cd ../
