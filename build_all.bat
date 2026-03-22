@echo off
REM Build all board environments and report results

setlocal EnableDelayedExpansion

set ENVS=m5stickcplus_11 m5stickcplus_2 m5_cardputer m5_cardputer_adv t_lora_pager t_display_16mb diy_smoochie
set PASSED=
set FAILED=

for %%e in (%ENVS%) do (
  echo ========== Building %%e ==========
  pio run -e %%e
  if !errorlevel! equ 0 (
    set "PASSED=!PASSED! %%e"
  ) else (
    set "FAILED=!FAILED! %%e"
  )
  echo.
)

echo ======================================
echo RESULTS:
echo   PASSED:!PASSED!
if defined FAILED (
  echo   FAILED:!FAILED!
  exit /b 1
) else (
  echo   All builds passed!
)
