# Docker Build & Run para ura_grpc (PJSIP + gRPC)

## Pré-requisitos
- Docker instalado
- Acesso ao repositório: https://github.com/kyulucas/ura_ia_tr.git

## Build da imagem

```bash
docker build \
  --build-arg GIT_REPO_URL=https://github.com/kyulucas/ura_ia_tr.git \
  --build-arg GIT_REPO_BRANCH=main \
  -t ura-grpc-server .
```

## Variáveis de ambiente obrigatórias
- `SIP_USERNAME`: Usuário SIP
- `SIP_PASSWORD`: Senha SIP
- `SIP_DOMAIN`: Domínio/servidor SIP

Você pode passar essas variáveis no `docker run` ou via arquivo `.env`.

## Rodando o container

```bash
docker run --rm -p 50051:50051 \
  -e SIP_USERNAME=usuario \
  -e SIP_PASSWORD=senha \
  -e SIP_DOMAIN=dominio.sip.com \
  ura-grpc-server
```

## Health check opcional
- O Dockerfile pode rodar `client.py` ao final do build para validar o serviço (opcional, não bloqueante).

## Observações
- Todas as dependências (PJSIP, Protobuf, gRPC, Speex, GSM, iLBC, SRTP) são compiladas do source para máxima reprodutibilidade.
- O build pode demorar na primeira vez devido ao download e compilação de dependências grandes.
- O executável final e as bibliotecas necessárias são copiadas para uma imagem mínima de runtime.

---

Se precisar de ajustes para CI/CD, multi-arquitetura ou outros detalhes, edite este arquivo ou o Dockerfile conforme necessário. 