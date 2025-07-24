Roteiro de Projeto: Agente de Voz Conversacional sobre SIP (Versão Refinada)

1. Ambiente de Desenvolvimento
Seu Plano: Subir um servidor SIP (Asterisk ou FreeSWITCH) em Docker. Preparar um ambiente Python 3.12+ para backend. O microserviço SIP (ura_grpc) é compilado e executado via Docker multi-stage, garantindo reprodutibilidade e isolamento.

Comentários e Adaptações:
- O microserviço SIP (C++/PJSIP/gRPC) é construído automaticamente via Dockerfile multi-stage, que compila todas as dependências do source (PJSIP, Protobuf, gRPC, Speex, GSM, iLBC, SRTP).
- O backend Python comunica-se com o microserviço via gRPC, desacoplando a lógica SIP do restante do sistema.
- O código do microserviço é versionado em repositório Git e clonado no build Docker, facilitando CI/CD.

Ação Concreta: Crie um arquivo pjsip.conf básico dentro do seu projeto e monte-o como um volume no contêiner Docker do Asterisk. Configure duas extensões SIP: uma para o bot e outra para o cliente de teste.

2. Cliente SIP
Seu Plano: Utilizar o microserviço ura_grpc (C++/PJSIP/gRPC) para registrar e gerenciar chamadas. O backend Python interage via gRPC.

Comentários e Adaptações:
- O controle de chamadas, áudio e eventos SIP é feito no microserviço C++.
- O backend Python pode ser testado independentemente, simulando chamadas gRPC.

3. Pipeline de Áudio
Seu Plano: Implementar callbacks no microserviço para capturar áudio RTP e expor via gRPC.

Comentários e Adaptações:
- O backend Python pode consumir chunks de áudio via gRPC streaming.
- O microserviço pode ser estendido para expor eventos e áudio em tempo real.

4. Echo Bot (Primeiro MVP)
Seu Plano: Integrar STT/TTS no backend Python, que responde via gRPC.

Comentários e Adaptações:
- O microserviço SIP apenas encaminha áudio e eventos; a lógica de IA fica no backend Python.

5. IA Conversacional
Seu Plano: Substituir o Echo Bot pela IA e manter o contexto no backend Python.

6. Otimizações
Seu Plano: Implementar VAD, barge-in e streaming no backend Python, usando eventos do microserviço via gRPC.

7. Escalabilidade
Seu Plano: Estruturar para múltiplas sessões SIP, monitorar custos e usar asyncio no backend Python.

Revisão dos Artefatos
- O Dockerfile multi-stage automatiza o build do microserviço SIP e suas dependências.
- O repositório Git centraliza o código e facilita CI/CD.

## Estrutura Sugerida de Pastas do Projeto
```
/ura_grpc         # Microserviço SIP (C++/PJSIP/gRPC)
  Dockerfile
  CMakeLists.txt
  server.cpp
  sip_service.proto
  ...
/src              # Backend Python
  /sip
  /audio
  /stt
  /tts
  /ia
  /utils
  main.py
/config
  pjsip.conf
  .env
/tests
requirements.txt
README.md
README_DOCKER.md
docker-compose.yml
roteiro_de_projeto.md
```

## Exemplo de docker-compose.yml
```yaml
version: '3'
services:
  asterisk:
    image: andrius/asterisk
    volumes:
      - ./config/pjsip.conf:/etc/asterisk/pjsip.conf
    ports:
      - "5060:5060/udp"
      - "5060:5060/tcp"
  bot:
    build: ./ura_grpc
    env_file:
      - ../config/.env
    ports:
      - "50051:50051"
    depends_on:
      - asterisk
```

## Exemplo de .env
```env
SIP_USERNAME=1000
SIP_PASSWORD=senha_super_secreta
SIP_DOMAIN=asterisk
OPENAI_API_KEY=sua-chave
GOOGLE_APPLICATION_CREDENTIALS=/app/config/google-credentials.json
```

## Testes Automatizados
- Crie testes unitários e de integração para:
  - Funções de manipulação de áudio
  - Integração com STT/TTS (mockando APIs)
  - Máquina de estados

## Monitoramento e Logs
- Utilize uma biblioteca de logging estruturado (ex: `structlog` ou `loguru`).
- Considere adicionar monitoração básica (Prometheus, Grafana, ou logs para um ELK stack).

## Documentação
- Mantenha um README.md atualizado com:
  - Como rodar o projeto
  - Como configurar o ambiente
  - Como rodar os testes
  - Fluxo básico de chamadas