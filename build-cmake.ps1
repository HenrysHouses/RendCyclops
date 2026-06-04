$env:CMAKE_EXPORT_COMPILE_COMMANDS = "ON"
Set-Location "$Script:PSScriptRoot/build"
cmake -S .. -B . -G Ninja
Set-Location "$Script:PSScriptRoot"
