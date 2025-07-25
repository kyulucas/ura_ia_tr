#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstdlib>
#include <map>
#include <grpcpp/grpcpp.h>
#include "sip_service.grpc.pb.h"

// Inclua o header do pjsua2
#include <pjsua2.hpp>
#include <pjlib.h>

using namespace pj;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using sipservice::SipService;
using sipservice::CallRequest;
using sipservice::CallResponse;
using sipservice::HangupRequest;
using sipservice::HangupResponse;
using sipservice::EventSubscriptionRequest;
using sipservice::CallEvent;
using sipservice::EventType;

// Função auxiliar para ler variáveis de ambiente
std::string getenv_str(const char* var, const char* fallback = "") {
    const char* val = std::getenv(var);
    return val ? std::string(val) : std::string(fallback);
}

// Função utilitária para garantir que a thread está registrada no PJLIB
void ensure_pj_thread_registered() {
    static pj_thread_desc thread_desc;
    static pj_thread_t* thread_ptr;
    if (!pj_thread_is_registered()) {
        pj_thread_register("grpc_worker_thread", thread_desc, &thread_ptr);
    }
}

class MyAccount : public Account {
public:
    std::function<void(OnRegStateParam&)> onRegStateCb;
    std::function<void(OnIncomingCallParam&)> onIncomingCallCb;
    void onRegState(OnRegStateParam &prm) override {
        if (onRegStateCb) onRegStateCb(prm);
    }
    void onIncomingCall(OnIncomingCallParam &prm) override {
        if (onIncomingCallCb) onIncomingCallCb(prm);
    }
};

class MyCall : public Call {
public:
    std::function<void(OnCallStateParam&)> onCallStateCb;
    MyCall(Account &acc, int call_id = PJSUA_INVALID_ID) : Call(acc, call_id) {}
    void onCallState(OnCallStateParam &prm) override {
        if (onCallStateCb) onCallStateCb(prm);
    }
};

class SipServiceImpl final : public SipService::Service {
public:
    Endpoint ep;
    std::shared_ptr<MyAccount> account;
    std::shared_ptr<MyCall> currentCall;
    std::mutex eventMutex;
    std::condition_variable eventCv;
    std::vector<std::unique_ptr<CallEvent>> eventQueue;
    std::string sip_user, sip_pass, sip_domain, sip_stun, dest_number;
    std::mutex regMutex;
    std::condition_variable regCv;
    bool registered = false;

    SipServiceImpl() {
        std::cout << "[DEBUG] Início do construtor SipServiceImpl" << std::endl;
        // Carrega variáveis de ambiente
        sip_user = getenv_str("SIP_USERNAME");
        sip_pass = getenv_str("SIP_PASSWORD");
        sip_domain = getenv_str("SIP_DOMAIN");
        sip_stun = getenv_str("SIP_STUN_SERVER");
        dest_number = getenv_str("DESTINATION_NUMBER");
        std::cout << "[DEBUG] Variáveis de ambiente carregadas: SIP_USERNAME=" << sip_user << ", SIP_DOMAIN=" << sip_domain << std::endl;
        // Inicializa o endpoint PJSUA2
        try {
            ep.libCreate();
            EpConfig ep_cfg;
            if (!sip_stun.empty()) {
                ep_cfg.uaConfig.stunServer.push_back(sip_stun);
            }
            ep.libInit(ep_cfg);
            std::cout << "[DEBUG] ep.libInit() executado" << std::endl;
            TransportConfig tcfg;
            tcfg.port = 5060;
            ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
            std::cout << "[DEBUG] ep.transportCreate() executado" << std::endl;
            ep.libStart();
            std::cout << "[DEBUG] ep.libStart() executado" << std::endl;
            // Cria e registra a conta SIP
            AccountConfig acc_cfg;
            acc_cfg.idUri = "sip:" + sip_user + "@" + sip_domain;
            acc_cfg.regConfig.registrarUri = "sip:" + sip_domain;
            acc_cfg.sipConfig.authCreds.push_back(AuthCredInfo("digest", "*", sip_user, 0, sip_pass));
            account = std::make_shared<MyAccount>();
            account->onRegStateCb = [this](OnRegStateParam &prm) {
                AccountInfo ai = account->getInfo();
                std::cout << "[DEBUG] onRegStateCb chamado: regStatus=" << ai.regStatus << std::endl;
                if (ai.regIsActive && ai.regStatus == 200) {
                    std::lock_guard<std::mutex> lock(regMutex);
                    registered = true;
                    regCv.notify_all();
                }
            };
            account->create(acc_cfg);
            std::cout << "[DEBUG] account->create() executado" << std::endl;
            // Espera registro SIP
            {
                std::unique_lock<std::mutex> lock(regMutex);
                regCv.wait(lock, [this]{ return registered; });
            }
            std::cout << "Conta SIP registrada com sucesso!" << std::endl;
        } catch (const std::exception& ex) {
            std::cerr << "[ERRO] Exceção no construtor SipServiceImpl: " << ex.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "[ERRO] Exceção desconhecida no construtor SipServiceImpl" << std::endl;
            throw;
        }
        std::cout << "[DEBUG] Fim do construtor SipServiceImpl" << std::endl;
    }

    Status MakeCall(ServerContext* context, const CallRequest* request, CallResponse* response) override {
        std::cout << "[DEBUG] Início MakeCall" << std::endl;
        ensure_pj_thread_registered();
        std::cout << "[DEBUG] ensure_pj_thread_registered() executado" << std::endl;
        try {
            CallOpParam prm(true);
            prm.opt.audioCount = 1;
            prm.opt.videoCount = 0;
            std::string dest = request->destination().empty() ? dest_number : request->destination();
            std::cout << "[DEBUG] Destino da chamada: " << dest << std::endl;
            currentCall = std::make_shared<MyCall>(*account);
            currentCall->onCallStateCb = [this](OnCallStateParam &prm) {
                CallInfo ci = currentCall->getInfo();
                std::cout << "[DEBUG] onCallStateCb: state=" << ci.state << ", stateText=" << ci.stateText << std::endl;
                CallEvent event;
                event.set_call_id(std::to_string(ci.id));
                if (ci.state == PJSIP_INV_STATE_CALLING)
                    event.set_event_type(EventType::CALL_RINGING);
                else if (ci.state == PJSIP_INV_STATE_CONFIRMED)
                    event.set_event_type(EventType::CALL_ESTABLISHED);
                else if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
                    event.set_event_type(EventType::CALL_TERMINATED);
                event.set_details(ci.stateText);
                {
                    std::lock_guard<std::mutex> lock(eventMutex);
                    eventQueue.push_back(std::make_unique<CallEvent>(event));
                }
                eventCv.notify_all();
            };
            std::cout << "[DEBUG] Antes de currentCall->makeCall()" << std::endl;
            currentCall->makeCall(dest, prm);
            std::cout << "[DEBUG] Depois de currentCall->makeCall()" << std::endl;
            response->set_success(true);
            response->set_call_id("1");
        } catch (const std::exception& ex) {
            std::cerr << "[ERRO] Exceção em MakeCall: " << ex.what() << std::endl;
            response->set_success(false);
            response->set_error_message(ex.what());
        } catch (...) {
            std::cerr << "[ERRO] Exceção desconhecida em MakeCall" << std::endl;
            response->set_success(false);
            response->set_error_message("Exceção desconhecida em MakeCall");
        }
        std::cout << "[DEBUG] Fim MakeCall" << std::endl;
        return Status::OK;
    }

    Status SubscribeToEvents(ServerContext* context, const EventSubscriptionRequest* request, ServerWriter<CallEvent>* writer) override {
        std::cout << "[DEBUG] Início SubscribeToEvents" << std::endl;
        ensure_pj_thread_registered();
        std::cout << "[DEBUG] ensure_pj_thread_registered() executado" << std::endl;
        try {
            while (!context->IsCancelled()) {
                std::unique_lock<std::mutex> lock(eventMutex);
                eventCv.wait(lock, [this]{ return !eventQueue.empty(); });
                while (!eventQueue.empty()) {
                    std::cout << "[DEBUG] Enviando evento para o client" << std::endl;
                    writer->Write(*eventQueue.front());
                    eventQueue.erase(eventQueue.begin());
                }
            }
        } catch (const std::exception& ex) {
            std::cerr << "[ERRO] Exceção em SubscribeToEvents: " << ex.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERRO] Exceção desconhecida em SubscribeToEvents" << std::endl;
        }
        std::cout << "[DEBUG] Fim SubscribeToEvents" << std::endl;
        return Status::OK;
    }

    // Implemente Hangup e outros métodos conforme necessário
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    SipServiceImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor gRPC rodando em " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}
