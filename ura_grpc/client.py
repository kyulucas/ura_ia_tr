import grpc
import threading
import os
from dotenv import load_dotenv

import sip_service_pb2
import sip_service_pb2_grpc

# Carrega as variáveis do arquivo .env
load_dotenv()

# --- Leitura das Configurações ---
GRPC_ADDRESS = os.getenv("GRPC_SERVER_ADDRESS")
DEST_NUMBER = os.getenv("DESTINATION_NUMBER")
SIP_DOMAIN = os.getenv("SIP_DOMAIN")
SIP_USERNAME = os.getenv("SIP_USERNAME")  # Para account_id


def listen_for_events(stub):
    """Função que roda em uma thread para escutar eventos do servidor."""
    print("--- [Thread de Eventos] Se inscrevendo para receber eventos... ---")
    try:
        request = sip_service_pb2.EventSubscriptionRequest(client_id="python-client-1")
        for event in stub.SubscribeToEvents(request):
            print(f"\n[EVENTO RECEBIDO] Tipo: {event.event_type} Detalhes: {event.details}")
    except grpc.RpcError as err:
        print(f"--- [Thread de Eventos] Conexão encerrada: {err} ---")


def main():
    """Função principal que executa o teste."""
    with grpc.insecure_channel(GRPC_ADDRESS) as channel:
        grpc.channel_ready_future(channel).result(timeout=5)
        stub = sip_service_pb2_grpc.SipServiceStub(channel)

        # Inicia a thread para ouvir eventos
        event_thread = threading.Thread(target=listen_for_events, args=(stub,), daemon=True)
        event_thread.start()

        # Monta a URI de destino completa
        destination_uri = f"sip:{DEST_NUMBER}@{SIP_DOMAIN}"
        print(f"\n[PYTHON] Chamando MakeCall para: {destination_uri}")
        try:
            request = sip_service_pb2.CallRequest(account_id=SIP_USERNAME, destination=destination_uri)
            response = stub.MakeCall(request)
            print(f"[PYTHON] Resposta de MakeCall recebida: Sucesso={response.success}, CallID={response.call_id}, Msg='{response.error_message}'")
        except grpc.RpcError as err:
            print(f"[PYTHON] Erro ao chamar MakeCall: {err.details()}")

        # Mantém a thread principal viva para receber eventos
        event_thread.join(timeout=30)  # Espera por 30 segundos
        print("\n[PYTHON] Teste finalizado.")


if __name__ == '__main__':
    main()
