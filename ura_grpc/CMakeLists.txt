set(CMAKE_BUILD_TYPE Debug)
cmake_minimum_required(VERSION 3.10)
project(ura_grpc)

# Adiciona os diretórios de include para gRPC/Protobuf (via apt) e PJSIP (compilado)
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    /usr/include
    /usr/local/include
)

# Adiciona os diretórios onde as bibliotecas (.a / .so) estão
link_directories(/usr/lib /usr/local/lib)

# Define o executável e seus arquivos fonte
add_executable(server
    server.cpp
    sip_service.pb.cc
    sip_service.grpc.pb.cc
)

# Linkagem manual completa, incluindo todos os codecs
target_link_libraries(server
    # gRPC e dependências do sistema
    grpc++
    protobuf
    pthread

    # --- Bibliotecas PJSIP (do mais alto ao mais baixo nível) ---
    pjsua2
    pjsua
    pjsip-ua
    pjsip-simple
    pjsip
    
    # --- Bibliotecas de Mídia e Codecs do PJSIP ---
    pjmedia-codec
    pjmedia-videodev
    pjmedia-audiodev
    pjmedia
    pjnath
    pjlib-util

    # --- Codecs Específicos e SRTP ---
    srtp
    resample
    gsmcodec
    speex
    ilbccodec
    g7221codec
    # yuv (geralmente para vídeo, pode ser opcional)
    # webrtc (pode ser opcional, dependendo dos recursos de AEC)

    # --- Biblioteca Base do PJSIP ---
    pj

    # --- Dependências Finais do Sistema ---
    ssl
    crypto
    asound
    z
    dl
    m
    rt
)
