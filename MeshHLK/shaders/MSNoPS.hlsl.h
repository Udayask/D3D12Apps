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
; shader hash: 26a1c895a6b3f041f37085b3c992a8ac
;
; Pipeline Runtime Information: 
;
; Mesh Shader
; MeshOutputTopology=triangle
; NumThreads=(1,1,128)
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
%"class.RWStructuredBuffer<LogLayout>" = type { %struct.LogLayout }
%struct.LogLayout = type { i32, i32, i32, i32 }

define void @MSNoPS() {
  %1 = call %dx.types.Handle @dx.op.createHandle(i32 57, i8 1, i32 0, i32 0, i1 false)  ; CreateHandle(resourceClass,rangeId,index,nonUniformIndex)
  %2 = call i32 @dx.op.atomicBinOp.i32(i32 78, %dx.types.Handle %1, i32 0, i32 0, i32 4, i32 undef, i32 1)  ; AtomicBinOp(handle,atomicOp,offset0,offset1,offset2,newValue)
  ret void
}

; Function Attrs: nounwind
declare i32 @dx.op.atomicBinOp.i32(i32, %dx.types.Handle, i32, i32, i32, i32, i32) #0

; Function Attrs: nounwind readonly
declare %dx.types.Handle @dx.op.createHandle(i32, i8, i32, i32, i1) #1

attributes #0 = { nounwind }
attributes #1 = { nounwind readonly }

!llvm.ident = !{!0}
!dx.version = !{!1}
!dx.valver = !{!2}
!dx.shaderModel = !{!3}
!dx.resources = !{!4}
!dx.entryPoints = !{!8}

!0 = !{!"dxcoob 1.8.2405.15 (fd7e54bcd)"}
!1 = !{i32 1, i32 5}
!2 = !{i32 1, i32 8}
!3 = !{!"ms", i32 6, i32 5}
!4 = !{null, !5, null, null}
!5 = !{!6}
!6 = !{i32 0, %"class.RWStructuredBuffer<LogLayout>"* undef, !"", i32 0, i32 0, i32 1, i32 12, i1 false, i1 false, i1 false, !7}
!7 = !{i32 1, i32 16}
!8 = !{void ()* @MSNoPS, !"MSNoPS", null, !4, !9}
!9 = !{i32 0, i64 16, i32 9, !10}
!10 = !{!11, i32 0, i32 0, i32 2, i32 8}
!11 = !{i32 1, i32 1, i32 128}

#endif

const unsigned char g_MSNoPS[] = {
  0x44, 0x58, 0x42, 0x43, 0xff, 0xa4, 0xdd, 0x95, 0x83, 0x14, 0x71, 0x37,
  0x83, 0xf8, 0xa8, 0x49, 0x69, 0x14, 0xa0, 0xe2, 0x01, 0x00, 0x00, 0x00,
  0x70, 0x0b, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00,
  0x4c, 0x00, 0x00, 0x00, 0x5c, 0x00, 0x00, 0x00, 0x6c, 0x00, 0x00, 0x00,
  0xdc, 0x00, 0x00, 0x00, 0x70, 0x06, 0x00, 0x00, 0x8c, 0x06, 0x00, 0x00,
  0x53, 0x46, 0x49, 0x30, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x49, 0x53, 0x47, 0x31, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x4f, 0x53, 0x47, 0x31,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x50, 0x53, 0x56, 0x30, 0x68, 0x00, 0x00, 0x00, 0x34, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
  0x0d, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x08, 0x00, 0x00, 0x00, 0x00, 0x4d, 0x53, 0x4e, 0x6f, 0x50, 0x53, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x53, 0x54, 0x41, 0x54, 0x8c, 0x05, 0x00, 0x00,
  0x65, 0x00, 0x0d, 0x00, 0x63, 0x01, 0x00, 0x00, 0x44, 0x58, 0x49, 0x4c,
  0x05, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x74, 0x05, 0x00, 0x00,
  0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00, 0x5a, 0x01, 0x00, 0x00,
  0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00,
  0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49, 0x06, 0x10, 0x32, 0x39,
  0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19, 0x1e, 0x04, 0x8b, 0x62,
  0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42, 0xa4, 0x10, 0x32, 0x14,
  0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88, 0x48, 0x90, 0x14, 0x20,
  0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42, 0xe4, 0x48, 0x0e, 0x90,
  0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c, 0xe1, 0x83, 0xe5, 0x8a,
  0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00,
  0x1b, 0x88, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07, 0x40, 0xda, 0x60, 0x08,
  0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x12, 0x50, 0x01, 0x00, 0x00, 0x00,
  0x49, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x82, 0x60, 0x42,
  0x20, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00, 0x27, 0x00, 0x00, 0x00,
  0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04, 0x93, 0x22, 0xa4, 0x84,
  0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14, 0x12, 0x4c, 0x8a, 0x8c,
  0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x44, 0x23, 0x00, 0x25, 0x00, 0x14, 0xe6,
  0x08, 0xc0, 0x60, 0x8e, 0x00, 0x21, 0x72, 0xcf, 0x70, 0xf9, 0x13, 0xf6,
  0x10, 0x92, 0x1f, 0x02, 0xcd, 0xb0, 0x10, 0x28, 0x28, 0x05, 0x19, 0xc3,
  0x8c, 0x31, 0xc6, 0xa0, 0x33, 0x47, 0x10, 0x14, 0xc3, 0x0c, 0x32, 0x46,
  0x22, 0x35, 0x10, 0x70, 0x90, 0x34, 0x45, 0x94, 0x30, 0xf9, 0xa5, 0x63,
  0x94, 0x00, 0xe6, 0x50, 0x13, 0x22, 0xc6, 0x18, 0x63, 0xa6, 0x31, 0x18,
  0x07, 0x76, 0x08, 0x87, 0x79, 0x98, 0x07, 0x37, 0x90, 0x85, 0x5b, 0x98,
  0x05, 0x7a, 0x90, 0x87, 0x7a, 0x18, 0x07, 0x7a, 0xa8, 0x07, 0x79, 0x28,
  0x07, 0x72, 0x10, 0x85, 0x7a, 0x30, 0x07, 0x73, 0x28, 0x07, 0x79, 0xe0,
  0x03, 0x56, 0x78, 0x87, 0x73, 0x60, 0x85, 0x70, 0xc8, 0x87, 0x77, 0xa8,
  0x07, 0x7a, 0xf0, 0x03, 0x14, 0x34, 0x72, 0x73, 0x04, 0xa0, 0x00, 0x00,
  0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87, 0x36, 0x68, 0x87, 0x79,
  0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50, 0x0e, 0x6d, 0xd0, 0x0e,
  0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07,
  0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0, 0x07, 0x73, 0x20, 0x07,
  0x6d, 0x90, 0x0e, 0x78, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e,
  0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe9, 0x30, 0x07,
  0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x76, 0x40, 0x07,
  0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10, 0x07, 0x76, 0xa0, 0x07,
  0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20, 0x07, 0x7a, 0x30, 0x07,
  0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0, 0x07, 0x76, 0x40, 0x07,
  0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07,
  0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e, 0x00, 0x08, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86, 0x3c, 0x07, 0x10, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x79, 0x14, 0x20,
  0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc8, 0x02, 0x01,
  0x0e, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14, 0x19, 0x11, 0x4c, 0x90,
  0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a, 0x25, 0x50, 0x0a, 0x23,
  0x00, 0xc5, 0x50, 0x18, 0x05, 0x52, 0x06, 0xe5, 0x50, 0x08, 0x05, 0x51,
  0x12, 0x05, 0x28, 0x50, 0x04, 0x94, 0x46, 0x00, 0x08, 0x16, 0x08, 0xb9,
  0x19, 0x00, 0x7a, 0x33, 0x00, 0xd4, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x79, 0x18, 0x00, 0x00, 0x75, 0x00, 0x00, 0x00, 0x1a, 0x03, 0x4c, 0x90,
  0x46, 0x02, 0x13, 0x44, 0x8f, 0x0c, 0x6f, 0xec, 0xed, 0x4d, 0x0c, 0x24,
  0xc6, 0x05, 0xc7, 0x45, 0x86, 0x06, 0xa6, 0xc6, 0x25, 0xa6, 0x06, 0x04,
  0xc5, 0x8c, 0xec, 0xa6, 0xac, 0x86, 0x46, 0x6c, 0x8c, 0x2c, 0x65, 0x43,
  0x10, 0x4c, 0x10, 0x86, 0x61, 0x82, 0x30, 0x10, 0x1b, 0x84, 0x81, 0x98,
  0x20, 0x0c, 0xc5, 0x06, 0xc1, 0x30, 0x28, 0xb4, 0xcd, 0x4d, 0x10, 0x06,
  0x63, 0xc3, 0x80, 0x24, 0xc4, 0x04, 0xe1, 0x99, 0x68, 0x30, 0xbd, 0x9d,
  0x55, 0x05, 0x59, 0x4d, 0x10, 0x86, 0x63, 0x82, 0x90, 0x40, 0x13, 0x84,
  0x01, 0xd9, 0x20, 0x0c, 0xcf, 0x86, 0xc5, 0x58, 0x18, 0xc3, 0x18, 0x1a,
  0xc7, 0x71, 0xa0, 0x0d, 0x41, 0xb4, 0x81, 0x00, 0x24, 0x00, 0x98, 0x20,
  0x38, 0x12, 0x03, 0xb4, 0x09, 0xc2, 0x90, 0x6c, 0x20, 0x92, 0xca, 0x32,
  0x26, 0x08, 0x0d, 0xb5, 0x41, 0x30, 0xb0, 0x0d, 0x41, 0xb6, 0x41, 0x30,
  0xb4, 0x0d, 0xc3, 0x73, 0x6d, 0x6c, 0x82, 0xa6, 0x92, 0xdc, 0xec, 0xde,
  0xc6, 0xc2, 0xe8, 0xd2, 0xde, 0xdc, 0xe6, 0x26, 0x08, 0x83, 0xb2, 0xc1,
  0x48, 0x3a, 0xcb, 0xf0, 0x08, 0x36, 0x4d, 0x53, 0x49, 0x6e, 0x76, 0x6f,
  0x63, 0x61, 0x74, 0x69, 0x6f, 0x6e, 0x73, 0x13, 0x84, 0x61, 0xd9, 0x60,
  0x24, 0x60, 0x60, 0x85, 0x81, 0x47, 0xb0, 0x81, 0x9a, 0x4a, 0x72, 0xb3,
  0x7b, 0x1b, 0x0b, 0xa3, 0x4b, 0x7b, 0x73, 0x9b, 0x9b, 0x20, 0x0c, 0xcc,
  0x06, 0x23, 0x19, 0x03, 0x8b, 0x0c, 0x3c, 0x82, 0xc6, 0xd0, 0xdb, 0xdb,
  0x5a, 0x5a, 0xd9, 0x06, 0x23, 0x31, 0x03, 0xab, 0xf1, 0x88, 0x0d, 0xc5,
  0xf3, 0x89, 0x41, 0x19, 0x9c, 0xc1, 0x86, 0xc2, 0xa0, 0x38, 0x0c, 0x0d,
  0x26, 0x08, 0x02, 0xb0, 0x01, 0xd8, 0x30, 0x18, 0x6b, 0xb0, 0x06, 0x1b,
  0x02, 0x36, 0xd8, 0x30, 0x0c, 0x6a, 0xd0, 0x06, 0x34, 0x9a, 0xa6, 0x9c,
  0xde, 0xa0, 0xa6, 0x26, 0x08, 0x50, 0x34, 0x41, 0x18, 0x9a, 0x09, 0xc2,
  0xe0, 0x6c, 0x18, 0x86, 0x41, 0x0e, 0x26, 0x08, 0xc3, 0xb3, 0xa1, 0x98,
  0x03, 0xc3, 0xa0, 0x03, 0x32, 0xd8, 0x40, 0x18, 0x70, 0x10, 0x07, 0x75,
  0xb0, 0xa1, 0x50, 0x83, 0x37, 0x00, 0x26, 0x3b, 0xa0, 0x11, 0x46, 0xf7,
  0xd6, 0x96, 0x36, 0xc6, 0x22, 0xcd, 0x6d, 0x8e, 0x6e, 0x6e, 0x03, 0x81,
  0x07, 0x43, 0x1e, 0x58, 0x55, 0xd8, 0xd8, 0xec, 0xda, 0x5c, 0xd2, 0xc8,
  0xca, 0xdc, 0xe8, 0xa6, 0x04, 0x41, 0x15, 0x32, 0x3c, 0x17, 0xbb, 0x32,
  0xb9, 0xb9, 0xb4, 0x37, 0xb7, 0x29, 0x01, 0xd1, 0x84, 0x0c, 0xcf, 0xc5,
  0x2e, 0x8c, 0xcd, 0xae, 0x4c, 0x6e, 0x4a, 0x60, 0xd4, 0x21, 0xc3, 0x73,
  0x99, 0x43, 0x0b, 0x23, 0x2b, 0x93, 0x6b, 0x7a, 0x23, 0x2b, 0x63, 0x9b,
  0x12, 0x24, 0x65, 0xc8, 0xf0, 0x5c, 0xe4, 0xca, 0xe6, 0xde, 0xea, 0xe4,
  0xc6, 0xca, 0xe6, 0xa6, 0x04, 0x52, 0x25, 0x32, 0x3c, 0x17, 0xba, 0x3c,
  0xb8, 0xb2, 0x20, 0x37, 0xb7, 0x37, 0xba, 0x30, 0xba, 0xb4, 0x37, 0xb7,
  0xb9, 0x29, 0x02, 0x1a, 0xb4, 0x41, 0x1d, 0x32, 0x3c, 0x97, 0x32, 0x37,
  0x3a, 0xb9, 0x3c, 0xa8, 0xb7, 0x34, 0x37, 0xba, 0xb9, 0x29, 0x81, 0x1d,
  0x74, 0x21, 0xc3, 0x73, 0x19, 0x7b, 0xab, 0x73, 0xa3, 0x2b, 0x93, 0x9b,
  0x9b, 0x12, 0xe4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00,
  0x4c, 0x00, 0x00, 0x00, 0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66,
  0x14, 0x01, 0x3d, 0x88, 0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07,
  0x79, 0x78, 0x07, 0x73, 0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10,
  0x0e, 0xf4, 0x80, 0x0e, 0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce,
  0xa1, 0x1c, 0x66, 0x30, 0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b,
  0xcc, 0x03, 0x3d, 0xc8, 0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c,
  0x74, 0x70, 0x07, 0x7b, 0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07,
  0x7a, 0x70, 0x03, 0x76, 0x78, 0x87, 0x70, 0x20, 0x87, 0x19, 0xcc, 0x11,
  0x0e, 0xec, 0x90, 0x0e, 0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0,
  0x0e, 0xf0, 0x50, 0x0e, 0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8,
  0x21, 0x1d, 0xc2, 0x61, 0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b,
  0xd0, 0x43, 0x39, 0xb4, 0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b,
  0xcc, 0xf0, 0x14, 0x76, 0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87,
  0x72, 0x68, 0x07, 0x37, 0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07,
  0x76, 0x28, 0x07, 0x76, 0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87,
  0x5f, 0x08, 0x87, 0x71, 0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81,
  0x2c, 0xee, 0xf0, 0x0e, 0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30,
  0x03, 0x62, 0xc8, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4,
  0xa1, 0x1c, 0xdc, 0x61, 0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca,
  0x61, 0x06, 0xd6, 0x90, 0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39,
  0xc8, 0x43, 0x39, 0xb8, 0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b,
  0x94, 0xc3, 0x2f, 0xbc, 0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b,
  0xb0, 0xc3, 0x0c, 0xc4, 0x21, 0x07, 0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87,
  0x76, 0x80, 0x87, 0x19, 0xd1, 0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20,
  0x0e, 0xe7, 0xe0, 0x06, 0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1, 0x90,
  0x0f, 0xef, 0x50, 0x0f, 0xf4, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00,
  0x0b, 0x00, 0x00, 0x00, 0x06, 0x60, 0x98, 0xec, 0xe9, 0xa4, 0xac, 0x05,
  0x54, 0xc3, 0xe5, 0x3b, 0x8f, 0x0f, 0x4c, 0x0e, 0x83, 0x08, 0x1b, 0xd2,
  0xa0, 0x8f, 0x8f, 0xdc, 0xb6, 0x09, 0x48, 0xc3, 0xe5, 0x3b, 0x8f, 0x2f,
  0x44, 0x04, 0x30, 0x11, 0x21, 0xd0, 0x0c, 0x0b, 0x01, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x48, 0x41, 0x53, 0x48, 0x14, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x26, 0xa1, 0xc8, 0x95, 0xa6, 0xb3, 0xf0, 0x41,
  0xf3, 0x70, 0x85, 0xb3, 0xc9, 0x92, 0xa8, 0xac, 0x44, 0x58, 0x49, 0x4c,
  0xdc, 0x04, 0x00, 0x00, 0x65, 0x00, 0x0d, 0x00, 0x37, 0x01, 0x00, 0x00,
  0x44, 0x58, 0x49, 0x4c, 0x05, 0x01, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0xc4, 0x04, 0x00, 0x00, 0x42, 0x43, 0xc0, 0xde, 0x21, 0x0c, 0x00, 0x00,
  0x2e, 0x01, 0x00, 0x00, 0x0b, 0x82, 0x20, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x13, 0x00, 0x00, 0x00, 0x07, 0x81, 0x23, 0x91, 0x41, 0xc8, 0x04, 0x49,
  0x06, 0x10, 0x32, 0x39, 0x92, 0x01, 0x84, 0x0c, 0x25, 0x05, 0x08, 0x19,
  0x1e, 0x04, 0x8b, 0x62, 0x80, 0x14, 0x45, 0x02, 0x42, 0x92, 0x0b, 0x42,
  0xa4, 0x10, 0x32, 0x14, 0x38, 0x08, 0x18, 0x4b, 0x0a, 0x32, 0x52, 0x88,
  0x48, 0x90, 0x14, 0x20, 0x43, 0x46, 0x88, 0xa5, 0x00, 0x19, 0x32, 0x42,
  0xe4, 0x48, 0x0e, 0x90, 0x91, 0x22, 0xc4, 0x50, 0x41, 0x51, 0x81, 0x8c,
  0xe1, 0x83, 0xe5, 0x8a, 0x04, 0x29, 0x46, 0x06, 0x51, 0x18, 0x00, 0x00,
  0x06, 0x00, 0x00, 0x00, 0x1b, 0x88, 0xe0, 0xff, 0xff, 0xff, 0xff, 0x07,
  0x40, 0xda, 0x60, 0x08, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x12, 0x50,
  0x01, 0x00, 0x00, 0x00, 0x49, 0x18, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
  0x13, 0x82, 0x60, 0x42, 0x20, 0x00, 0x00, 0x00, 0x89, 0x20, 0x00, 0x00,
  0x27, 0x00, 0x00, 0x00, 0x32, 0x22, 0x48, 0x09, 0x20, 0x64, 0x85, 0x04,
  0x93, 0x22, 0xa4, 0x84, 0x04, 0x93, 0x22, 0xe3, 0x84, 0xa1, 0x90, 0x14,
  0x12, 0x4c, 0x8a, 0x8c, 0x0b, 0x84, 0xa4, 0x4c, 0x10, 0x44, 0x23, 0x00,
  0x25, 0x00, 0x14, 0xe6, 0x08, 0xc0, 0x60, 0x8e, 0x00, 0x21, 0x72, 0xcf,
  0x70, 0xf9, 0x13, 0xf6, 0x10, 0x92, 0x1f, 0x02, 0xcd, 0xb0, 0x10, 0x28,
  0x28, 0x05, 0x19, 0xc3, 0x8c, 0x31, 0xc6, 0xa0, 0x33, 0x47, 0x10, 0x14,
  0xc3, 0x0c, 0x32, 0x46, 0x22, 0x35, 0x10, 0x70, 0x90, 0x34, 0x45, 0x94,
  0x30, 0xf9, 0xa5, 0x63, 0x94, 0x00, 0xe6, 0x50, 0x13, 0x22, 0xc6, 0x18,
  0x63, 0xa6, 0x31, 0x18, 0x07, 0x76, 0x08, 0x87, 0x79, 0x98, 0x07, 0x37,
  0x90, 0x85, 0x5b, 0x98, 0x05, 0x7a, 0x90, 0x87, 0x7a, 0x18, 0x07, 0x7a,
  0xa8, 0x07, 0x79, 0x28, 0x07, 0x72, 0x10, 0x85, 0x7a, 0x30, 0x07, 0x73,
  0x28, 0x07, 0x79, 0xe0, 0x03, 0x56, 0x78, 0x87, 0x73, 0x60, 0x85, 0x70,
  0xc8, 0x87, 0x77, 0xa8, 0x07, 0x7a, 0xf0, 0x03, 0x14, 0x34, 0x72, 0x73,
  0x04, 0xa0, 0x00, 0x00, 0x13, 0x14, 0x72, 0xc0, 0x87, 0x74, 0x60, 0x87,
  0x36, 0x68, 0x87, 0x79, 0x68, 0x03, 0x72, 0xc0, 0x87, 0x0d, 0xaf, 0x50,
  0x0e, 0x6d, 0xd0, 0x0e, 0x7a, 0x50, 0x0e, 0x6d, 0x00, 0x0f, 0x7a, 0x30,
  0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x71, 0xa0,
  0x07, 0x73, 0x20, 0x07, 0x6d, 0x90, 0x0e, 0x78, 0xa0, 0x07, 0x73, 0x20,
  0x07, 0x6d, 0x90, 0x0e, 0x71, 0x60, 0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0,
  0x06, 0xe9, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x90,
  0x0e, 0x76, 0x40, 0x07, 0x7a, 0x60, 0x07, 0x74, 0xd0, 0x06, 0xe6, 0x10,
  0x07, 0x76, 0xa0, 0x07, 0x73, 0x20, 0x07, 0x6d, 0x60, 0x0e, 0x73, 0x20,
  0x07, 0x7a, 0x30, 0x07, 0x72, 0xd0, 0x06, 0xe6, 0x60, 0x07, 0x74, 0xa0,
  0x07, 0x76, 0x40, 0x07, 0x6d, 0xe0, 0x0e, 0x78, 0xa0, 0x07, 0x71, 0x60,
  0x07, 0x7a, 0x30, 0x07, 0x72, 0xa0, 0x07, 0x76, 0x40, 0x07, 0x43, 0x9e,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x86,
  0x3c, 0x07, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x0c, 0x79, 0x14, 0x20, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0xc8, 0x02, 0x01, 0x0b, 0x00, 0x00, 0x00, 0x32, 0x1e, 0x98, 0x14,
  0x19, 0x11, 0x4c, 0x90, 0x8c, 0x09, 0x26, 0x47, 0xc6, 0x04, 0x43, 0x1a,
  0x25, 0x50, 0x0a, 0x05, 0x51, 0x0c, 0x23, 0x00, 0x85, 0x51, 0x20, 0x25,
  0x51, 0x80, 0x02, 0x45, 0x40, 0x69, 0x04, 0x80, 0x60, 0x81, 0xd0, 0x9b,
  0x01, 0x00, 0x00, 0x00, 0x79, 0x18, 0x00, 0x00, 0x3d, 0x00, 0x00, 0x00,
  0x1a, 0x03, 0x4c, 0x90, 0x46, 0x02, 0x13, 0x44, 0x8f, 0x0c, 0x6f, 0xec,
  0xed, 0x4d, 0x0c, 0x24, 0xc6, 0x05, 0xc7, 0x45, 0x86, 0x06, 0xa6, 0xc6,
  0x25, 0xa6, 0x06, 0x04, 0xc5, 0x8c, 0xec, 0xa6, 0xac, 0x86, 0x46, 0x6c,
  0x8c, 0x2c, 0x65, 0x43, 0x10, 0x4c, 0x10, 0x86, 0x61, 0x82, 0x30, 0x10,
  0x1b, 0x84, 0x81, 0x98, 0x20, 0x0c, 0xc5, 0x06, 0x61, 0x30, 0x28, 0xb4,
  0xcd, 0x4d, 0x10, 0x06, 0x63, 0xc3, 0x80, 0x24, 0xc4, 0x04, 0x61, 0x38,
  0x26, 0x08, 0xcf, 0x43, 0x60, 0x82, 0x30, 0x20, 0x13, 0x84, 0xa4, 0x99,
  0x20, 0x0c, 0xc9, 0x06, 0x61, 0x80, 0x36, 0x2c, 0x0b, 0xd3, 0x2c, 0xcb,
  0xe0, 0x3c, 0xcf, 0x13, 0x6d, 0x08, 0xa4, 0x0d, 0x04, 0x30, 0x01, 0xc0,
  0x04, 0x41, 0x00, 0x68, 0x34, 0x4d, 0x39, 0xbd, 0x41, 0x4d, 0x4d, 0x10,
  0x20, 0x67, 0x82, 0x30, 0x28, 0x13, 0x84, 0x61, 0xd9, 0x30, 0x0c, 0x43,
  0x36, 0x41, 0x18, 0x98, 0x0d, 0x85, 0xb6, 0x2c, 0x9b, 0xb1, 0x81, 0x58,
  0x2e, 0x8c, 0xdb, 0x50, 0x54, 0x16, 0x40, 0x75, 0x55, 0xd8, 0xd8, 0xec,
  0xda, 0x5c, 0xd2, 0xc8, 0xca, 0xdc, 0xe8, 0xa6, 0x04, 0x41, 0x15, 0x32,
  0x3c, 0x17, 0xbb, 0x32, 0xb9, 0xb9, 0xb4, 0x37, 0xb7, 0x29, 0x01, 0xd1,
  0x84, 0x0c, 0xcf, 0xc5, 0x2e, 0x8c, 0xcd, 0xae, 0x4c, 0x6e, 0x4a, 0x60,
  0xd4, 0x21, 0xc3, 0x73, 0x99, 0x43, 0x0b, 0x23, 0x2b, 0x93, 0x6b, 0x7a,
  0x23, 0x2b, 0x63, 0x9b, 0x12, 0x24, 0x65, 0xc8, 0xf0, 0x5c, 0xe4, 0xca,
  0xe6, 0xde, 0xea, 0xe4, 0xc6, 0xca, 0xe6, 0xa6, 0x04, 0x53, 0x1d, 0x32,
  0x3c, 0x97, 0x32, 0x37, 0x3a, 0xb9, 0x3c, 0xa8, 0xb7, 0x34, 0x37, 0xba,
  0xb9, 0x29, 0x41, 0x07, 0x79, 0x18, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
  0x33, 0x08, 0x80, 0x1c, 0xc4, 0xe1, 0x1c, 0x66, 0x14, 0x01, 0x3d, 0x88,
  0x43, 0x38, 0x84, 0xc3, 0x8c, 0x42, 0x80, 0x07, 0x79, 0x78, 0x07, 0x73,
  0x98, 0x71, 0x0c, 0xe6, 0x00, 0x0f, 0xed, 0x10, 0x0e, 0xf4, 0x80, 0x0e,
  0x33, 0x0c, 0x42, 0x1e, 0xc2, 0xc1, 0x1d, 0xce, 0xa1, 0x1c, 0x66, 0x30,
  0x05, 0x3d, 0x88, 0x43, 0x38, 0x84, 0x83, 0x1b, 0xcc, 0x03, 0x3d, 0xc8,
  0x43, 0x3d, 0x8c, 0x03, 0x3d, 0xcc, 0x78, 0x8c, 0x74, 0x70, 0x07, 0x7b,
  0x08, 0x07, 0x79, 0x48, 0x87, 0x70, 0x70, 0x07, 0x7a, 0x70, 0x03, 0x76,
  0x78, 0x87, 0x70, 0x20, 0x87, 0x19, 0xcc, 0x11, 0x0e, 0xec, 0x90, 0x0e,
  0xe1, 0x30, 0x0f, 0x6e, 0x30, 0x0f, 0xe3, 0xf0, 0x0e, 0xf0, 0x50, 0x0e,
  0x33, 0x10, 0xc4, 0x1d, 0xde, 0x21, 0x1c, 0xd8, 0x21, 0x1d, 0xc2, 0x61,
  0x1e, 0x66, 0x30, 0x89, 0x3b, 0xbc, 0x83, 0x3b, 0xd0, 0x43, 0x39, 0xb4,
  0x03, 0x3c, 0xbc, 0x83, 0x3c, 0x84, 0x03, 0x3b, 0xcc, 0xf0, 0x14, 0x76,
  0x60, 0x07, 0x7b, 0x68, 0x07, 0x37, 0x68, 0x87, 0x72, 0x68, 0x07, 0x37,
  0x80, 0x87, 0x70, 0x90, 0x87, 0x70, 0x60, 0x07, 0x76, 0x28, 0x07, 0x76,
  0xf8, 0x05, 0x76, 0x78, 0x87, 0x77, 0x80, 0x87, 0x5f, 0x08, 0x87, 0x71,
  0x18, 0x87, 0x72, 0x98, 0x87, 0x79, 0x98, 0x81, 0x2c, 0xee, 0xf0, 0x0e,
  0xee, 0xe0, 0x0e, 0xf5, 0xc0, 0x0e, 0xec, 0x30, 0x03, 0x62, 0xc8, 0xa1,
  0x1c, 0xe4, 0xa1, 0x1c, 0xcc, 0xa1, 0x1c, 0xe4, 0xa1, 0x1c, 0xdc, 0x61,
  0x1c, 0xca, 0x21, 0x1c, 0xc4, 0x81, 0x1d, 0xca, 0x61, 0x06, 0xd6, 0x90,
  0x43, 0x39, 0xc8, 0x43, 0x39, 0x98, 0x43, 0x39, 0xc8, 0x43, 0x39, 0xb8,
  0xc3, 0x38, 0x94, 0x43, 0x38, 0x88, 0x03, 0x3b, 0x94, 0xc3, 0x2f, 0xbc,
  0x83, 0x3c, 0xfc, 0x82, 0x3b, 0xd4, 0x03, 0x3b, 0xb0, 0xc3, 0x0c, 0xc4,
  0x21, 0x07, 0x7c, 0x70, 0x03, 0x7a, 0x28, 0x87, 0x76, 0x80, 0x87, 0x19,
  0xd1, 0x43, 0x0e, 0xf8, 0xe0, 0x06, 0xe4, 0x20, 0x0e, 0xe7, 0xe0, 0x06,
  0xf6, 0x10, 0x0e, 0xf2, 0xc0, 0x0e, 0xe1, 0x90, 0x0f, 0xef, 0x50, 0x0f,
  0xf4, 0x00, 0x00, 0x00, 0x71, 0x20, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00,
  0x06, 0x60, 0x98, 0xec, 0xe9, 0xa4, 0xac, 0x05, 0x54, 0xc3, 0xe5, 0x3b,
  0x8f, 0x0f, 0x4c, 0x0e, 0x83, 0x08, 0x1b, 0xd2, 0xa0, 0x8f, 0x8f, 0xdc,
  0xb6, 0x09, 0x48, 0xc3, 0xe5, 0x3b, 0x8f, 0x2f, 0x44, 0x04, 0x30, 0x11,
  0x21, 0xd0, 0x0c, 0x0b, 0x01, 0x00, 0x00, 0x00, 0x61, 0x20, 0x00, 0x00,
  0x0d, 0x00, 0x00, 0x00, 0x13, 0x04, 0x41, 0x2c, 0x10, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x34, 0x4a, 0xae, 0x38, 0x03, 0x0a, 0x61, 0x06,
  0x80, 0x48, 0x09, 0x00, 0x23, 0x06, 0x09, 0x00, 0x82, 0x60, 0xa0, 0x4c,
  0x45, 0xe0, 0x38, 0xc8, 0x88, 0xc1, 0x02, 0x80, 0x20, 0x18, 0x1c, 0x55,
  0x11, 0x3c, 0x0f, 0x31, 0x4c, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
