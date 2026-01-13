; Test program - count down from 5
        ORG 0

START:  MVI A, 5        ; Load 5 into A
LOOP:   DCR A           ; Decrement A
        JNZ LOOP        ; If not zero, keep looping
        OUT 1           ; Output final value
        HLT             ; Stop

; Data section
        ORG 100h
MSG:    DB 'H'
        DB 'i'
        DB 0
