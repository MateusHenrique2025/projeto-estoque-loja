-- schema.sql - Cria o banco e as tabelas do Sistema de Estoque.
--
-- Este arquivo é executado AUTOMATICAMENTE pelo Docker, uma única vez, quando o container do banco é criado
-- pela primeira vez (volume vazio):   docker compose up -d --wait
--
-- Depois de alterar este arquivo, aplique de novo apagando os dados do container e subindo outra vez:
--     docker compose down -v
--     docker compose up -d --wait
--
-- Não troque o nome do banco sem avisar a equipe: o docker-compose.yml e o servidor usam "projeto_iteam"
-- (variável MARIADB_DATABASE no compose e DB_NOME no servidor).

SET NAMES utf8mb4;

CREATE DATABASE IF NOT EXISTS projeto_iteam
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE projeto_iteam;

-- Usuários que podem entrar no sistema. A senha NUNCA é guardada em texto:
-- guardamos o sal e o hash PBKDF2-HMAC-SHA256 (200000 iterações), em hexadecimal.
CREATE TABLE IF NOT EXISTS usuarios (
    id       INT UNSIGNED NOT NULL AUTO_INCREMENT,
    usuario  VARCHAR(50)  NOT NULL,
    sal      CHAR(32)     NOT NULL,
    hash     CHAR(64)     NOT NULL,
    PRIMARY KEY (id),
    CONSTRAINT uq_usuarios_usuario UNIQUE (usuario)
) ENGINE = InnoDB;

-- Produtos do estoque. "codigo" é o código digitado pela loja (único); "id" é a chave interna.
CREATE TABLE IF NOT EXISTS produtos (
    id          INT UNSIGNED  NOT NULL AUTO_INCREMENT,
    codigo      INT UNSIGNED  NOT NULL,
    nome        VARCHAR(100)  NOT NULL,
    categoria   VARCHAR(50)   NOT NULL,
    preco       DECIMAL(10,2) NOT NULL,
    quantidade  INT           NOT NULL DEFAULT 0,
    criado_em   TIMESTAMP     NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    CONSTRAINT uq_produtos_codigo UNIQUE (codigo),
    CONSTRAINT ck_produtos_preco CHECK (preco >= 0),
    CONSTRAINT ck_produtos_quantidade CHECK (quantidade >= 0)
) ENGINE = InnoDB;

-- Usuário inicial (o mesmo do sistema anterior): admin / Estoque@123  -> troque a senha pela tela "Trocar senha".
INSERT INTO usuarios (usuario, sal, hash)
SELECT 'admin', '2e4910e3c12672f48ddd4e1f22725f43', '4954cdf681803fbace47357115549e0db5301b168a7e05a0b7ce50424c403532' FROM DUAL
WHERE NOT EXISTS (SELECT 1 FROM usuarios);

-- Produtos de exemplo (material de construção). Só entram se a tabela estiver vazia.
-- Preços e quantidades são fictícios, apenas para demonstração; apague/edite pela tela do sistema.
INSERT INTO produtos (codigo, nome, categoria, preco, quantidade)
SELECT * FROM (
    SELECT 1001 AS codigo, 'Cimento CP II 50kg'          AS nome, 'Cimento e argamassa' AS categoria,  38.90 AS preco, 120 AS quantidade UNION ALL
    SELECT 1002, 'Argamassa AC-II 20kg',                          'Cimento e argamassa',                 21.50,  80 UNION ALL
    SELECT 1003, 'Cal hidratada 20kg',                            'Cimento e argamassa',                 17.90,  60 UNION ALL
    SELECT 1101, 'Areia média (saco 20kg)',                       'Agregados',                            9.90, 200 UNION ALL
    SELECT 1102, 'Brita 1 (saco 20kg)',                           'Agregados',                           11.50, 150 UNION ALL
    SELECT 1201, 'Tijolo baiano 8 furos (milheiro)',              'Alvenaria',                          890.00,  12 UNION ALL
    SELECT 1202, 'Bloco de concreto 14x19x39',                    'Alvenaria',                            4.80, 500 UNION ALL
    SELECT 1301, 'Vergalhão CA-50 8mm (barra 12m)',               'Ferragens',                           58.00,  45 UNION ALL
    SELECT 1302, 'Prego 18x27 (kg)',                              'Ferragens',                           16.90,  35 UNION ALL
    SELECT 1303, 'Arame recozido nº 18 (kg)',                     'Ferragens',                           19.50,   4 UNION ALL
    SELECT 1401, 'Tinta acrílica branca 18L',                     'Tintas',                             329.00,  14 UNION ALL
    SELECT 1402, 'Massa corrida 25kg',                            'Tintas',                              74.90,   9 UNION ALL
    SELECT 1403, 'Rolo de lã 23cm',                               'Tintas',                              14.90,  40 UNION ALL
    SELECT 1501, 'Tubo PVC esgoto 100mm (barra 6m)',              'Hidráulica',                         118.00,  18 UNION ALL
    SELECT 1502, 'Caixa d''água 1000L',                           'Hidráulica',                         549.00,   3 UNION ALL
    SELECT 1503, 'Registro de gaveta 3/4"',                       'Hidráulica',                          42.00,  25 UNION ALL
    SELECT 1601, 'Fio flexível 2,5mm (rolo 100m)',                'Elétrica',                           189.00,  16 UNION ALL
    SELECT 1602, 'Disjuntor monopolar 20A',                       'Elétrica',                            12.50,   5 UNION ALL
    SELECT 1701, 'Piso cerâmico 60x60 (caixa 2,2m²)',             'Revestimentos',                       89.90,  30 UNION ALL
    SELECT 1801, 'Luva de raspa (par)',                           'EPI',                                 13.90,   2
) AS exemplo
WHERE NOT EXISTS (SELECT 1 FROM produtos);
