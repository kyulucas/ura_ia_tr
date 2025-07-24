cmake_minimum_required(VERSION 3.13)
project(SipServerCpp)

find_package(gRPC REQUIRED)
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    /usr/local/include
)

link_directories(/usr/local/lib)

add_executable(server
    server.cpp
    sip_service.pb.cc
    sip_service.grpc.pb.cc
)

target_link_libraries(server
    gRPC::grpc++
    protobuf
    pjsua2-x86_64-pc-linux-gnu
    pjsua-x86_64-pc-linux-gnu
    pjsip-ua-x86_64-pc-linux-gnu
    pjsip-simple-x86_64-pc-linux-gnu
    pjsip-x86_64-pc-linux-gnu
    pjmedia-codec-x86_64-pc-linux-gnu
    pjmedia-videodev-x86_64-pc-linux-gnu
    pjmedia-audiodev-x86_64-pc-linux-gnu
    pjmedia-x86_64-pc-linux-gnu
    pjnath-x86_64-pc-linux-gnu
    pjlib-util-x86_64-pc-linux-gnu
    srtp-x86_64-pc-linux-gnu
    resample-x86_64-pc-linux-gnu
    gsmcodec-x86_64-pc-linux-gnu
    speex-x86_64-pc-linux-gnu
    ilbccodec-x86_64-pc-linux-gnu
    g7221codec-x86_64-pc-linux-gnu
    yuv-x86_64-pc-linux-gnu
    webrtc-x86_64-pc-linux-gnu
    pj-x86_64-pc-linux-gnu
    ssl
    crypto
    sqlite3
    asound
    z
    Threads::Threads
    dl
    m
)
