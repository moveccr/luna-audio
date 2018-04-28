; MEMORY MAP
	; 0000 00FF        RESET/RST etc.
	; 0100 01FF        shared variables
	; 0200 3FFF        program
	; 4000 7FFF  16K   buffer page 0 
	; 8000 BFFF  16K   buffer page 1
	; C000 FDFF        unused
	; **** internal RAM 512 bytes
	; FE00 FF80  384   player main, interrupt handler
	; FF80 FFDF   96   stack (initialize SP=FFE0)
	; FFE0 FFFF   32   interrupt vector table
	


; I/O PORT
PSG_ADR	.EQU	83H		; PSG address
PSG_DAT	.EQU	82H		; data output

	.ORG	0000H
RESET:
	JP	ENTRY

	.ORG	0038H
INT0:
	RETI

	.ORG	0066H
NMI:
	RETN


; shared variable area
	.ORG	0100H
				; start command (NZ = start)
				; Host -> XP
CMD_START:	DB	0
				; timer value (by sampling frequency)
				; Host -> XP
TIMER:		DB	0
				; format code
				; Host -> XP
FORMAT:		DB	0

				; ready to start status (1 = ready)
				; XP -> Host
STAT_READY:	DB	0
				; error status (NZ = error)
				; XP -> Host
STAT_ERROR:	DB	0

				; unused, reserved
PAGEENDL:	DB	0

				; page end address H
				; notify current page
				; XP -> Host
				; page 0 = 80H
				; page 1 = 0C0H
PAGEENDH:	DB	0



; initializer program
	.ORG	0200H
ENTRY:
	DI
			; clear status
	XOR	A
	LD	(STAT_READY),A
	LD	(STAT_ERROR),A

			; init devices

			; internal I/O address = 00H - 3FH
	LD	A,00H
ICR	.EQU	3FH
	OUT0	(ICR),A

			; memory wait = 0
			; I/O wait = 3
			; no DMA
	LD	A,20H
DCNTL	.EQU	32H
	OUT0	(DCNTL),A

			; disable refresh
	LD	A,00H
RCR	.EQU	36H
	OUT0	(RCR),A

			; MMU
			; VA=PA, all common 1
	LD	A,00H
CBR	.EQU	38H
BBR	.EQU	39H
CBAR	.EQU	3AH
	OUT0	(CBR),A
	OUT0	(BBR),A
	OUT0	(CBAR),A

			; disable external interrupt
	LD	A,00H
ITC	.EQU	34H
	OUT0	(ITC),A

			; Interrupt Vector Low = E
			; I = FF
			; Interrupt Vector Address = FFE0
	LD	A,0EH
IL	.EQU	33H
	OUT0	(IL),A
	LD	A,0FFH
	LD	I,A
			; interrupt mode 1
	IM	1

			; PSG mixer
			; tone = off, noise = off
			; IOA, IOB = output
	LD	A,7
	OUT	(PSG_ADR),A
	LD	A,0C0H
	OUT	(PSG_DAT),A

			; reset CMD_START
	XOR	A
	LD	(CMD_START),A

			; now ready
	LD	A,1
	LD	(STAT_READY),A
	

			; wait for start
START_LOOP:
	LD	A,(CMD_START)
	OR	A
	JR	Z,START_LOOP

			; ok, now start

			; clear ready
	XOR	A
	LD	(STAT_READY),A

			; copy to internal ram
			; code make by format

			; copy vector table
	LD	HL,VECTOR
	LD	BC,VECTOR_END - VECTOR
	LD	DE,RUNTIME_VEC
	LDIR

			; copy interrupt entry code
	LD	HL,PRTINT
	LD	BC,PRTINT_END - PRTINT
	LD	DE,INTERNAL_RAM
	LDIR

			; format 1 to 5
	LD	A,(FORMAT)
	OR	A
	JP	Z,ERROR
	CP	6
	JP	C,ERROR

			; BC = (A - 1) * 8
	DEC	A
	ADD	A,A
	ADD	A,A
	ADD	A,A
	LD	C,A
	LD	B,0

			; SP = map entry
	LD	HL,MAP_FMT
	ADD	HL,BC
	LD	SP,HL

			; copy interrupt handler
	POP	HL
	POP	BC
			; also, set interrupt vector
	LD	(VEC_PRT0),HL
	LDIR
			; copy main routine
	POP	HL
	POP	BC
			; also, push main routine to stack
	LD	(INITIAL_SP - 2),HL
	LDIR

			; init psg address = 8
	LD	A,8
	OUT	(PSG_ADR),A

			; init page end address
	LD	A,80H
	LD	(PAGEENDH),A

			; init page address
	LD	HL,4000H

			; init regs
	XOR	A
	LD	E,A
	LD	D,A
	LD	B,A
	LD	C,PSG_DAT

			; init SP
	LD	SP,INITIAL_SP - 2

			; set timer
TMDR0L	.EQU	0CH
TMDR0H	.EQU	0DH
RLDR0L	.EQU	0EH
RLDR0H	.EQU	0FH
TCR	.EQU	10H
	LD	A,(TIMER)
	OUT0	(RLDR0L),A
	OUT0	(TMDR0L),A
	LD	A,00H
	OUT0	(RLDR0H),A
	OUT0	(TMDR0H),A
	LD	A,11H
	OUT0	(TCR),A

	XOR	A
			; go
	EI
			; jump to pushed main address
	RET

			; set error status and halt!
ERROR:
	LD	A,1
	LD	(STAT_ERROR),A
	HALT

; format map
MAP_FMT:
	DW	PCM1INT
	DW	PCM1INT_END - PCM1INT
	DW	PCM1
	DW	PCM1_END - PCM1

	DW	PCM2INT
	DW	PCM2INT_END - PCM2INT
	DW	PCM2
	DW	PCM2_END - PCM2

	DW	PCM3INT
	DW	PCM3INT_END - PCM3INT
	DW	PCM3
	DW	PCM3_END - PCM3

	DW	PAM2INT
	DW	PAM2INT_END - PAM2INT
	DW	PAM2
	DW	PAM2_END - PAM2

	DW	PAM3INT
	DW	PAM3INT_END - PAM3INT
	DW	PAM3
	DW	PAM3_END - PAM3
	

	; 割り込みエントリ共通条件
	; リロケータブルコードであること。
	; HL   データアドレス
	; C    PSG_DAT
	; PSG のアドレスレジスタは 8 を指している

PRTINT:
	; HL のラウンディングとホストとのやりとりは
	; PRTINT が共通で行う。
	; フォーマットに応じてローダーが各フォーマットのINTコードを
	; PRTINT の直後に配置する。

			; interrupt acknowledge

			; reset PRT0 Interrupt
	IN0	F,(TCR)
	IN0	F,(TMDR0L)

	; 再生停止はホストから割り込みかリセットで。

	LD	A,(PAGEENDH)
	CP	H
	JR	NZ,PRTINT_SKIP
			; 80H <-> 0C0H
	XOR	40H		
	LD	(PAGEENDH),A
			; send interrupt to host
			; level 5 interrupt
HOSTINTR	.EQU	0A0H
	OUT	(HOSTINTR),A
PRTINT_SKIP:
	
PRTINT_END:
	
PCM1:
PCM2:
PCM3:
	HALT
PCM1_END:
PCM2_END:
PCM3_END:

PCM1INT:
	; PSG のアドレスレジスタは 8 を指していること
	OUTI			; 2 12+3
	EI			; 1 3
	RETI			; 2 22

PCM1INT_END:
		
PCM2INT:
	LD	A,8		; 2 6
	OUT	(PSG_ADR),A	; 2 10+3
	OUTI			; 2 12+3
	INC	A		; 1 4
	OUT	(PSG_ADR),A	; 2 10+3
	OUTI			; 2 12+3

	EI			; 1 3
	RETI			; 1 22
PCM2INT_END:

PCM3INT:
			; skip padding
	INC	HL		; 1 4
	LD	A,8		; 2 6
	OUT	(PSG_ADR),A	; 2 10+3
	OUTI			; 2 12+3
	INC	A		; 1 4
	OUT	(PSG_ADR),A	; 2 10+3
	OUTI			; 2 12+3
	INC	A		; 1 4
	OUT	(PSG_ADR),A	; 2 10+3
	OUTI			; 2 12+3

	EI			; 1 3
	RETI			; 1 22
PCM3INT_END:

PAM2:
	; support freq = 3938.5Hz .. 
	; 4kHz := 6.144M / 4k = 1536
	; 1536/(13*2)=59.07 , roundup 60
	.REPT	60
	OUT	(C),A		; 2 10+3
	OUT	(C),E		; 2 10+3
	.ENDM			; 2*2*60=240 13*2*60=1560
	HALT			; 1
PAM2_END:
	
PAM2INT:
			; PHASE 0
	LD	A,(HL)		; 1 6
	INC	HL		; 1 4
			; PHASE 1
	LD	E,(HL)		; 1 6
	INC	HL		; 1 4

	EI			; 1 3
	RETI			; 2 22
PAM2INT_END:


PAM3:
	; support freq = 3938.5Hz .. 
	; 4kHz := 6.144M / 4k = 1536
	; 1536/(13*3)=39.3 , roundup 40
	.REPT	40
	OUT	(C),A		; 2 10+3
	OUT	(C),E		; 2 10+3
	OUT	(C),D		; 2 10+3
	.ENDM			; 2*3*40=240 13*3*40=1560
	HALT			; 1
PAM3_END:

PAM3INT:
	; SP を使ってPOP は検討したが、それよりこちらのほうが高速。

			; skip padding
	INC	HL		; 1 4
			; PHASE 0
	LD	A,(HL)		; 1 6
	INC	HL		; 1 4
			; PHASE 1
	LD	E,(HL)		; 1 6
	INC	HL		; 1 4
			; PHASE 2
	LD	D,(HL)		; 1 6
	INC	HL		; 1 4
				; 1*7=7 34
	
	EI			; 1 3
	RETI			; 2 22

PAM3INT_END:

; vector table
; copy to FFE0

VECTOR:
			; VEC_INT1
	DW	INT_IGN
			; VEC_INT2
	DW	INT_IGN
			; VEC_PRT0
	DW	INT_IGN
			; VEC_PRT1
	DW	INT_IGN
			; VEC_DMAC0
	DW	INT_IGN
			; VEC_DMAC1
	DW	INT_IGN
			; VEC_SIO
	DW	INT_IGN
			; VEC_ASCI0
	DW	INT_IGN
			; VEC_ASCI1
	DW	INT_IGN
			; INT_IGN
	RETI

VECTOR_END:


			; internal RAM
	.ORG	0FE00H
INTERNAL_RAM:

	.ORG	0FFE0H
INITIAL_SP:
RUNTIME_VEC:
VEC_INT1:
	.ORG	0FFE0H + 2
VEC_INT2:
	.ORG	0FFE0H + 4
VEC_PRT0:
	.ORG	0FFE0H + 6
VEC_PRT1:
	.ORG	0FFE0H + 8
VEC_DMAC0:
	.ORG	0FFE0H + 10
VEC_DMAC1:
	.ORG	0FFE0H + 12
VEC_SIO:
	.ORG	0FFE0H + 14
VEC_ASCI0:
	.ORG	0FFE0H + 16
VEC_ASCI1:
			; 本当はここはベクタテーブルだが
			; 使われることはないので押し込む。
	.ORG	0FFE0H + 18
INT_IGN:

