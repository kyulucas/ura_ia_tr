import pjsua2
from src.sip.my_call import MyCall

class MyAccount(pjsua2.Account):
    def __init__(self):
        super().__init__()

    def onRegState(self, prm):
        info = self.getInfo()
        print(f"[PJSUA2] Registro: {info.regIsActive} - {info.regStatus} {info.regReason}")

    def onIncomingCall(self, prm):
        print("[PJSUA2] Chamada recebida!")
        call = MyCall(self, prm.callId)
        call.answer_call() 