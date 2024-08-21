:: change path to point to dxc appropriately (don't use vk SDK one as that one can't sign the bin)

"c:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\dxc.exe" -T vs_6_6 -E VsMain -Fo vs.bin Shaders.hlsl

"c:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\dxc.exe" -T ps_6_6 -E PsFeedback -Fo ps.bin Shaders.hlsl

"c:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\dxc.exe" -T cs_6_6 -E GenMips -Fo mipgen.bin Mipgen.hlsl
