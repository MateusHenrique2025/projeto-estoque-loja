@echo off
rem resetar-banco.bat - APAGA todos os dados do banco e recria tudo do zero (tabelas + produtos de exemplo).
rem Use quando aparecer "erro no banco de dados" / "table doesn't exist", ou depois de alterar o schema.sql.
cd /d "%~dp0"
echo ATENCAO: isto apaga todos os produtos e usuarios cadastrados e volta ao estado inicial.
choice /M "Continuar"
if errorlevel 2 exit /b 0
docker compose down -v
docker compose up -d --build --wait
if errorlevel 1 (
    echo.
    echo ERRO. Rode:  docker compose logs
    pause
    exit /b 1
)
start http://localhost:8080
pause
