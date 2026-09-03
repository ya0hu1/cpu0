; Minimal Cpu0 backend example.
;
; Run:
;   build/bin/llc -march=cpu0 examples/cpu0/add.ll -o -

define i32 @add(i32 %a, i32 %b) {
entry:
  %sum = add i32 %a, %b
  ret i32 %sum
}
