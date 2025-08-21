@echo off
cd %~dp0\..
bazel build ...
if %errorlevel% neq 0 exit /b %errorlevel%
nvim --clean -R -u .\test\nvim_test.lua
