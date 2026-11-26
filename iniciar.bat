
@echo off
rem iniciar.bat - Sobe o banco e o servidor no Docker e abre o navegador.
cd /d "%~dp0"

docker compose up -d --build --wait

if errorlevel 1 (
    echo.
    echo ERRO ao subir. Confira se o Docker Desktop esta aberto.
    echo Execute: docker compose logs
    pause
    exit /b 1
)

start http://localhost:8082

echo.
echo ========================================
echo SISTEMA DE ESTOQUE INICIADO COM SUCESSO!
echo ========================================
echo.
echo Endereco: http://localhost:8082
echo Usuario: admin
echo.
pause
