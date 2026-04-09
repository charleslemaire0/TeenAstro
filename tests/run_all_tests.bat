@echo off
REM run_all_tests.bat - Run all TeenAstro native unit tests (+ optional emulator regression)
REM
REM Usage:
REM   tests\run_all_tests.bat                    -- all discovered test_* suites
REM   tests\run_all_tests.bat --with-emulator    -- + GXAS flip script (mainunit_emu on :9997)
REM   tests\run_all_tests.bat test_la3           -- one suite
REM

cd /d "%~dp0"
python run_all_tests.py %*
