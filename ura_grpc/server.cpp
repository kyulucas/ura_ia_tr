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
    if (!pj_thread_is_registered()) {
        pj_thread_t *pj_thread;
        pj_status_t status = pj_thread_register("grpc", NULL, &pj_thread);
        // Opcional: checar status
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
        // Carrega variáveis de ambiente
        sip_user = getenv_str("SIP_USERNAME");
        sip_pass = getenv_str("SIP_PASSWORD");
        sip_domain = getenv_str("SIP_DOMAIN");
        sip_stun = getenv_str("SIP_STUN");
        dest_number = getenv_str("DEST_NUMBER");

        // Inicializa o endpoint PJSUA2
        ep.libCreate();
        EpConfig ep_cfg;
        if (!sip_stun.empty()) {
            ep_cfg.uaConfig.stunServer.push_back(sip_stun);
        }
        ep.libInit(ep_cfg); // STUN e configs devem ser setadas antes
        TransportConfig tcfg;
        tcfg.port = 5060;
        ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
        ep.libStart();

        // Cria e registra a conta SIP
        AccountConfig acc_cfg;
        acc_cfg.idUri = "sip:" + sip_user + "@" + sip_domain;
        acc_cfg.regConfig.registrarUri = "sip:" + sip_domain;
        acc_cfg.sipConfig.authCreds.push_back(AuthCredInfo("digest", "*", sip_user, 0, sip_pass));
        account = std::make_shared<MyAccount>();
        // Callback para notificar registro
        account->onRegStateCb = [this](OnRegStateParam &prm) {
            AccountInfo ai = account->getInfo();
            if (ai.regIsActive && ai.regStatus == 200) {
                std::lock_guard<std::mutex> lock(regMutex);
                registered = true;
                regCv.notify_all();
            }
        };
        account->create(acc_cfg);
        // Espera registro SIP
        {
            std::unique_lock<std::mutex> lock(regMutex);
            regCv.wait(lock, [this]{ return registered; });
        }
        std::cout << "Conta SIP registrada com sucesso!" << std::endl;
    }

    Status MakeCall(ServerContext* context, const CallRequest* request, CallResponse* response) override {
        ensure_pj_thread_registered();
        // Apenas faz a chamada, não cria/registro conta
        CallOpParam prm(true);
        prm.opt.audioCount = 1;
        prm.opt.videoCount = 0;
        std::string dest = request->destination().empty() ? dest_number : request->destination();
        currentCall = std::make_shared<MyCall>(*account);
        currentCall->onCallStateCb = [this](OnCallStateParam &prm) {
            CallInfo ci = currentCall->getInfo();
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
        try {
            currentCall->makeCall(dest, prm);
            response->set_success(true);
            response->set_call_id("1"); // Use o ID real se desejar
        } catch (Error &err) {
            response->set_success(false);
            response->set_error_message(err.info());
        }
        return Status::OK;
    }

    Status SubscribeToEvents(ServerContext* context, const EventSubscriptionRequest* request, ServerWriter<CallEvent>* writer) override {
        while (!context->IsCancelled()) {
            std::unique_lock<std::mutex> lock(eventMutex);
            eventCv.wait(lock, [this]{ return !eventQueue.empty(); });
            while (!eventQueue.empty()) {
                writer->Write(*eventQueue.front());
                eventQueue.erase(eventQueue.begin());
            }
        }
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
