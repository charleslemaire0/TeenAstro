@echo off
REM run_all_tests.bat - Run all TeenAstro math library unit tests
REM
REM Usage:
REM   tests\run_all_tests.bat               -- run all suites
REM   tests\run_all_tests.bat test_la3      -- run one suite
REM

cd /d "%~dp0"
python run_all_tests.py %*
