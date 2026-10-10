@echo off
setlocal
title Fantasy - POC com Assets Reais 10.98

if "%~1"=="" (
  echo.
  echo FANTASY - POC COM ASSETS REAIS
  echo.
  echo Informe a pasta que contem o pack 10.98.
  echo Voce tambem pode arrastar a pasta do pack para cima deste arquivo .bat.
  echo.
  set /p "PACK=Caminho da pasta do pack: "
) else (
  set "PACK=%~1"
)

if "%PACK%"=="" (
  echo Nenhuma pasta informada.
  pause
  exit /b 2
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-real-asset-poc.ps1" -PackRoot "%PACK%"
set "EXITCODE=%ERRORLEVEL%"

echo.
if not "%EXITCODE%"=="0" (
  echo POC FALHOU. Codigo: %EXITCODE%
  echo Veja a mensagem acima para identificar o arquivo ausente ou incompatibilidade.
) else (
  echo POC CONCLUIDO COM SUCESSO.
  echo A pasta FantasyRealAssetPoc foi criada dentro do pack informado.
)
echo.
pause
exit /b %EXITCODE%
