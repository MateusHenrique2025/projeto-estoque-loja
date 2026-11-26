# Sistema de Estoque — API em C++ + MariaDB + front-end

Sistema de estoque de uma loja: login, cadastro de produtos, entrada de mercadoria, vendas, relatório e troca de senha.
Antes era um programa de console (só Windows, dados em arquivo `.txt`); agora segue a estrutura do projeto da turma.

```
Navegador (public/*.html + app.js)
        │  fetch() com JSON + token de login
        ▼
Servidor C++ (cpp-httplib)  ──►  rotas_*.cpp  ──►  *_repositorio.cpp  ──►  MariaDB (container Docker)
```

## Estrutura

```
projeto-estoque-loja/
├── Dockerfile                compila e executa o servidor C++ em um container
├── docker-compose.yml        sobe o banco MariaDB e o servidor (2 containers)
├── build.sh / build.bat      compilam o servidor
├── schema.sql                cria as tabelas (usuarios, produtos) e o usuário admin inicial
├── libs/                     bibliotecas de terceiros (httplib.h, nlohmann/json.hpp) — não editar
├── src/
│   ├── main.cpp              liga tudo: rotas, arquivos estáticos, tratamento de erros, porta
│   ├── config.hpp            host/usuário/senha do banco e porta do servidor
│   ├── banco.hpp/.cpp        conexão com o MariaDB (consultar / executar, com parâmetros "?")
│   ├── http_util.hpp         enviarJson, enviarErro, lerCorpoObjeto, lerId, comBanco
│   ├── senha.*               hash de senha PBKDF2-HMAC-SHA256 (portátil, sem APIs do Windows)
│   ├── usuario_repositorio.* SQL da tabela "usuarios"
│   ├── rotas_auth.*          login, logout, troca de senha e proteção de todas as rotas /api/
│   ├── produto_repositorio.* TODO o SQL de produtos (CRUD, entrada, venda, relatório)
│   └── rotas_produtos.*      rotas HTTP + validação dos produtos
└── public/                   front-end (o servidor entrega estes arquivos ao navegador)
    ├── login.html/.css/.js   tela de login
    ├── index.html            tela de produtos (tabela, cadastro, busca, Entrada, Venda)
    ├── relatorio.html/.js    relatório do estoque
    ├── senha.html/.js        trocar senha
    ├── config.js             configuração da tela de produtos (campos, botões, menu)
    ├── app.js                CRUD genérico: monta tabela, formulário e busca a partir do config.js
    ├── auth.js               guarda o token de login e protege as telas
    └── estilo.css
```

## Rodando pela primeira vez (Docker — o jeito mais fácil)

Só precisa do **Docker Desktop** instalado e aberto. Na pasta do projeto:

```bash
docker compose up -d --build --wait
```

Na primeira vez baixa as imagens e compila o servidor (alguns minutos). Depois abra **http://localhost:8080** e entre com
**admin / Estoque@123** (troque a senha em *Trocar senha*).

Comandos do dia a dia: `docker compose ps` (ver se está no ar), `docker compose logs app` (mensagens do servidor),
`docker compose stop` / `docker compose start`, e **`docker compose up -d --build`** depois de mudar código em `src/`.
Mudou só o `public/`? Basta recarregar a página (Ctrl+F5).

### Alternativa: compilar o servidor direto no seu computador (sem container para o servidor)

1. Suba só o banco: `docker compose up -d --wait db`
2. Compile: `./build.sh` (MSYS2 UCRT64) ou `build.bat` (precisa de g++ e da libmariadb — veja *Problemas comuns*).
3. Execute dentro da pasta do projeto: `./build/servidor.exe` e abra http://localhost:8080.

## API

Tudo em `/api/` exige login (cabeçalho `Authorization: Bearer <token>`), exceto `/api/login` e `/api/saude*`.
O token vale 8 horas e fica só na memória do servidor (reiniciar o servidor exige novo login).
Após 5 senhas erradas seguidas para o mesmo usuário, o login é bloqueado por 1 minuto.

| Método | Rota | O que faz |
|--------|------|-----------|
| POST | `/api/login` `{usuario, senha}` | devolve `{token, usuario}` |
| POST | `/api/logout` | encerra a sessão |
| POST | `/api/trocar-senha` `{senhaAtual, novaSenha}` | troca a senha (mínimo 8 caracteres) |
| GET | `/api/produtos` (`?busca=texto`, `?situacao=baixo`) | lista |
| GET | `/api/produtos/:id` | busca um |
| POST | `/api/produtos` `{codigo, nome, categoria, preco, quantidade}` | cadastra (código repetido → 409) |
| PUT | `/api/produtos/:id` | edita |
| DELETE | `/api/produtos/:id` | exclui |
| POST | `/api/produtos/:id/entrada` `{quantidade}` | entrada de mercadoria |
| POST | `/api/produtos/:id/venda` `{quantidade}` | venda; sem estoque suficiente → 422 |
| GET | `/api/relatorio` | totais, valor do estoque e produtos com estoque baixo (≤ 5) |

Exemplo (terminal do MSYS2):

```bash
TOKEN=$(curl -s -X POST http://localhost:8080/api/login -d '{"usuario":"admin","senha":"Estoque@123"}' | sed 's/.*"token":"\([^"]*\)".*/\1/')
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/produtos
```

## Padrão das respostas

| Situação | Status | Corpo |
|----------|:------:|-------|
| Lista / busca / atualização OK | 200 | JSON (array ou objeto) |
| Criado | 201 | o registro criado |
| Removido | 204 | vazio |
| JSON inválido ou id inválido | 400 | `{"erro": "..."}` |
| Registro não encontrado / rota inexistente | 404 | `{"erro": "..."}` |
| Valor único repetido ou registro relacionado | 409 | `{"erro": "..."}` |
| Dados que não passam na validação (ou numa regra `CHECK` do banco) | 422 | `{"erro": "Dados invalidos", "detalhes": ["...", "..."]}` |
| Falha no banco / erro interno | 500 | `{"erro": "..."}` (os detalhes ficam só no console do servidor) |

## Configuração (variáveis de ambiente)

Os padrões servem para o banco do `docker-compose.yml`. Para mudar sem editar o código, defina antes de executar (no `cmd`: `set NOME=valor`; no MSYS2/bash: `export NOME=valor`):

| Variável | Padrão | Para que serve |
|----------|--------|----------------|
| `DB_HOST` | `127.0.0.1` | endereço do MariaDB |
| `DB_PORTA` | `3306` | porta do MariaDB (o `docker-compose.yml` também lê esta variável) |
| `DB_USUARIO` | `iteam` | usuário |
| `DB_SENHA` | `iteam123` | senha |
| `DB_NOME` | `projeto_iteam` | nome do banco |
| `PORTA` | `8080` | porta do servidor (também aceita `servidor.exe 8081`) |

## O banco de dados no Docker: comandos do dia a dia

Rode sempre **na pasta onde está o `docker-compose.yml`** (o Docker Desktop precisa estar aberto).

| Quero… | Comando |
|--------|---------|
| Subir o banco (e esperar ficar pronto) | `docker compose up -d --wait` |
| Ver se está no ar | `docker compose ps` (deve mostrar **healthy**) |
| Ver os registros de funcionamento (erros do banco) | `docker compose logs db` |
| Parar no fim do dia (os dados ficam guardados) | `docker compose stop` |
| **Zerar o banco** e aplicar o `schema.sql` de novo (apaga os dados!) | `docker compose down -v` e depois `docker compose up -d --wait` |
| Abrir um terminal SQL dentro do banco | `docker compose exec db mariadb -uiteam -piteam123 projeto_iteam` |
| Administrar pelo navegador (Adminer, opcional) | `docker compose --profile admin up -d`, depois <http://localhost:8081> (Sistema: MySQL; Servidor: `db`; Usuário: `iteam`; Senha: `iteam123`; Banco: `projeto_iteam`) |

- O `schema.sql` é executado **só quando o banco é criado** (volume vazio). Mudou o arquivo? Zere o banco com `down -v`.
- Os dados ficam em um *volume* do Docker e sobrevivem a `stop`/`down` e à reinicialização do computador.
- Só um banco por vez usa a porta 3306: ao trocar de pasta (por exemplo, entre dois projetos diferentes), rode `docker compose down` na pasta antiga.
- Se a porta 3306 já estiver ocupada, defina `DB_PORTA` (por exemplo, `set DB_PORTA=3307`) **antes** de subir o banco **e** ao executar o servidor.
- Usuário do banco: `iteam` / `iteam123` (aplicação) e `root` / `root` (administrador). Servem apenas para desenvolvimento local; a porta só é aberta para o seu computador.

## O que pode e o que não deve mudar

- **Podem e devem mudar:** `schema.sql`, `docker-compose.yml` (só se precisar, por exemplo, de outra porta), `src/*_repositorio.*`, `src/rotas_*`, `src/main.cpp` (para registrar rotas), `public/*`.
- **Mudem com cuidado:** `banco.*` e `http_util.hpp` — são usados por todas as rotas. Se alterar, avise a equipe.
- **Não editem:** `libs/` (código de terceiros).
- **Regra de ouro do SQL:** valores **sempre** em parâmetros `?`, nunca concatenados no texto do SQL (SQL injection).

## Problemas comuns

| Sintoma | Causa provável e o que fazer |
|---------|------------------------------|
| Sessão expirada / 401 | O servidor foi reiniciado ou o token venceu: entre de novo em `login.html`. |
| `AVISO: Nao foi possivel conectar ao banco` | O container do banco não está no ar (`docker compose ps`; suba com `docker compose up -d --wait`), o Docker Desktop está fechado, ou a porta/usuário/senha são outros (variáveis `DB_*`). |
| `Cannot connect to the Docker daemon` / `error during connect` | O **Docker Desktop não está aberto** (ou ainda está iniciando). Abra-o, espere o ícone ficar verde e tente de novo. |
| `Bind for 127.0.0.1:3306 failed: port is already allocated` | Outro banco (ou o container de outra pasta) usa a porta 3306. Rode `docker compose down` na outra pasta, ou use `DB_PORTA=3307`. |
| `Unknown database 'projeto_iteam'` ou `Table ... doesn't exist` | O banco foi criado antes de o `schema.sql` estar pronto/correto. Zere: `docker compose down -v` e `docker compose up -d --wait`. |
| O container fica `unhealthy` / reinicia sozinho | Provável erro de SQL no `schema.sql`: veja `docker compose logs db`, corrija o arquivo e zere o banco (`down -v`). |
| `mysql.h: No such file or directory` / `cannot find -lmariadb` | Falta a biblioteca cliente do MariaDB. Instale com `pacman -S mingw-w64-ucrt-x86_64-libmariadbclient` (MSYS2) ou o MariaDB Connector/C e, se necessário, defina `MARIADB_DIR`. |
| `ERRO: nao foi possivel usar a porta 8080` | Outro programa já usa a porta (talvez uma execução anterior do servidor). Feche-o ou rode `servidor.exe 8081`. |
| `AVISO: pasta ./public nao encontrada` | Você executou o servidor de outra pasta. Entre na pasta do projeto antes de executar. |
| Janela fecha ao abrir o `.exe` e/ou erro sobre `libmariadb.dll` | A DLL do MariaDB não está no `PATH`. Rode pelo terminal do MSYS2 ou copie a `libmariadb.dll` para a pasta `build/`. |
| Alterei o código e nada mudou | Compile de novo (`./build.sh`) e **reinicie** o servidor. Para mudanças só no `public/`, basta recarregar a página (Ctrl+F5). |
| Acentos estranhos no navegador | Salve os arquivos do projeto em **UTF-8**. |

## Créditos das bibliotecas

[cpp-httplib](https://github.com/yhirose/cpp-httplib) (licença MIT) e [nlohmann/json](https://github.com/nlohmann/json) (licença MIT). Os textos das licenças estão em `libs/`.
