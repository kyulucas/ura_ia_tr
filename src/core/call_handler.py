import uuid
from enum import Enum, auto

class CallState(Enum):
    INICIANDO = auto()
    PROGREDINDO = auto()
    ATIVA = auto()
    FINALIZADA = auto()

class CallHandler:
    def __init__(self, destination_uri):
        self.app_id = str(uuid.uuid4())
        self.sip_id = None
        self.destination_uri = destination_uri
        self.state = CallState.INICIANDO
        print(f"[{self.app_id}] Novo CallHandler criado para {destination_uri}") 