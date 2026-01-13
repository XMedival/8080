       IDENTIFICATION DIVISION.
           PROGRAM-ID. COMPILER.

       ENVIRONMENT DIVISION.
       INPUT-OUTPUT SECTION.
       FILE-CONTROL.
           SELECT INPUT-FILE  ASSIGN TO WS-INPUT-NAME
                  ORGANIZATION IS LINE SEQUENTIAL
                  FILE STATUS IS INPUT-STATUS.
           SELECT OUTPUT-FILE ASSIGN TO WS-OUTPUT-NAME
                  ORGANIZATION IS LINE SEQUENTIAL
                  FILE STATUS IS OUTPUT-STATUS.

       DATA DIVISION.
       FILE SECTION.
       FD INPUT-FILE.
       01 INPUT-RECORD        PIC X(256).

       FD OUTPUT-FILE.
       01 OUTPUT-RECORD       PIC X(80).

       WORKING-STORAGE SECTION.
       01 WS-INPUT-NAME       PIC X(256).
       01 WS-OUTPUT-NAME      PIC X(256).
       01 INPUT-STATUS        PIC XX.
       01 OUTPUT-STATUS       PIC XX.
       01 EOF-FLAG            PIC 9 VALUE 0.

       01 HEX-CHARS           PIC X(16) VALUE "0123456789ABCDEF".

       01 CURRENT-ADDR        PIC 9(5) VALUE 0.
       01 LINE-NUM            PIC 9(5) VALUE 0.
       01 INSTR-LEN           PIC 9 VALUE 0.
       01 OUTPUT-POS          PIC 99 VALUE 1.
       01 BYTE-COUNT          PIC 99 VALUE 0.

       01 HEX-LINE.
           05 HEX-START       PIC X VALUE ":".
           05 HEX-LEN         PIC XX.
           05 HEX-ADDR        PIC XXXX.
           05 HEX-TYPE        PIC XX VALUE "00".
           05 HEX-DATA        PIC X(64).
           05 HEX-CHECKSUM    PIC XX.

       01 OPCODE-TABLE.
           05 FILLER PIC X(11) VALUE "NOP     00".
           05 FILLER PIC X(11) VALUE "HLT     76".
           05 FILLER PIC X(11) VALUE "RET     C9".
           05 FILLER PIC X(11) VALUE "PCHL    E9".
           05 FILLER PIC X(11) VALUE "SPHL    F9".
           05 FILLER PIC X(11) VALUE "XCHG    EB".
           05 FILLER PIC X(11) VALUE "XTHL    E3".
           05 FILLER PIC X(11) VALUE "EI      FB".
           05 FILLER PIC X(11) VALUE "DI      F3".
           05 FILLER PIC X(11) VALUE "RLC     07".
           05 FILLER PIC X(11) VALUE "RRC     0F".
           05 FILLER PIC X(11) VALUE "RAL     17".
           05 FILLER PIC X(11) VALUE "RAR     1F".
           05 FILLER PIC X(11) VALUE "DAA     27".
           05 FILLER PIC X(11) VALUE "CMA     2F".
           05 FILLER PIC X(11) VALUE "STC     37".
           05 FILLER PIC X(11) VALUE "CMC     3F".
           05 FILLER PIC X(11) VALUE "PUSH B  C5".
           05 FILLER PIC X(11) VALUE "PUSH D  D5".
           05 FILLER PIC X(11) VALUE "PUSH H  E5".
           05 FILLER PIC X(11) VALUE "PUSH PSWF5".
           05 FILLER PIC X(11) VALUE "POP B   C1".
           05 FILLER PIC X(11) VALUE "POP D   D1".
           05 FILLER PIC X(11) VALUE "POP H   E1".
           05 FILLER PIC X(11) VALUE "POP PSW F1".
           05 FILLER PIC X(11) VALUE "RNZ     C0".
           05 FILLER PIC X(11) VALUE "RZ      C8".
           05 FILLER PIC X(11) VALUE "RNC     D0".
           05 FILLER PIC X(11) VALUE "RC      D8".
           05 FILLER PIC X(11) VALUE "RPO     E0".
           05 FILLER PIC X(11) VALUE "RPE     E8".
           05 FILLER PIC X(11) VALUE "RP      F0".
           05 FILLER PIC X(11) VALUE "RM      F8".
           05 FILLER PIC X(11) VALUE "DAD B   09".
           05 FILLER PIC X(11) VALUE "DAD D   19".
           05 FILLER PIC X(11) VALUE "DAD H   29".
           05 FILLER PIC X(11) VALUE "DAD SP  39".
           05 FILLER PIC X(11) VALUE "INX B   03".
           05 FILLER PIC X(11) VALUE "INX D   13".
           05 FILLER PIC X(11) VALUE "INX H   23".
           05 FILLER PIC X(11) VALUE "INX SP  33".
           05 FILLER PIC X(11) VALUE "DCX B   0B".
           05 FILLER PIC X(11) VALUE "DCX D   1B".
           05 FILLER PIC X(11) VALUE "DCX H   2B".
           05 FILLER PIC X(11) VALUE "DCX SP  3B".
           05 FILLER PIC X(11) VALUE "LDAX B  0A".
           05 FILLER PIC X(11) VALUE "LDAX D  1A".
           05 FILLER PIC X(11) VALUE "STAX B  02".
           05 FILLER PIC X(11) VALUE "STAX D  12".
           05 FILLER PIC X(11) VALUE "JPO     E2".
           05 FILLER PIC X(11) VALUE "JPE     EA".
           05 FILLER PIC X(11) VALUE "JP      F2".
           05 FILLER PIC X(11) VALUE "JM      FA".
           05 FILLER PIC X(11) VALUE "CPO     E4".
           05 FILLER PIC X(11) VALUE "CPE     EC".
           05 FILLER PIC X(11) VALUE "CP      F4".
           05 FILLER PIC X(11) VALUE "CM      FC".
           05 FILLER PIC X(11) VALUE "LHLD    2A".
           05 FILLER PIC X(11) VALUE "SHLD    22".
           05 FILLER PIC X(11) VALUE "ACI     CE".
           05 FILLER PIC X(11) VALUE "SBI     DE".
           05 FILLER PIC X(11) VALUE "ZZEND      ".

       01 OPCODE-TBL REDEFINES OPCODE-TABLE.
           05 OPCODE-ENTRY OCCURS 60 TIMES.
               10 OP-MNEM      PIC X(8).
               10 OP-SPACE     PIC X.
               10 OP-HEX       PIC XX.

       01 REG-TABLE.
           05 FILLER PIC X(2) VALUE "B0".
           05 FILLER PIC X(2) VALUE "C1".
           05 FILLER PIC X(2) VALUE "D2".
           05 FILLER PIC X(2) VALUE "E3".
           05 FILLER PIC X(2) VALUE "H4".
           05 FILLER PIC X(2) VALUE "L5".
           05 FILLER PIC X(2) VALUE "M6".
           05 FILLER PIC X(2) VALUE "A7".

       01 REG-TBL REDEFINES REG-TABLE.
           05 REG-ENTRY OCCURS 8 TIMES.
               10 REG-NAME     PIC X.
               10 REG-NUM      PIC 9.

       01 WS-LINE             PIC X(256).
       01 WS-MNEMONIC         PIC X(8).
       01 WS-OPERAND1         PIC X(16).
       01 WS-OPERAND2         PIC X(16).
       01 WS-OPCODE           PIC 999 VALUE 0.
       01 WS-BYTE1            PIC 999 VALUE 0.
       01 WS-BYTE2            PIC 999 VALUE 0.
       01 WS-WORD             PIC 9(5) VALUE 0.
       01 WS-DST              PIC 9 VALUE 0.
       01 WS-SRC              PIC 9 VALUE 0.
       01 WS-RST-NUM          PIC 9 VALUE 0.
       01 I                   PIC 99.
       01 J                   PIC 99.
       01 K                   PIC 99.
       01 WS-POS              PIC 999.
       01 WS-CHAR             PIC X.
       01 WS-FOUND            PIC 9 VALUE 0.
       01 WS-HEX-BYTE         PIC XX.
       01 WS-HI-NIB           PIC 9.
       01 WS-LO-NIB           PIC 9.
       01 WS-TEMP             PIC 9(5).
       01 WS-CHECKSUM         PIC 9(5).

       PROCEDURE DIVISION.
       MAIN-PARA.
           ACCEPT WS-INPUT-NAME FROM ARGUMENT-VALUE
           ACCEPT WS-OUTPUT-NAME FROM ARGUMENT-VALUE

           IF WS-INPUT-NAME = SPACES
               DISPLAY "Usage: compiler input.asm output.hex"
               STOP RUN
           END-IF

           IF WS-OUTPUT-NAME = SPACES
               MOVE "out.hex" TO WS-OUTPUT-NAME
           END-IF

           OPEN INPUT INPUT-FILE
           IF INPUT-STATUS NOT = "00"
               DISPLAY "Error opening input file: " INPUT-STATUS
               STOP RUN
           END-IF

           OPEN OUTPUT OUTPUT-FILE
           IF OUTPUT-STATUS NOT = "00"
               DISPLAY "Error opening output file: " OUTPUT-STATUS
               STOP RUN
           END-IF

           INITIALIZE HEX-LINE
           MOVE 0 TO BYTE-COUNT
           MOVE 0 TO CURRENT-ADDR

           PERFORM UNTIL EOF-FLAG = 1
               READ INPUT-FILE INTO WS-LINE
                   AT END MOVE 1 TO EOF-FLAG
                   NOT AT END PERFORM PROCESS-LINE
               END-READ
           END-PERFORM

           IF BYTE-COUNT > 0
               PERFORM FLUSH-HEX-LINE
           END-IF

           PERFORM WRITE-EOF-RECORD

           CLOSE INPUT-FILE
           CLOSE OUTPUT-FILE

           DISPLAY "Assembly complete. Output: "
               FUNCTION TRIM(WS-OUTPUT-NAME)
           STOP RUN.

       PROCESS-LINE.
           ADD 1 TO LINE-NUM
           MOVE 0 TO INSTR-LEN
           MOVE 0 TO WS-OPCODE
           MOVE 0 TO WS-BYTE1
           MOVE 0 TO WS-BYTE2
           INITIALIZE WS-MNEMONIC
           INITIALIZE WS-OPERAND1
           INITIALIZE WS-OPERAND2

           PERFORM PARSE-LINE

           IF WS-MNEMONIC = SPACES
               CONTINUE
           ELSE
               PERFORM ENCODE-INSTRUCTION
               IF INSTR-LEN > 0
                   PERFORM OUTPUT-BYTES
               END-IF
           END-IF
           .

       PARSE-LINE.
           MOVE 1 TO WS-POS

           PERFORM SKIP-WHITESPACE
           IF WS-POS > 256
               CONTINUE
           ELSE
               IF WS-LINE(WS-POS:1) = ";" OR WS-LINE(WS-POS:1) = "*"
                   CONTINUE
               ELSE
                   PERFORM GET-MNEMONIC
                   PERFORM SKIP-WHITESPACE
                   IF WS-POS <= 256 AND WS-LINE(WS-POS:1) NOT = ";"
                       PERFORM GET-OPERAND1
                       PERFORM SKIP-WHITESPACE
                       IF WS-POS <= 256 AND WS-LINE(WS-POS:1) = ","
                           ADD 1 TO WS-POS
                           PERFORM SKIP-WHITESPACE
                           PERFORM GET-OPERAND2
                       END-IF
                   END-IF
               END-IF
           END-IF
           .

       SKIP-WHITESPACE.
           PERFORM UNTIL WS-POS > 256
               OR (WS-LINE(WS-POS:1) NOT = " "
                   AND WS-LINE(WS-POS:1) NOT = X"09")
               ADD 1 TO WS-POS
           END-PERFORM
           .

       GET-MNEMONIC.
           MOVE 1 TO J
           PERFORM UNTIL WS-POS > 256
               OR WS-LINE(WS-POS:1) = " "
               OR WS-LINE(WS-POS:1) = X"09"
               OR WS-LINE(WS-POS:1) = ";"
               OR J > 8
               MOVE FUNCTION UPPER-CASE(WS-LINE(WS-POS:1))
                   TO WS-MNEMONIC(J:1)
               ADD 1 TO WS-POS
               ADD 1 TO J
           END-PERFORM
           .

       GET-OPERAND1.
           MOVE 1 TO J
           PERFORM UNTIL WS-POS > 256
               OR WS-LINE(WS-POS:1) = " "
               OR WS-LINE(WS-POS:1) = X"09"
               OR WS-LINE(WS-POS:1) = ","
               OR WS-LINE(WS-POS:1) = ";"
               OR J > 16
               MOVE FUNCTION UPPER-CASE(WS-LINE(WS-POS:1))
                   TO WS-OPERAND1(J:1)
               ADD 1 TO WS-POS
               ADD 1 TO J
           END-PERFORM
           .

       GET-OPERAND2.
           MOVE 1 TO J
           PERFORM UNTIL WS-POS > 256
               OR WS-LINE(WS-POS:1) = " "
               OR WS-LINE(WS-POS:1) = X"09"
               OR WS-LINE(WS-POS:1) = ","
               OR WS-LINE(WS-POS:1) = ";"
               OR J > 16
               MOVE FUNCTION UPPER-CASE(WS-LINE(WS-POS:1))
                   TO WS-OPERAND2(J:1)
               ADD 1 TO WS-POS
               ADD 1 TO J
           END-PERFORM
           .

       ENCODE-INSTRUCTION.
           MOVE 0 TO WS-FOUND
           EVALUATE TRUE
               WHEN WS-MNEMONIC = "ORG"
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO CURRENT-ADDR
                   IF BYTE-COUNT > 0
                       PERFORM FLUSH-HEX-LINE
                   END-IF
                   MOVE 0 TO INSTR-LEN

               WHEN WS-MNEMONIC = "DB"
                   PERFORM ENCODE-DB

               WHEN WS-MNEMONIC = "MOV"
                   PERFORM GET-DST-REG
                   PERFORM GET-SRC-REG
                   COMPUTE WS-OPCODE = 64 + WS-DST * 8 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "MVI"
                   PERFORM GET-DST-REG
                   COMPUTE WS-OPCODE = 6 + WS-DST * 8
                   MOVE WS-OPERAND2 TO WS-OPERAND1
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ADD"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 128 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ADC"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 136 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "SUB"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 144 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "SBB"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 152 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ANA"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 160 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "XRA"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 168 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ORA"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 176 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "CMP"
                   PERFORM GET-SRC-REG-OP1
                   COMPUTE WS-OPCODE = 184 + WS-SRC
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "INR"
                   PERFORM GET-DST-REG
                   COMPUTE WS-OPCODE = 4 + WS-DST * 8
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "DCR"
                   PERFORM GET-DST-REG
                   COMPUTE WS-OPCODE = 5 + WS-DST * 8
                   MOVE 1 TO INSTR-LEN

               WHEN WS-MNEMONIC = "JMP"
                   MOVE 195 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "JNZ"
                   MOVE 194 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "JZ"
                   MOVE 202 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "JNC"
                   MOVE 210 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "JC"
                   MOVE 218 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "CALL"
                   MOVE 205 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "CNZ"
                   MOVE 196 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "CZ"
                   MOVE 204 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "LXI"
                   EVALUATE TRUE
                       WHEN WS-OPERAND1(1:1) = "B"
                           MOVE 1 TO WS-OPCODE
                       WHEN WS-OPERAND1(1:1) = "D"
                           MOVE 17 TO WS-OPCODE
                       WHEN WS-OPERAND1(1:1) = "H"
                           MOVE 33 TO WS-OPCODE
                       WHEN WS-OPERAND1(1:2) = "SP"
                           MOVE 49 TO WS-OPCODE
                   END-EVALUATE
                   MOVE WS-OPERAND2 TO WS-OPERAND1
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "LDA"
                   MOVE 58 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "STA"
                   MOVE 50 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   COMPUTE WS-BYTE1 = FUNCTION MOD(WS-WORD, 256)
                   DIVIDE WS-WORD BY 256 GIVING WS-BYTE2
                   MOVE 3 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ADI"
                   MOVE 198 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ACI"
                   MOVE 206 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "SUI"
                   MOVE 214 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "SBI"
                   MOVE 222 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ANI"
                   MOVE 230 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "ORI"
                   MOVE 246 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "XRI"
                   MOVE 238 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "CPI"
                   MOVE 254 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "IN"
                   MOVE 219 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "OUT"
                   MOVE 211 TO WS-OPCODE
                   PERFORM PARSE-NUMBER-OP1
                   MOVE WS-WORD TO WS-BYTE1
                   MOVE 2 TO INSTR-LEN

               WHEN WS-MNEMONIC = "RST"
                   MOVE WS-OPERAND1(1:1) TO WS-RST-NUM
                   COMPUTE WS-OPCODE = 199 + WS-RST-NUM * 8
                   MOVE 1 TO INSTR-LEN

               WHEN OTHER
                   PERFORM LOOKUP-SIMPLE-OPCODE
           END-EVALUATE
           .

       ENCODE-DB.
           MOVE 0 TO WS-WORD
           IF WS-OPERAND1(1:1) = "'"
               MOVE FUNCTION ORD(WS-OPERAND1(2:1)) TO WS-WORD
               SUBTRACT 1 FROM WS-WORD
           ELSE
               PERFORM PARSE-NUMBER-OP1
           END-IF
           MOVE WS-WORD TO WS-OPCODE
           MOVE 1 TO INSTR-LEN
           .

       GET-DST-REG.
           MOVE 0 TO WS-DST
           PERFORM VARYING I FROM 1 BY 1 UNTIL I > 8
               IF REG-NAME(I) = WS-OPERAND1(1:1)
                   MOVE REG-NUM(I) TO WS-DST
               END-IF
           END-PERFORM
           .

       GET-SRC-REG.
           MOVE 0 TO WS-SRC
           PERFORM VARYING I FROM 1 BY 1 UNTIL I > 8
               IF REG-NAME(I) = WS-OPERAND2(1:1)
                   MOVE REG-NUM(I) TO WS-SRC
               END-IF
           END-PERFORM
           .

       GET-SRC-REG-OP1.
           MOVE 0 TO WS-SRC
           PERFORM VARYING I FROM 1 BY 1 UNTIL I > 8
               IF REG-NAME(I) = WS-OPERAND1(1:1)
                   MOVE REG-NUM(I) TO WS-SRC
               END-IF
           END-PERFORM
           .

       LOOKUP-SIMPLE-OPCODE.
           MOVE 0 TO WS-FOUND
           STRING WS-MNEMONIC DELIMITED SPACES
                  " " DELIMITED SIZE
                  WS-OPERAND1 DELIMITED SPACES
               INTO WS-LINE
           END-STRING

           PERFORM VARYING I FROM 1 BY 1 UNTIL I > 60 OR WS-FOUND = 1
               IF OP-MNEM(I) = WS-MNEMONIC
                   OR FUNCTION TRIM(OP-MNEM(I)) =
                      FUNCTION TRIM(WS-LINE)
                   PERFORM CONVERT-HEX-TO-DEC
                   MOVE 1 TO INSTR-LEN
                   MOVE 1 TO WS-FOUND
               END-IF
           END-PERFORM

           IF WS-FOUND = 0
               DISPLAY "Unknown instruction: " WS-MNEMONIC
                   " at line " LINE-NUM
           END-IF
           .

       CONVERT-HEX-TO-DEC.
           MOVE 0 TO WS-OPCODE
           MOVE OP-HEX(I)(1:1) TO WS-CHAR
           PERFORM GET-HEX-VALUE
           MULTIPLY WS-HI-NIB BY 16 GIVING WS-TEMP
           MOVE OP-HEX(I)(2:1) TO WS-CHAR
           PERFORM GET-HEX-VALUE
           ADD WS-TEMP TO WS-HI-NIB GIVING WS-OPCODE
           .

       GET-HEX-VALUE.
           EVALUATE WS-CHAR
               WHEN "0" MOVE 0 TO WS-HI-NIB
               WHEN "1" MOVE 1 TO WS-HI-NIB
               WHEN "2" MOVE 2 TO WS-HI-NIB
               WHEN "3" MOVE 3 TO WS-HI-NIB
               WHEN "4" MOVE 4 TO WS-HI-NIB
               WHEN "5" MOVE 5 TO WS-HI-NIB
               WHEN "6" MOVE 6 TO WS-HI-NIB
               WHEN "7" MOVE 7 TO WS-HI-NIB
               WHEN "8" MOVE 8 TO WS-HI-NIB
               WHEN "9" MOVE 9 TO WS-HI-NIB
               WHEN "A" MOVE 10 TO WS-HI-NIB
               WHEN "B" MOVE 11 TO WS-HI-NIB
               WHEN "C" MOVE 12 TO WS-HI-NIB
               WHEN "D" MOVE 13 TO WS-HI-NIB
               WHEN "E" MOVE 14 TO WS-HI-NIB
               WHEN "F" MOVE 15 TO WS-HI-NIB
               WHEN OTHER MOVE 0 TO WS-HI-NIB
           END-EVALUATE
           .

       PARSE-NUMBER-OP1.
           MOVE 0 TO WS-WORD
           MOVE 1 TO J
           MOVE FUNCTION LENGTH(FUNCTION TRIM(WS-OPERAND1)) TO K

           IF K = 0
               CONTINUE
           ELSE IF WS-OPERAND1(K:1) = "H"
               SUBTRACT 1 FROM K
               PERFORM PARSE-HEX-NUMBER
           ELSE
               PERFORM PARSE-DEC-NUMBER
           END-IF
           END-IF
           .

       PARSE-HEX-NUMBER.
           PERFORM VARYING J FROM 1 BY 1 UNTIL J > K
               MOVE WS-OPERAND1(J:1) TO WS-CHAR
               PERFORM GET-HEX-VALUE
               COMPUTE WS-WORD = WS-WORD * 16 + WS-HI-NIB
           END-PERFORM
           .

       PARSE-DEC-NUMBER.
           PERFORM VARYING J FROM 1 BY 1 UNTIL J > K
               IF WS-OPERAND1(J:1) >= "0" AND WS-OPERAND1(J:1) <= "9"
                   COMPUTE WS-WORD = WS-WORD * 10 +
                       FUNCTION ORD(WS-OPERAND1(J:1)) - 49
               END-IF
           END-PERFORM
           .

       OUTPUT-BYTES.
           MOVE WS-OPCODE TO WS-TEMP
           PERFORM EMIT-BYTE

           IF INSTR-LEN >= 2
               MOVE WS-BYTE1 TO WS-TEMP
               PERFORM EMIT-BYTE
           END-IF

           IF INSTR-LEN >= 3
               MOVE WS-BYTE2 TO WS-TEMP
               PERFORM EMIT-BYTE
           END-IF
           .

       EMIT-BYTE.
           IF BYTE-COUNT >= 16
               PERFORM FLUSH-HEX-LINE
           END-IF

           COMPUTE WS-HI-NIB = WS-TEMP / 16
           COMPUTE WS-LO-NIB = FUNCTION MOD(WS-TEMP, 16)
           ADD 1 TO WS-HI-NIB
           ADD 1 TO WS-LO-NIB
           COMPUTE K = BYTE-COUNT * 2 + 1
           MOVE HEX-CHARS(WS-HI-NIB:1) TO HEX-DATA(K:1)
           ADD 1 TO K
           MOVE HEX-CHARS(WS-LO-NIB:1) TO HEX-DATA(K:1)

           ADD 1 TO BYTE-COUNT
           ADD 1 TO CURRENT-ADDR
           .

       FLUSH-HEX-LINE.
           IF BYTE-COUNT = 0
               CONTINUE
           ELSE
               COMPUTE WS-TEMP = CURRENT-ADDR - BYTE-COUNT
               COMPUTE WS-HI-NIB = WS-TEMP / 4096
               ADD 1 TO WS-HI-NIB
               MOVE HEX-CHARS(WS-HI-NIB:1) TO HEX-ADDR(1:1)
               COMPUTE WS-TEMP = FUNCTION MOD(WS-TEMP, 4096)
               COMPUTE WS-HI-NIB = WS-TEMP / 256
               ADD 1 TO WS-HI-NIB
               MOVE HEX-CHARS(WS-HI-NIB:1) TO HEX-ADDR(2:1)
               COMPUTE WS-TEMP = FUNCTION MOD(WS-TEMP, 256)
               COMPUTE WS-HI-NIB = WS-TEMP / 16
               ADD 1 TO WS-HI-NIB
               MOVE HEX-CHARS(WS-HI-NIB:1) TO HEX-ADDR(3:1)
               COMPUTE WS-LO-NIB = FUNCTION MOD(WS-TEMP, 16)
               ADD 1 TO WS-LO-NIB
               MOVE HEX-CHARS(WS-LO-NIB:1) TO HEX-ADDR(4:1)

               COMPUTE WS-HI-NIB = BYTE-COUNT / 16
               ADD 1 TO WS-HI-NIB
               MOVE HEX-CHARS(WS-HI-NIB:1) TO HEX-LEN(1:1)
               COMPUTE WS-LO-NIB = FUNCTION MOD(BYTE-COUNT, 16)
               ADD 1 TO WS-LO-NIB
               MOVE HEX-CHARS(WS-LO-NIB:1) TO HEX-LEN(2:1)

               MOVE "00" TO HEX-TYPE
               MOVE "00" TO HEX-CHECKSUM

               STRING ":" DELIMITED SIZE
                      HEX-LEN DELIMITED SIZE
                      HEX-ADDR DELIMITED SIZE
                      HEX-TYPE DELIMITED SIZE
                      HEX-DATA(1:BYTE-COUNT * 2) DELIMITED SIZE
                      HEX-CHECKSUM DELIMITED SIZE
                   INTO OUTPUT-RECORD
               END-STRING

               WRITE OUTPUT-RECORD

               INITIALIZE HEX-DATA
               MOVE 0 TO BYTE-COUNT
           END-IF
           .

       WRITE-EOF-RECORD.
           MOVE ":00000001FF" TO OUTPUT-RECORD
           WRITE OUTPUT-RECORD
           .
