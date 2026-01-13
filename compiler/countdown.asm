; Countdown from 9 to 0, output each digit
; Then print newline and halt

        ORG 0

START:  MVI B, 10       ; Counter (10 iterations: 9 down to 0)
        MVI A, '9'      ; Start with ASCII '9'

LOOP:   OUT 1           ; Output digit
        DCR A           ; Next digit (ASCII value decreases)
        DCR B           ; Decrement counter
        JNZ LOOP        ; Continue if not done

        MVI A, 0Ah      ; Newline
        OUT 1
        HLT
