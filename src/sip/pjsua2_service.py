import pjsua2
from src.sip.my_account import MyAccount
from src.sip.my_call import MyCall

class PJSUA2Service:
    def __init__(self, sip_user, sip_pass, sip_server):
        self.ep = pjsua2.Endpoint()
        self.ep.libCreate()
        ep_cfg = pjsua2.EpConfig()
        self.ep.libInit(ep_cfg)
        tcfg = pjsua2.TransportConfig()
        tcfg.port = 5060
        self.ep.transportCreate(pjsua2.PJSIP_TRANSPORT_UDP, tcfg)
        self.ep.libStart()
        print("[PJSUA2] Endpoint iniciado.")

        acc_cfg = pjsua2.AccountConfig()
        acc_cfg.idUri = f"sip:{sip_user}@{sip_server}"
        acc_cfg.regConfig.registrarUri = f"sip:{sip_server}"
        cred = pjsua2.AuthCredInfo("digest", sip_server, sip_user, 0, sip_pass)
        acc_cfg.sipConfig.authCreds.append(cred)
        self.account = MyAccount()
        self.account.create(acc_cfg)
        print("[PJSUA2] Conta criada e registro iniciado.")

    def make_call(self, dest_uri):
        call = MyCall(self.account, dest_uri)
        call.make_call()
        return call

    def shutdown(self):
        self.ep.libDestroy()
        print("[PJSUA2] Endpoint finalizado.") 