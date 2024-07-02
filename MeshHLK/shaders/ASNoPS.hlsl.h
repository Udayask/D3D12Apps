#if 0
;
; Input signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
;
; Output signature:
;
; Name                 Index   Mask Register SysValue  Format   Used
; -------------------- ----- ------ -------- -------- ------- ------
; no parameters
; shader hash: 80eb9faf349828434b39c52cfeaa5b9a
;
; Pipeline Runtime Information: 
;
; Amplification Shader
; NumThreads=(1,128,1)
;
;
; Buffer Definitions:
;
; Resource bind info for LogUAV
; {
;
;   struct struct.LogLayout
;   {
;
;       uint ASInvocations;                           ; Offset:    0
;       uint MSInvocations;                           ; Offset:    4
;       uint PSInvocations;                           ; Offset:    8
;       uint Cookie;                                  ; Offset:   12
;   
;   } $Element;                                       ; Offset:    0 Size:    16
;
; }
;
;
; Resource Bindings:
;
; Name                                 Type  Format         Dim      ID      HLSL Bind  Count
; ------------------------------ ---------- ------- ----------- ------- -------------- ------
; LogUAV                                UAV  struct         r/w      U0             u0     1
;
target datalayout = "e-m:e-p:32:32-i1:32-i8:32-i16:32-i32:32-i64:64-f16:32-f32:32-f64:64-n8:16:32:64"
target triple = "dxil-ms-dx"

%dx.types.Handle = type { i8* }
%struct.smallPayload = type { i32, i32 }
%"class.RWStructuredBuffer<LogLayout>" = type { %struct.LogLayout }
%struct.LogLayout = type { i32, i32, i32, i32 }

define void @ASNoPS() {
  %1 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %2 = call i32 @dx.op.groupId.i32(i32 94, i32 2)  ; GroupId(component)
  %3 = alloca %struct.smallPayload, align 4
  %4 = call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle %1, i32 0, i32 0, i32 0, i32 undef, i32 1)  ; AtomicBinOp(handle,atomicOp,offset0,offset1,offset2,newValue)
  %5 = getelementptr inbounds %struct.smallPayload, %struct.smallPayload* %3, i32 0, i32 0
  store i32 %2, i32* %5, align 4, !tbaa !12
  %6 = getelementptr inbounds %struct.smallPayload, %struct.smallPayload* %3, i32 0, i32 1
  store i32 1024, i32* %6, align 4, !tbaa !12
  call void @dx.op.dispatchMesh.struct.smallPayload(i32 173, i32 1, i32 1, i32 1024, %struct.smallPayload* nonnull %3)  ; DispatchMesh(threadGroupCountX,threadGroupCountY,threadGroupCountZ,payload)
  ret void
}

; Function Attrs: nounwind readnone
declare i32 @dx.op.groupId.i32(i32, i32) #0

; Function Attrs: nounwind
declare i32 @dx.op.atomicBinOp.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32) #1

; Function Attrs: nounwind
declare void @dx.op.dispatchMesh.struct.smallPayload(i32, i32, i32, i32, %struct.smallPayload*) #1

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #2

attributes #0 = { nounwind readnone }
attributes #1 = { nounwind }
attributes #2 = { nounwind readonly }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!8}

!0 = !{!"dxcoob 1.8.2405.15 (fd7e54bcd)"}
!1 = !{i32 1, i32 5}
!2 = !{i32 1, i32 8}
!3 = !{!"as", i32 6, i32 5}
!4 = !{null, !5, null, null}
!5 = !{!6}
!6 = !{i32 0, %"class.RWStructuredBuffer<LogLayout>"* undef, !"", i32 0, i32 0, i32 1, i32 12, i1 false, i1 false, i1 false, !7}
!7 = !{i32 1, i32 16}
!8 = !{void ()* @ASNoPS, !"ASNoPS", null, !4, !9}
!9 = !{i32 0, i64 16, i32 10, !10}
!10 = !{!11, i32 8}
!11 = !{i32 1, i32 128, i32 1}
!12 = !{!13, !13, i64 0}
!13 = !{!"int", !14, i64 0}
!14 = !{!"omnipotent char", !15, i64 0}
!15 = !{!"Simple C/C++ TBAA"}

#endif

const unsigned char g_ASNoPS[] = {
  0x44, 0x58, 0x42, 0x43, 0x4e, 0xad, 0x8e, 0x22, 0xc5, 0x30, 0xb3, 0x1a,
  0x33, 0x32, 0x93, 0x3c, 0x48, 0xa5, 0x08, 0x03, 0x01, 0x00, 0x00, 0x00,
  0x10, 0x0d, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00,
  0x4c, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
  0xdc, 0x00, 0x00, 0x00, 0x0c, 0x07, 0x00, 0x00, 0x28, 0x07, 0x00, 0x00,
  0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x50, 0x53, 0x56, 0x30, 0x68, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x41, 0x53, 0x4e, 0x6f, 0x50, 0x53, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x53, 0x54, 0x41, 0x54, 0x28, 0x06, 0x00, 0x00,
  0x65, 0x00, 0x0e, 0x00, 0x8a, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
  0x05, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x06, 0x00, 0x00,
  0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0x81, 0x01, 0x00, 0x00,
  0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39,
  0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62,
  0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xa4, 0x10, 0x32, 0x14,
  0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88, 0x48, 0x90, 0x14, 0x20,
  0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90,
  0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a,
  0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0x02, 0xa8, 0x0d,
  0x84, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20, 0x6d, 0x30, 0x86, 0xff,
  0xff, 0xff, 0xff, 0x1f, 0x00, 0x09, 0xa8, 0x00, 0x49, 0x18, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x4c, 0x08, 0x06,
  0x00, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
  0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04, 0x93, 0x22, 0xa4, 0x84,
  0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c,
  0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x5c, 0x23, 0x00, 0x25, 0x00, 0x14, 0xe6,
  0x08, 0xc0, 0xa0, 0x0c, 0x63, 0x0c, 0x22, 0x73, 0x04, 0x08, 0x99, 0x7b,
  0x86, 0xcb, 0x9f, 0xb0, 0x87, 0x90, 0xfc, 0x10, 0x68, 0x86, 0x85, 0x40,
  0xc1, 0x29, 0xc8, 0x18, 0x68, 0x8c, 0x31, 0x06, 0xa5, 0x9b, 0xa4, 0x29,
  0xa2, 0x84, 0xc9, 0x97, 0x18, 0x60, 0x59, 0x52, 0x00, 0x5b, 0x1c, 0x60,
  0x40, 0xc2, 0x18, 0xb4, 0x8a, 0x01, 0xc6, 0x18, 0x83, 0x51, 0x9b, 0x23,
  0x08, 0x8a, 0x81, 0x86, 0x19, 0xe3, 0x11, 0x1c, 0x08, 0x38, 0x48, 0x9a,
  0x22, 0x4a, 0x98, 0xfc, 0xd2, 0x31, 0x4a, 0x00, 0x73, 0xa8, 0x09, 0x11,
  0x63, 0x8c, 0x31, 0xd3, 0x18, 0x8c, 0x03, 0x3b, 0x84, 0xc3, 0x3c, 0xcc,
  0x83, 0x1b, 0xc8, 0xc2, 0x2d, 0xcc, 0x02, 0x3d, 0xc8, 0x43, 0x3d, 0x8c,
  0x03, 0x3d, 0xd4, 0x83, 0x3c, 0x94, 0x03, 0x39, 0x88, 0x42, 0x3d, 0x98,
  0x83, 0x39, 0x94, 0x83, 0x3c, 0xf0, 0x01, 0x2b, 0xbc, 0xc3, 0x39, 0xb0,
  0x42, 0x38, 0xe4, 0xc3, 0x3b, 0xd4, 0x03, 0x3d, 0xf8, 0x01, 0x0a, 0x26,
  0xd1, 0x39, 0x02, 0x50, 0x00, 0x00, 0x00, 0x00, 0x13, 0x14, 0x72, 0xc0,
  0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0,
  0x87, 0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d,
  0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d,
  0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78,
  0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a,
  0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73,
  0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60, 0x07, 0x74,
  0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d,
  0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6,
  0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78,
  0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76,
  0x40, 0x07, 0x43, 0x9e, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x86, 0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x12, 0x20, 0x00, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x34, 0x40, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4, 0x81, 0x80, 0x00, 0x18,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x04, 0x00, 0x00,
  0x0e, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11, 0x4c, 0x90,
  0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x25, 0x50, 0x0a, 0x23,
  0x00, 0xc5, 0x50, 0x18, 0x05, 0x52, 0x06, 0xe5, 0x50, 0x08, 0x05, 0x51,
  0x14, 0x05, 0x28, 0x40, 0xb6, 0x40, 0xe8, 0x8d, 0x00, 0xd0, 0x9c, 0x01,
  0x20, 0x3a, 0x03, 0x40, 0x75, 0x06, 0x80, 0xd6, 0x0c, 0x00, 0x00, 0x00,
  0x79, 0x18, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90,
  0x46, 0x02, 0x13, 0x44, 0x8f, 0x0c, 0x6f, 0xec, 0xed, 0x4d, 0x0c, 0x24,
  0xc6, 0x05, 0xc7, 0x45, 0x86, 0x06, 0xa6, 0xc6, 0x25, 0xa6, 0x06, 0x04,
  0xc5, 0x8c, 0xec, 0xa6, 0xac, 0x86, 0x46, 0x6c, 0x8c, 0x2c, 0x65, 0x43,
  0x10, 0x4c, 0x10, 0x86, 0x62, 0x82, 0x30, 0x18, 0x1b, 0x84, 0x81, 0x98,
  0x20, 0x0c, 0xc7, 0x06, 0xc1, 0x30, 0x28, 0x84, 0xcd, 0x4d, 0x10, 0x06,
  0x64, 0xc3, 0x80, 0x24, 0xc4, 0x04, 0xa1, 0xaa, 0x68, 0x30, 0xbd, 0x9d,
  0x55, 0x05, 0x59, 0x4d, 0x10, 0x86, 0x64, 0x82, 0xf0, 0x48, 0x13, 0x84,
  0x41, 0xd9, 0x20, 0x0c, 0xcf, 0x86, 0xc5, 0x58, 0x18, 0xc3, 0x18, 0x1a,
  0xc7, 0x71, 0xa0, 0x0d, 0x41, 0xb4, 0x81, 0x00, 0x24, 0x00, 0x98, 0x20,
  0x50, 0x14, 0x03, 0xb4, 0x09, 0xc2, 0xb0, 0x6c, 0x20, 0x92, 0xca, 0x32,
  0x26, 0x08, 0xd3, 0xb4, 0x41, 0x30, 0xb0, 0x0d, 0x41, 0xb6, 0x41, 0x30,
  0xb4, 0x0d, 0xc3, 0x73, 0x6d, 0x6c, 0x82, 0xa6, 0x92, 0xdc, 0xec, 0xde,
  0xc6, 0xc2, 0xe8, 0xd2, 0xde, 0xdc, 0xe6, 0x26, 0x08, 0x03, 0xb3, 0xc1,
  0x48, 0x3a, 0xcb, 0xf0, 0x08, 0x36, 0x4d, 0x53, 0x49, 0x6e, 0x76, 0x6f,
  0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x13, 0x84, 0xa1, 0xd9, 0x60,
  0x24, 0x60, 0x60, 0x85, 0x81, 0x47, 0xb0, 0x81, 0x9a, 0x4a, 0x72, 0xb3,
  0x7b, 0x1b, 0x0b, 0xa3, 0x4b, 0x7b, 0x73, 0x9b, 0x9b, 0x20, 0x0c, 0xce,
  0x06, 0x23, 0x19, 0x03, 0x8b, 0x0c, 0x3c, 0x82, 0xc6, 0xd0, 0xdb, 0xdb,
  0x5a, 0x5a, 0xd9, 0x06, 0x23, 0x31, 0x03, 0xab, 0xf1, 0x88, 0x0d, 0xc5,
  0xf3, 0x89, 0x41, 0x19, 0x9c, 0xc1, 0x04, 0x61, 0xb1, 0xa8, 0x84, 0xc9,
  0xc9, 0x85, 0xe5, 0x4d, 0xb1, 0xa5, 0x8d, 0x95, 0x6d, 0x30, 0x12, 0x35,
  0xb0, 0x0c, 0x8f, 0x60, 0xe2, 0x56, 0xd7, 0x76, 0x24, 0xf7, 0x56, 0x07,
  0x37, 0xb7, 0xc1, 0x48, 0xd8, 0xc0, 0x0a, 0x03, 0x8f, 0xd8, 0x30, 0x90,
  0xc1, 0x1a, 0xb4, 0xc1, 0x86, 0xc3, 0xa0, 0x38, 0x0c, 0x0d, 0xd2, 0xc0,
  0x0d, 0x26, 0x08, 0x02, 0xb0, 0x01, 0xd8, 0x30, 0x18, 0x71, 0x10, 0x07,
  0x1b, 0x02, 0x39, 0xd8, 0x30, 0x0c, 0x70, 0x30, 0x07, 0x34, 0x82, 0xa6,
  0x9c, 0xde, 0xa0, 0xa6, 0x26, 0x08, 0x56, 0x34, 0x41, 0x18, 0x9e, 0x09,
  0xc2, 0x00, 0x6d, 0x18, 0x06, 0x3c, 0x18, 0x36, 0x08, 0x79, 0x40, 0x06,
  0x1b, 0x08, 0xc3, 0x0e, 0xee, 0x40, 0x0f, 0x36, 0x14, 0x70, 0x50, 0x07,
  0xc0, 0xb4, 0x07, 0x34, 0xc2, 0xe8, 0xde, 0xda, 0xd2, 0xc6, 0x58, 0xa4,
  0xb9, 0xcd, 0xd1, 0xcd, 0x6d, 0x20, 0xfa, 0x60, 0xf0, 0x83, 0x3b, 0xa8,
  0xc2, 0xc6, 0x66, 0xd7, 0xe6, 0x92, 0x46, 0x56, 0xe6, 0x46, 0x37, 0x25,
  0x08, 0xaa, 0x90, 0xe1, 0xb9, 0xd8, 0x95, 0xc9, 0xcd, 0xa5, 0xbd, 0xb9,
  0x4d, 0x09, 0x88, 0x26, 0x64, 0x78, 0x2e, 0x76, 0x61, 0x6c, 0x76, 0x65,
  0x72, 0x53, 0x02, 0xa3, 0x0e, 0x19, 0x9e, 0xcb, 0x1c, 0x5a, 0x18, 0x59,
  0x99, 0x5c, 0xd3, 0x1b, 0x59, 0x19, 0xdb, 0x94, 0x20, 0x29, 0x43, 0x86,
  0xe7, 0x22, 0x57, 0x36, 0xf7, 0x56, 0x27, 0x37, 0x56, 0x36, 0x37, 0x25,
  0x90, 0x2a, 0x91, 0xe1, 0xb9, 0xd0, 0xe5, 0xc1, 0x95, 0x05, 0xb9, 0xb9,
  0xbd, 0xd1, 0x85, 0xd1, 0xa5, 0xbd, 0xb9, 0xcd, 0x4d, 0x11, 0xdc, 0x60,
  0x0e, 0xea, 0x90, 0xe1, 0xb9, 0x94, 0xb9, 0xd1, 0xc9, 0xe5, 0x41, 0xbd,
  0xa5, 0xb9, 0xd1, 0xcd, 0x4d, 0x09, 0xf6, 0xa0, 0x0b, 0x19, 0x9e, 0xcb,
  0xd8, 0x5b, 0x9d, 0x1b, 0x5d, 0x99, 0xdc, 0xdc, 0x94, 0xc0, 0x0f, 0x00,
  0x79, 0x18, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c,
  0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3,
  0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6,
  0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e,
  0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43,
  0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03,
  0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48,
  0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20,
  0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e,
  0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d,
  0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89,
  0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83,
  0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68,
  0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90,
  0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78,
  0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98,
  0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5,
  0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c,
  0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c,
  0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43,
  0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43,
  0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82,
  0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70,
  0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19, 0xd1, 0x43, 0x0e, 0xf8,
  0xe0, 0x06, 0xe4, 0x20, 0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2,
  0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x00, 0x00, 0x00,
  0x71, 0x20, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x06, 0x60, 0x68, 0xec,
  0xe9, 0xa4, 0xac, 0x09, 0x54, 0xc3, 0xe5, 0x3b, 0x8f, 0x0f, 0x4c, 0x0e,
  0x83, 0x08, 0x1b, 0xd2, 0xa0, 0x8f, 0x8f, 0xdc, 0xb6, 0x11, 0x48, 0xc3,
  0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04, 0x30, 0x11, 0x21, 0xd0, 0x0c, 0x0b,
  0x61, 0x03, 0x66, 0x30, 0x5c, 0xbe, 0xf3, 0xf8, 0x03, 0x22, 0x3d, 0xc0,
  0x24, 0x1c, 0x26, 0x21, 0x1d, 0xbe, 0x34, 0x45, 0x94, 0x30, 0xf9, 0x12,
  0x03, 0x2c, 0x4b, 0x0a, 0x60, 0x8b, 0x03, 0x0c, 0x16, 0x10, 0x0d, 0x97,
  0xef, 0x3c, 0xbe, 0x11, 0x39, 0xd4, 0x23, 0x0e, 0x3e, 0x72, 0xdb, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x80, 0xeb, 0x9f, 0xaf, 0x34, 0x98, 0x28, 0x43,
  0x4b, 0x39, 0xc5, 0x2c, 0xfe, 0xaa, 0x5b, 0x9a, 0x44, 0x58, 0x49, 0x4c,
  0xe0, 0x05, 0x00, 0x00, 0x65, 0x00, 0x0e, 0x00, 0x78, 0x01, 0x00, 0x00,
  0x44, 0x58, 0x49, 0x4c, 0x05, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0xc8, 0x05, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00,
  0x6f, 0x01, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49,
  0x06, 0x10, 0x32, 0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19,
  0x1e, 0x04, 0x8b, 0x62, 0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42,
  0xa4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88,
  0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42,
  0xe4, 0x48, 0x0e, 0x90, 0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c,
  0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00,
  0x0a, 0x00, 0x00, 0x00, 0x1b, 0x8c, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07,
  0x40, 0x02, 0xa8, 0x0d, 0x84, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x03, 0x20,
  0x6d, 0x30, 0x86, 0xff, 0xff, 0xff, 0xff, 0x1f, 0x00, 0x09, 0xa8, 0x36,
  0x10, 0x44, 0x01, 0x9c, 0x01, 0x00, 0x00, 0x00, 0x49, 0x18, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42, 0x20, 0x4c, 0x08, 0x86,
  0x09, 0x01, 0x01, 0x00, 0x89, 0x20, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x00,
  0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04, 0x93, 0x22, 0xa4, 0x84,
  0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c,
  0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x60, 0x23, 0x00, 0x25, 0x00, 0x14, 0xe6,
  0x08, 0xc0, 0xa0, 0x0c, 0x63, 0x0c, 0x22, 0x73, 0x04, 0x08, 0x99, 0x7b,
  0x86, 0xcb, 0x9f, 0xb0, 0x87, 0x90, 0xfc, 0x10, 0x68, 0x86, 0x85, 0x40,
  0xc1, 0x29, 0xc8, 0x18, 0x68, 0x8c, 0x31, 0x06, 0xa5, 0x9b, 0xa4, 0x29,
  0xa2, 0x84, 0xc9, 0x97, 0x18, 0x60, 0x59, 0x52, 0x00, 0x5b, 0x1c, 0x60,
  0x40, 0xc2, 0x18, 0xb4, 0x8a, 0x01, 0xc6, 0x18, 0x83, 0x51, 0x9b, 0x23,
  0x08, 0x8a, 0x81, 0x86, 0x19, 0xe3, 0x11, 0x1c, 0x08, 0x38, 0x48, 0x9a,
  0x22, 0x4a, 0x98, 0xfc, 0xd2, 0x31, 0x4a, 0x00, 0x73, 0xa8, 0x09, 0x11,
  0x63, 0x8c, 0x31, 0xd3, 0x18, 0x8c, 0x03, 0x3b, 0x84, 0xc3, 0x3c, 0xcc,
  0x83, 0x1b, 0xc8, 0xc2, 0x2d, 0xcc, 0x02, 0x3d, 0xc8, 0x43, 0x3d, 0x8c,
  0x03, 0x3d, 0xd4, 0x83, 0x3c, 0x94, 0x03, 0x39, 0x88, 0x42, 0x3d, 0x98,
  0x83, 0x39, 0x94, 0x83, 0x3c, 0xf0, 0x01, 0x2b, 0xbc, 0xc3, 0x39, 0xb0,
  0x42, 0x38, 0xe4, 0xc3, 0x3b, 0xd4, 0x03, 0x3d, 0xf8, 0x01, 0x0a, 0x26,
  0xd1, 0x39, 0x02, 0x50, 0xa0, 0x01, 0x00, 0x00, 0x13, 0x14, 0x72, 0xc0,
  0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0,
  0x87, 0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d,
  0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d,
  0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78,
  0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a,
  0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73,
  0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60, 0x07, 0x74,
  0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d,
  0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6,
  0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78,
  0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76,
  0x40, 0x07, 0x43, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x86, 0x3c, 0x04, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x12, 0x20, 0x00, 0x04, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0xf2, 0x34, 0x40, 0x00, 0x08, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0xe4, 0x81, 0x80, 0x00, 0x18,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x0b, 0x04, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11, 0x4c, 0x90,
  0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x25, 0x50, 0x0a, 0x05,
  0x51, 0x0c, 0x23, 0x00, 0x85, 0x51, 0x20, 0x45, 0x51, 0x80, 0x02, 0xf4,
  0x46, 0x00, 0xc8, 0x8e, 0x00, 0x14, 0x08, 0xd5, 0x19, 0x00, 0x00, 0x00,
  0x79, 0x18, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90,
  0x46, 0x02, 0x13, 0x44, 0x8f, 0x0c, 0x6f, 0xec, 0xed, 0x4d, 0x0c, 0x24,
  0xc6, 0x05, 0xc7, 0x45, 0x86, 0x06, 0xa6, 0xc6, 0x25, 0xa6, 0x06, 0x04,
  0xc5, 0x8c, 0xec, 0xa6, 0xac, 0x86, 0x46, 0x6c, 0x8c, 0x2c, 0x65, 0x43,
  0x10, 0x4c, 0x10, 0x86, 0x62, 0x82, 0x30, 0x18, 0x1b, 0x84, 0x81, 0x98,
  0x20, 0x0c, 0xc7, 0x06, 0x61, 0x30, 0x28, 0x84, 0xcd, 0x4d, 0x10, 0x06,
  0x64, 0xc3, 0x80, 0x24, 0xc4, 0x04, 0x61, 0x48, 0x26, 0x08, 0x55, 0x44,
  0x60, 0x82, 0x30, 0x28, 0x13, 0x84, 0xc7, 0x99, 0x20, 0x0c, 0xcb, 0x06,
  0x61, 0x80, 0x36, 0x2c, 0x0b, 0xd3, 0x2c, 0xcb, 0xe0, 0x3c, 0xcf, 0x13,
  0x6d, 0x08, 0xa4, 0x0d, 0x04, 0x30, 0x01, 0xc0, 0x04, 0x41, 0x00, 0x68,
  0x04, 0x4d, 0x39, 0xbd, 0x41, 0x4d, 0x4d, 0x10, 0x2c, 0x68, 0x82, 0x30,
  0x30, 0x13, 0x84, 0xa1, 0xd9, 0x30, 0x0c, 0xd9, 0xb0, 0x41, 0xd0, 0x8c,
  0x0d, 0xc4, 0x72, 0x61, 0xdb, 0x86, 0xa2, 0xb2, 0x00, 0x8a, 0xe3, 0x90,
  0xe6, 0x46, 0xc7, 0xe7, 0xad, 0xcd, 0x2d, 0x0d, 0xee, 0x8d, 0xae, 0xcc,
  0x8d, 0x0e, 0x64, 0x0c, 0x2d, 0x4c, 0x8e, 0xd1, 0x54, 0x5a, 0x1b, 0x1c,
  0x5b, 0x19, 0xc8, 0xd0, 0xcb, 0xd0, 0xca, 0x0a, 0x08, 0x95, 0x50, 0x50,
  0xd0, 0x86, 0x00, 0x0c, 0x26, 0x08, 0xd6, 0xb3, 0x61, 0xf8, 0xc2, 0x40,
  0x0c, 0x36, 0x0c, 0xde, 0x18, 0x88, 0xc1, 0x86, 0x81, 0x0c, 0xc8, 0x40,
  0x0c, 0xaa, 0xb0, 0xb1, 0xd9, 0xb5, 0xb9, 0xa4, 0x91, 0x95, 0xb9, 0xd1,
  0x4d, 0x09, 0x82, 0x2a, 0x64, 0x78, 0x2e, 0x76, 0x65, 0x72, 0x73, 0x69,
  0x6f, 0x6e, 0x53, 0x02, 0xa2, 0x09, 0x19, 0x9e, 0x8b, 0x5d, 0x18, 0x9b,
  0x5d, 0x99, 0xdc, 0x94, 0xc0, 0xa8, 0x43, 0x86, 0xe7, 0x32, 0x87, 0x16,
  0x46, 0x56, 0x26, 0xd7, 0xf4, 0x46, 0x56, 0xc6, 0x36, 0x25, 0x48, 0xca,
  0x90, 0xe1, 0xb9, 0xc8, 0x95, 0xcd, 0xbd, 0xd5, 0xc9, 0x8d, 0x95, 0xcd,
  0x4d, 0x09, 0xa6, 0x3a, 0x64, 0x78, 0x2e, 0x65, 0x6e, 0x74, 0x72, 0x79,
  0x50, 0x6f, 0x69, 0x6e, 0x74, 0x73, 0x53, 0x02, 0x0e, 0x00, 0x00, 0x00,
  0x79, 0x18, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c,
  0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3,
  0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6,
  0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e,
  0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43,
  0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03,
  0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48,
  0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20,
  0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e,
  0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d,
  0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89,
  0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83,
  0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68,
  0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90,
  0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78,
  0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98,
  0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5,
  0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c,
  0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c,
  0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43,
  0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43,
  0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82,
  0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70,
  0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19, 0xd1, 0x43, 0x0e, 0xf8,
  0xe0, 0x06, 0xe4, 0x20, 0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2,
  0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x00, 0x00, 0x00,
  0x71, 0x20, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x06, 0x60, 0x68, 0xec,
  0xe9, 0xa4, 0xac, 0x09, 0x54, 0xc3, 0xe5, 0x3b, 0x8f, 0x0f, 0x4c, 0x0e,
  0x83, 0x08, 0x1b, 0xd2, 0xa0, 0x8f, 0x8f, 0xdc, 0xb6, 0x11, 0x48, 0xc3,
  0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04, 0x30, 0x11, 0x21, 0xd0, 0x0c, 0x0b,
  0x61, 0x03, 0x66, 0x30, 0x5c, 0xbe, 0xf3, 0xf8, 0x03, 0x22, 0x3d, 0xc0,
  0x24, 0x1c, 0x26, 0x21, 0x1d, 0xbe, 0x34, 0x45, 0x94, 0x30, 0xf9, 0x12,
  0x03, 0x2c, 0x4b, 0x0a, 0x60, 0x8b, 0x03, 0x0c, 0x16, 0x10, 0x0d, 0x97,
  0xef, 0x3c, 0xbe, 0x11, 0x39, 0xd4, 0x23, 0x0e, 0x3e, 0x72, 0xdb, 0x00,
  0x61, 0x20, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x13, 0x04, 0x41, 0x2c,
  0x10, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x34, 0x0a, 0x10, 0xa2,
  0xe4, 0x8a, 0x37, 0xa0, 0x08, 0x8a, 0x33, 0x60, 0x06, 0xa0, 0xb4, 0x05,
  0xc8, 0x94, 0x00, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0x00, 0x59,
  0x47, 0x10, 0x45, 0xcc, 0x88, 0x81, 0x01, 0x80, 0x20, 0x18, 0x10, 0xda,
  0x61, 0xcc, 0x44, 0x2c, 0x43, 0x31, 0x0a, 0x23, 0x06, 0x0b, 0x00, 0x82,
  0x60, 0x90, 0x6c, 0xc7, 0x40, 0x51, 0x94, 0x81, 0xed, 0x35, 0x08, 0x55,
  0x35, 0x6c, 0x40, 0x04, 0xc4, 0x00, 0xec, 0x35, 0x0c, 0x96, 0x36, 0x6c,
  0x40, 0x04, 0xce, 0x00, 0x8c, 0x18, 0x24, 0x04, 0x08, 0x82, 0x41, 0xd3,
  0x21, 0xdb, 0xe6, 0x10, 0x18, 0x10, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00,
  0x5b, 0x86, 0x22, 0x20, 0x83, 0x2d, 0xc3, 0x11, 0x90, 0x01, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
