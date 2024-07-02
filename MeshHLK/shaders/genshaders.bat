dxc.exe simpleShaders.hlsl -Fh VS_simple.hlsl.h /Zi -T vs_6_0 -E VS_simple -Qembed_debug
dxc.exe simpleShaders.hlsl -Fh PS_simple.hlsl.h /Zi -T ps_6_0 -E PS_simple -Qembed_debug
dxc.exe simpleShaders.hlsl -Fh GS_simple.hlsl.h /Zi -T gs_6_0 -E GS_simple -Qembed_debug
dxc.exe simpleShaders.hlsl -Fh HS_simple.hlsl.h /Zi -T hs_6_0 -E HS_simple -Qembed_debug
dxc.exe simpleShaders.hlsl -Fh DS_simple.hlsl.h /Zi -T ds_6_0 -E DS_simple -Qembed_debug
dxc.exe simpleShaders.hlsl -Fh CS_simple.hlsl.h /Zi -T cs_6_0 -E CS_simple -Qembed_debug

dxc.exe TestShaders.hlsl -Fh VS.hlsl.h -T vs_6_1 -E VS 
dxc.exe TestShaders.hlsl -Fh VSLine.hlsl.h -T vs_6_1 -E VSLine
dxc.exe TestShaders.hlsl -Fh VSSysVal.hlsl.h -T vs_6_1 -E VSSysVal

dxc.exe TestShaders.hlsl -Fh PS.hlsl.h -T ps_6_1 -E PS
dxc.exe TestShaders.hlsl -Fh PSViewInstancing.hlsl.h -T ps_6_1 -E PSViewInstancing
dxc.exe TestShaders.hlsl -Fh PSSysVal.hlsl.h -T ps_6_1 -E PSSysVal
dxc.exe TestShaders.hlsl -Fh PSPrimitiveID.hlsl.h -T ps_6_1 -E PSPrimitiveID

dxc.exe TestShaders.hlsl -Fh ASSmallPayload.hlsl.h -T as_6_5 -E ASSmallPayload
dxc.exe TestShaders.hlsl -Fh ASLargePayload.hlsl.h -T as_6_5 -E ASLargePayload
dxc.exe TestShaders.hlsl -Fh ASNoPS.hlsl.h -T as_6_5 -E ASNoPS
dxc.exe TestShaders.hlsl -Fh ASPrimitiveID.hlsl.h -T as_6_5 -E ASPrimitiveID
dxc.exe TestShaders.hlsl -Fh ASRayQuery.hlsl.h -T as_6_5 -E ASRayQuery

dxc.exe TestShaders.hlsl -Fh MSSmall.hlsl.h -T ms_6_5 -E MSSmall
dxc.exe TestShaders.hlsl -Fh MSMedium.hlsl.h -T ms_6_5 -E MSMedium
dxc.exe TestShaders.hlsl -Fh MSMediumLine.hlsl.h -T ms_6_5 -E MSMediumLine 
dxc.exe TestShaders.hlsl -Fh MSLarge.hlsl.h -T ms_6_5 -E MSLarge
dxc.exe TestShaders.hlsl -Fh MSSmallFromAS.hlsl.h -T ms_6_5 -E MSSmallFromAS
dxc.exe TestShaders.hlsl -Fh MSMediumFromAS.hlsl.h -T ms_6_5 -E MSMediumFromAS
dxc.exe TestShaders.hlsl -Fh MSMediumLineFromAS.hlsl.h -T ms_6_5 -E MSMediumLineFromAS
dxc.exe TestShaders.hlsl -Fh MSLargeFromAS.hlsl.h -T ms_6_5 -E MSLargeFromAS
dxc.exe TestShaders.hlsl -Fh MSNoPS.hlsl.h -T ms_6_5 -E MSNoPS
dxc.exe TestShaders.hlsl -Fh MSViewInstancing.hlsl.h -T ms_6_5 -E MSViewInstancing
dxc.exe TestShaders.hlsl -Fh MSLargeViewInstancingFromAS.hlsl.h -T ms_6_5 -E MSLargeViewInstancingFromAS
dxc.exe TestShaders.hlsl -Fh MSSmallViewInstancingFromAS.hlsl.h -T ms_6_5 -E MSSmallViewInstancingFromAS
dxc.exe TestShaders.hlsl -Fh MSVariableRateShading.hlsl.h -T ms_6_5 -E MSVariableRateShading
dxc.exe TestShaders.hlsl -Fh MSSysVal.hlsl.h -T ms_6_5 -E MSSysVal
dxc.exe TestShaders.hlsl -Fh MSPrimitiveID.hlsl.h -T ms_6_5 -E MSPrimitiveID
dxc.exe TestShaders.hlsl -Fh MSPrimitiveIDFromAS.hlsl.h -T ms_6_5 -E MSPrimitiveIDFromAS
dxc.exe TestShaders.hlsl -Fh MSRayQuery.hlsl.h -T ms_6_5 -E MSRayQuery
dxc.exe TestShaders.hlsl -Fh MSRayQueryFromAS.hlsl.h -T ms_6_5 -E MSRayQueryFromAS
dxc.exe TestShaders.hlsl -Fh MSCullAllPrimitives.hlsl.h -T ms_6_5 -E MSCullAllPrimitives

dxc.exe TestShaders.hlsl -Fh VSViewInstancing.hlsl.h -T vs_6_5 -E VSViewInstancing
dxc.exe TestShaders.hlsl -Fh VSVRS.hlsl.h -T vs_6_5 -E VSVRS

dxc.exe TestShaders.hlsl -Fh PSRayQuery.hlsl.h -T ps_6_5 -E PSRayQuery
dxc.exe TestShaders.hlsl -Fh PSVRS.hlsl.h -T ps_6_5 -E PSVRS


 


 

