cmake_minimum_required(VERSION 3.10)
project(ura_grpc)

# Inclui diretórios padrão do sistema
include_directories(/usr/include /usr/local/include)
link_directories(/usr/lib /usr/local/lib)

# Adiciona o executável
add_executable(server server.cpp sip_service.pb.cc sip_service.grpc.pb.cc)

# Linkagem manual das bibliotecas necessárias
# (ajuste a lista de libs PJSIP conforme necessário para seu build)
target_link_libraries(server
    grpc++
    grpc
    protobuf
    pthread
    pjsua2-x86_64-pc-linux-gnu
    pjsua-x86_64-pc-linux-gnu
    pjsip-ua-x86_64-pc-linux-gnu
    pjsip-simple-x86_64-pc-linux-gnu
    pjsip-x86_64-pc-linux-gnu
    pjmedia-codec-x86_64-pc-linux-gnu
    pjmedia-x86_64-pc-linux-gnu
    pjmedia-videodev-x86_64-pc-linux-gnu
    pjmedia-audiodev-x86_64-pc-linux-gnu
    pjmedia-x86_64-pc-linux-gnu
    pjnath-x86_64-pc-linux-gnu
    pjlib-util-x86_64-pc-linux-gnu
    pj-x86_64-pc-linux-gnu
    ssl
    crypto
    z
    m
    dl
    rt
    asound
)
