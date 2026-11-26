# Dockerfile - Compila o servidor em C++ e o executa em uma imagem enxuta.
FROM ubuntu:24.04 AS compilar
RUN apt-get update && apt-get install -y --no-install-recommends g++ libmariadb-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /projeto
COPY build.sh ./
COPY libs ./libs
COPY src ./src
# sed: garante fim de linha Unix no script, mesmo que o arquivo tenha sido salvo no Windows (CRLF)
RUN sed -i 's/\r$//' build.sh && chmod +x build.sh && ./build.sh

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends libmariadb3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=compilar /projeto/build/servidor ./servidor
COPY public ./public
EXPOSE 8080
CMD ["./servidor"]

