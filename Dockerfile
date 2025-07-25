# ===================================================================
# Dockerfile Final e Otimizado para o Microserviço SIP
# ===================================================================

# -------------------------------------------------------------------
# Fase 1: BUILDER - Ambiente de compilação completo e otimizado
# -------------------------------------------------------------------
FROM ubuntu:22.04 AS builder

# --- Argumentos para centralizar e facilitar a manutenção ---
ARG PJSIP_VERSION=2.14
ARG GIT_REPO_URL
ARG GIT_REPO_BRANCH=main

# --- Instalação de dependências do sistema em uma única camada ---
# Usamos DEBIAN_FRONTEND para builds não-interativos e limpamos o cache do apt.
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    libssl-dev \
    libasound2-dev \
    libsqlite3-dev \
    zlib1g-dev \
    pkg-config \
    ca-certificates \
    autoconf \
    automake \
    libtool \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# --- OTIMIZAÇÃO DE CACHE: Compilamos as dependências pesadas PRIMEIRO ---
# 1.1. Compilar PJSIP (com dependências internas)
RUN git clone --depth 1 --branch ${PJSIP_VERSION} https://github.com/pjsip/pjproject.git
WORKDIR /build/pjproject
# Removemos pacotes de dev externos para forçar o PJSIP a usar suas versões internas
RUN apt-get remove -y libgsm-dev libspeex-dev libilbc-dev libsrtp2-dev || true
# Configura, compila e instala PJSIP no sistema do container builder
RUN ./configure --prefix=/usr/local --enable-shared --with-ssl
RUN make dep
RUN make -j$(nproc)
RUN make install
RUN ldconfig

# --- Agora, trazemos o código da nossa aplicação ---
# Se o código do app mudar, o Docker re-executará apenas daqui para baixo.
WORKDIR /build
RUN git clone --branch ${GIT_REPO_BRANCH} ${GIT_REPO_URL} app
WORKDIR /build/app/ura_grpc

# 1.2. Geração de Código gRPC
# O .proto já vem do git clone. Esta etapa agora está no lugar certo.
RUN protoc --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` sip_service.proto && \
    protoc --cpp_out=. sip_service.proto

# 1.3. Build do nosso microserviço
RUN rm -rf build && mkdir build && cd build && cmake .. && make

# =========================
# 2. Estágio runner: imagem mínima (VERSÃO FINAL E CORRIGIDA)
# =========================
FROM ubuntu:22.04 AS runner

# Instala apenas as dependências mínimas do sistema, SEM gRPC/Protobuf
RUN apt-get update && apt-get install -y --no-install-recommends \
    libssl3 \
    libasound2 \
    libsqlite3-0 \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copia o executável do nosso servidor
COPY --from=builder /build/app/ura_grpc/build/server /app/server

# COPIA AS BIBLIOTECAS EXATAS USADAS NO BUILD (PJSIP, gRPC, Protobuf, etc.)
COPY --from=builder /usr/local/lib /usr/local/lib

# Configura o path para que o sistema encontre as bibliotecas copiadas
ENV LD_LIBRARY_PATH=/usr/local/lib

EXPOSE 50051

# Variáveis de ambiente para o SIP
ENV SIP_USERNAME="" \
    SIP_PASSWORD="" \
    SIP_DOMAIN=""

CMD ["/app/server"] 