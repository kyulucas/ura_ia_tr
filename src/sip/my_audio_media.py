import pjsua2

class MyAudioMedia(pjsua2.AudioMedia):
    def __init__(self, audio_media):
        super().__init__()
        self.audio_media = audio_media
        # Aqui você pode conectar a media ao seu pipeline de áudio

    def onRxFrame(self, frame):
        # Recebe áudio do usuário (para STT, gravação, etc)
        print(f"[PJSUA2] onRxFrame: {len(frame.buf)} bytes")
        # Exemplo: salvar, colocar em fila, etc.
        return frame

    def onTxFrame(self, frame):
        # Envia áudio para o usuário (TTS, Echo, etc)
        print(f"[PJSUA2] onTxFrame: {len(frame.buf)} bytes")
        # Exemplo: pegar de fila, gerar áudio, etc.
        return frame 