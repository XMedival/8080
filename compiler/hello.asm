; Hello World for Altair 8800 emulator
; Output "Hi\n" to serial port 1

        ORG 0

START:  MVI A, 'H'      ; Load 'H'
        OUT 1           ; Output to serial
        MVI A, 'i'      ; Load 'i'
        OUT 1           ; Output to serial
        MVI A, 0Ah      ; Load newline
        OUT 1           ; Output to serial
        HLT             ; Stop
