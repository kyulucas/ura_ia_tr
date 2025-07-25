import pjsua2
from src.sip.my_audio_media import MyAudioMedia

class MyCall(pjsua2.Call):
    def __init__(self, account, dest_uri_or_id):
        if isinstance(dest_uri_or_id, str):
            super().__init__(account)
            self.dest_uri = dest_uri_or_id
        else:
            super().__init__(account, dest_uri_or_id)
            self.dest_uri = None
        self.audio_media = None

    def make_call(self):
        prm = pjsua2.CallOpParam(True)
        self.makeCall(self.dest_uri, prm)
        print(f"[PJSUA2] Chamada iniciada para {self.dest_uri}")

    def answer_call(self):
        prm = pjsua2.CallOpParam()
        prm.statusCode = 200
        self.answer(prm)
        print("[PJSUA2] Chamada atendida.")

    def onCallState(self, prm):
        info = self.getInfo()
        print(f"[PJSUA2] Estado da chamada: {info.stateText}")
        if info.state == pjsua2.PJSIP_INV_STATE_DISCONNECTED:
            print("[PJSUA2] Chamada desconectada.")

    def onCallMediaState(self, prm):
        info = self.getInfo()
        for mi in info.media:
            if mi.type == pjsua2.PJMEDIA_TYPE_AUDIO and mi.status == pjsua2.PJSUA_CALL_MEDIA_ACTIVE:
                aud_med = self.getAudioMedia(-1)
                self.audio_media = MyAudioMedia(aud_med)
                print("[PJSUA2] Mídia de áudio conectada à chamada.") 