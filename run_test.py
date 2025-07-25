import os
import time
from dotenv import load_dotenv
from src.core.call_manager import CallManager

if __name__ == "__main__":
    load_dotenv()
    dest_number = os.getenv("DEST_NUMBER")
    sip_server = os.getenv("SIP_SERVER")

    print(f"[DEBUG] DEST_NUMBER: {dest_number}")
    print(f"[DEBUG] SIP_SERVER: {sip_server}")

    manager = CallManager()
    is_registered = manager.start()

    if not is_registered:
        print("[TEST-FAIL] Não foi possível registrar a conta SIP. Verifique as credenciais e a rede.")
        manager.shutdown()
        exit()

    destination_uri = f"{dest_number}@{sip_server}"
    print(f"\n[TEST] Fazendo chamada para {destination_uri}")
    app_call_id = manager.initiate_call(destination_uri)

    if not app_call_id:
        print("[TEST-FAIL] Falha ao iniciar a chamada pelo CallManager.")
        manager.shutdown()
        exit()

    print(f"[TEST-SUCCESS] Chamada iniciada com ID: {app_call_id}")
    try:
        print("[TEST] Chamada em andamento por 20 segundos... Atenda a chamada.")
        time.sleep(20)
    finally:
        print("[TEST] Encerrando a chamada e o serviço...")
        manager.terminate_call(app_call_id)
        time.sleep(2)
        manager.shutdown()
        print("[TEST] Teste finalizado.") 