# =========================
# 1. BUILDER STAGE
# =========================
FROM ubuntu:22.04 AS builder

ARG GIT_REPO_URL
ARG GIT_REPO_BRANCH=main

# Dependências de build
RUN apt-get update && apt-get install -y \
    build-essential cmake git wget curl pkg-config \
    libssl-dev libasound2-dev libsqlite3-dev zlib1g-dev \
    ca-certificates python3 python3-pip python3-venv

WORKDIR /build

# Clone o repositório do microserviço
RUN git clone --branch ${GIT_REPO_BRANCH} ${GIT_REPO_URL} app

# =========================
# 1.1. Protobuf
# =========================
WORKDIR /build
RUN git clone --branch v3.21.6 https://github.com/protocolbuffers/protobuf.git
WORKDIR /build/protobuf
RUN git submodule update --init --recursive
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_POSITION_INDEPENDENT_CODE=ON && \
    make -j$(nproc) && make install
RUN ldconfig

# =========================
# 1.2. gRPC
# =========================
WORKDIR /build
RUN git clone --recurse-submodules -b v1.54.3 https://github.com/grpc/grpc
WORKDIR /build/grpc
RUN mkdir -p build && cd build && \
    cmake .. -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release -DgRPC_PROTOBUF_PROVIDER=package && \
    make -j$(nproc) && make install
RUN ldconfig

# =========================
# 1.3. Codecs externos (Speex, GSM, iLBC, SRTP)
# =========================
WORKDIR /build
# Speex
RUN git clone https://github.com/xiph/speex.git && cd speex && ./autogen.sh && ./configure --enable-shared && make -j$(nproc) && make install
# GSM
RUN wget https://www.quut.com/gsm/gsm-1.0.22.tar.gz && tar xzf gsm-1.0.22.tar.gz && cd gsm-1.0.22 && make && make install
# iLBC
RUN git clone https://github.com/TimothyGu/libilbc.git && cd libilbc && cmake . && make -j$(nproc) && make install
# SRTP
RUN git clone https://github.com/cisco/libsrtp.git && cd libsrtp && git checkout v2.4.2 && ./configure --enable-openssl && make -j$(nproc) && make install
RUN ldconfig

# =========================
# 1.4. PJSIP
# =========================
WORKDIR /build
RUN git clone https://github.com/pjsip/pjproject.git
WORKDIR /build/pjproject
RUN ./configure --prefix=/usr --enable-shared --with-external-speex --with-external-gsm --with-external-ilbc --with-external-srtp --with-ssl=/usr
RUN make -j$(nproc) && make install
RUN ldconfig

# =========================
# 1.5. Build do microserviço
# =========================
WORKDIR /build/app/ura_grpc
RUN mkdir build && cd build && cmake .. && make

# =========================
# 2. RUNNER STAGE
# =========================
FROM ubuntu:22.04 AS runner

# Dependências mínimas de runtime
RUN apt-get update && apt-get install -y \
    libssl3 libasound2 libsqlite3-0 zlib1g && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copie apenas o executável e as libs necessárias do builder
COPY --from=builder /build/app/ura_grpc/build/server /app/server
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder /usr/lib /usr/lib

EXPOSE 50051

# Variáveis de ambiente SIP
ENV SIP_USERNAME="" \
    SIP_PASSWORD="" \
    SIP_DOMAIN=""

CMD ["/app/server"] 