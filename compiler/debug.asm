        ORG 0
        MVI B, 10
        MVI A, '9'
LOOP:   OUT 1
        DCR A
        DCR B
        JNZ LOOP
        HLT
