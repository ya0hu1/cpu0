; RUN: llc -march=riscvtoy -mtriple=riscvtoy-unknown-unknown \
; RUN:   -verify-machineinstrs -filetype=asm -o - %s | FileCheck %s

define i32 @read_u8(i8* %p) {
entry:
  %v = load i8, i8* %p
  %r = zext i8 %v to i32
  ret i32 %r
}

define i32 @read_s8(i8* %p) {
entry:
  %v = load i8, i8* %p
  %r = sext i8 %v to i32
  ret i32 %r
}

define void @write_u8(i8* %p, i8 %v) {
entry:
  store i8 %v, i8* %p
  ret void
}

define i32 @read_u16(i16* %p) {
entry:
  %v = load i16, i16* %p
  %r = zext i16 %v to i32
  ret i32 %r
}

define i32 @read_s16(i16* %p) {
entry:
  %v = load i16, i16* %p
  %r = sext i16 %v to i32
  ret i32 %r
}

define void @write_u16(i16* %p, i16 %v) {
entry:
  store i16 %v, i16* %p
  ret void
}

; CHECK-LABEL: read_u8:
; CHECK: lbu a0, 0(a0)
; CHECK-NEXT: ret

; CHECK-LABEL: read_s8:
; CHECK: lb a0, 0(a0)
; CHECK-NEXT: ret

; CHECK-LABEL: write_u8:
; CHECK: sb a1, 0(a0)
; CHECK-NEXT: ret

; CHECK-LABEL: read_u16:
; CHECK: lhu a0, 0(a0)
; CHECK-NEXT: ret

; CHECK-LABEL: read_s16:
; CHECK: lh a0, 0(a0)
; CHECK-NEXT: ret

; CHECK-LABEL: write_u16:
; CHECK: sh a1, 0(a0)
; CHECK-NEXT: ret
