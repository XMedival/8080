; Simple test - just count 3, 2, 1, 0
        ORG 0

        MVI A, '3'
        OUT 1
        MVI A, '2'
        OUT 1
        MVI A, '1'
        OUT 1
        MVI A, '0'
        OUT 1
        MVI A, 0Ah      ; newline
        OUT 1
        HLT
