import grpc
import threading
import time
import sip_service_pb2
import sip_service_pb2_grpc

def event_listener(stub):
    request = sip_service_pb2.EventSubscriptionRequest(client_id="python-client-1")
    for event in stub.SubscribeToEvents(request):
        print(f"[EVENTO] call_id={event.call_id} tipo={event.event_type} detalhes={event.details}")

def main():
    channel = grpc.insecure_channel('localhost:50051')
    stub = sip_service_pb2_grpc.SipServiceStub(channel)

    # Inicia thread para escutar eventos
    t = threading.Thread(target=event_listener, args=(stub,), daemon=True)
    t.start()

    # Chama MakeCall
    print("[PYTHON] Chamando MakeCall...")
    response = stub.MakeCall(sip_service_pb2.CallRequest(account_id="acc1", destination="sip:1234@domain"))
    print(f"[PYTHON] Resposta MakeCall: success={response.success} call_id={response.call_id} erro={response.error_message}")

    # Aguarda eventos
    print("[PYTHON] Aguardando eventos...")
    time.sleep(15)  # Tempo suficiente para receber todos os eventos

if __name__ == "__main__":
    main()
