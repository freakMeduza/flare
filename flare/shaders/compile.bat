@echo off

for /r %%i in (*.vert *.frag) do C:/VulkanSDK/1.2.182.0/Bin/glslc.exe -c "%%~i" -o "%%~i.spv"
pause