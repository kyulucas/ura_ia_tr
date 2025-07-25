from src.services.sip_service import BaresipyService
from src.core.call_handler import CallHandler, CallState
import threading

class CallManager:
    _instance = None
    _lock = threading.Lock()

    def __new__(cls, *args, **kwargs):
        with cls._lock:
            if cls._instance is None:
                cls._instance = super(CallManager, cls).__new__(cls)
        return cls._instance

    def __init__(self):
        if hasattr(self, '_initialized'):
            return
        self.sip_service = BaresipyService(call_manager=self)
        self.active_calls_by_app_id = {}
        self.active_calls_by_sip_id = {}
        self._initialized = True

    def start(self):
        print("Call Manager: Iniciando SIP Service...")
        self.sip_service.start()
        # Espera pelo registro SIP de forma robusta
        return self.sip_service.wait_for_registration(timeout=10)

    def initiate_call(self, destination_uri):
        handler = CallHandler(destination_uri)
        self.active_calls_by_app_id[handler.app_id] = handler
        sip_call_id = self.sip_service.make_call(destination_uri)
        if sip_call_id:
            handler.sip_id = sip_call_id
            self.active_calls_by_sip_id[sip_call_id] = handler
            return handler.app_id
        else:
            del self.active_calls_by_app_id[handler.app_id]
            return None

    def notify_call_established(self, sip_call_id):
        if sip_call_id in self.active_calls_by_sip_id:
            handler = self.active_calls_by_sip_id[sip_call_id]
            handler.state = CallState.ATIVA
            print(f"Call Manager: Notificando {handler.app_id} que a chamada está ATIVA.")
        else:
            print(f"Call Manager: Recebido evento ESTABLISHED para um sip_id desconhecido: {sip_call_id}")

    def notify_call_closed(self, sip_call_id):
        if sip_call_id in self.active_calls_by_sip_id:
            handler = self.active_calls_by_sip_id[sip_call_id]
            handler.state = CallState.FINALIZADA
            print(f"Call Manager: Notificando {handler.app_id} que a chamada foi FINALIZADA.")
            del self.active_calls_by_sip_id[sip_call_id]
            if handler.app_id in self.active_calls_by_app_id:
                del self.active_calls_by_app_id[handler.app_id]
        else:
            print(f"Call Manager: Recebido evento CLOSED para um sip_id desconhecido: {sip_call_id}")

    def terminate_call(self, app_id):
        if app_id in self.active_calls_by_app_id:
            handler = self.active_calls_by_app_id[app_id]
            self.sip_service.hangup(handler.sip_id)
        else:
            print(f"Call Manager: App ID de chamada {app_id} não encontrado.")

    def shutdown(self):
        print("Call Manager: Encerrando todas as chamadas...")
        for handler in list(self.active_calls_by_app_id.values()):
            self.terminate_call(handler.app_id)
        self.sip_service.shutdown() 