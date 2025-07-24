# Agente de Voz Conversacional sobre SIP

## Visão Geral
Este projeto implementa um backend Python para integração com SIP, IA (STT, TTS, GPT) e automação de voz, usando uma arquitetura moderna baseada em microserviço C++ (PJSIP + gRPC). O sistema é multiplataforma e reprodutível via Docker.

## Novidades e Mudanças Recentes
- **Nova arquitetura SIP:** O antigo módulo baresipy foi removido. Agora, toda a lógica SIP roda em um microserviço C++ dedicado, compilado e empacotado via Docker multi-stage.
- **Comunicação via gRPC:** O backend Python comunica-se com o microserviço SIP usando gRPC, desacoplando a lógica de telefonia do restante do sistema.
- **Build automatizado:** O build do microserviço e de todas as dependências (PJSIP, Protobuf, gRPC, codecs) é feito do zero via Docker, garantindo reprodutibilidade e isolamento.
- **Repositório Git:** O código do microserviço é clonado automaticamente no build Docker, facilitando CI/CD e automação.

## Como rodar com Docker
1. **Build da imagem:**
   ```bash
   docker build \
     --build-arg GIT_REPO_URL=https://github.com/kyulucas/ura_ia_tr.git \
     --build-arg GIT_REPO_BRANCH=main \
     -t ura-grpc-server .
   ```
2. **Execute o container:**
   ```bash
   docker run --rm -p 50051:50051 \
     -e SIP_USERNAME=usuario \
     -e SIP_PASSWORD=senha \
     -e SIP_DOMAIN=dominio.sip.com \
     ura-grpc-server
   ```
3. **Backend Python:**
   - O backend Python pode se comunicar com o microserviço SIP via gRPC na porta 50051.

Para detalhes completos, consulte o arquivo `README_DOCKER.md`.

## Notas importantes
- Todas as dependências SIP (PJSIP, codecs, Protobuf, gRPC) são compiladas do source no Dockerfile.
- O build é reprodutível e não depende do ambiente do host.
- O backend Python pode ser desenvolvido e testado separadamente, comunicando-se via gRPC.

## Continuidade
- Consulte o arquivo `contexto_ia.txt` para um resumo detalhado do histórico, decisões técnicas e próximos passos do projeto.
- Consulte o `README_DOCKER.md` para instruções detalhadas de build e execução do microserviço SIP.
- Para dúvidas sobre SIP, integração com IA ou automação, consulte a documentação dos respectivos pacotes ou envie o contexto para o agente de IA. 