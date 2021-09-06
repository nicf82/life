.globl	_PutSprite

_PutSprite::
	;Usage
	;de = sprite data destination location
  ;bc, x and y
  ;iyh width
  ;iyl height
	;The params com in backwards so Y,X,MEMDEST
	
	call GetScreenPos
RepeatYB:			;Very similar to the RepeatY above where we are reading, but now writing
	push hl
		ld b,0
		ld c,iyh	;Width is now in BC

		;Similar to above we need to load from de, write to hl, bc (width) times
		;but notice src and dst are reversed, so we need to 'ex' commands for ldir to work
		;as intended - its fast, it just re-labels de -> hl and hl -> de
		ex de,hl	
		ldir		;load from hl, write to de, bc (width) times
		ex de,hl	
	pop hl
	
	call GetNextLine	;update hl to be the next line
	dec iyl			;dec height
	jp nz,RepeatYB		;if height not now 0, repeat
ret

GetNextLine:

	ld a,h   
	add &08		;Add &0800 to hl by adding &08 to h
	ld h,a

	;This handles wrapping around the memory top to bottom
	bit 7,h		;If bit 7 is non-zero, we have wrapped from &ffXX and ended up at a &00XX location
	ret nz
	ld bc,&c050	;Not 100% sure why this isnt &c000, but &c050 is line 1 in the table below
	add hl,bc
ret

;Convert coordinates to screen memory position, result will be in hl
;XY coords in B,C - CPC in mode 1 is 320 wide, 4 pixels per byte so 80 bytes wide
GetScreenPos:
	push bc
		ld b,0 			;Set b ot 0 because we will look at y first
		ld hl,scr_addr_table 	;Load mem location offset for line 0 into hl
		add hl,bc		;C contains Y, the line offset so add it to the base twice
		add hl,bc		;(twice because each line location is 2-bytes)
					;hl now points to the correct screen address for the line we wanted to select
		ld a,(hl)		;Load first (most sig) byte from hl memory loc into a
		inc l	;We are incing hl here, but we know h (most sig byte) wont change because of the align 2
		ld h,(hl)		;Load second byte (least sig) into h
		ld l,a			;Put first (most sig) byte into l, result is hl contains (hl) but with bytes reversed
	pop bc
	ld c,b			;Load X co-ord, ie the column into c
	ld b,&c0		;set b to the base screen memory location
	add hl,bc		;add bc containing the base address and x co-ord to the line address already in hl
ret

;Eaxh word her is the offset of a line of the CPC screen, 8*25 = 200 lines
align 2
scr_addr_table:
    defb &00,&00, &00,&08, &00,&10, &00,&18, &00,&20, &00,&28, &00,&30, &00,&38;1
    defb &50,&00, &50,&08, &50,&10, &50,&18, &50,&20, &50,&28, &50,&30, &50,&38;2
    defb &A0,&00, &A0,&08, &A0,&10, &A0,&18, &A0,&20, &A0,&28, &A0,&30, &A0,&38;3
    defb &F0,&00, &F0,&08, &F0,&10, &F0,&18, &F0,&20, &F0,&28, &F0,&30, &F0,&38;4
    defb &40,&01, &40,&09, &40,&11, &40,&19, &40,&21, &40,&29, &40,&31, &40,&39;5
    defb &90,&01, &90,&09, &90,&11, &90,&19, &90,&21, &90,&29, &90,&31, &90,&39;6
    defb &E0,&01, &E0,&09, &E0,&11, &E0,&19, &E0,&21, &E0,&29, &E0,&31, &E0,&39;7
    defb &30,&02, &30,&0A, &30,&12, &30,&1A, &30,&22, &30,&2A, &30,&32, &30,&3A;8
    defb &80,&02, &80,&0A, &80,&12, &80,&1A, &80,&22, &80,&2A, &80,&32, &80,&3A;9
    defb &D0,&02, &D0,&0A, &D0,&12, &D0,&1A, &D0,&22, &D0,&2A, &D0,&32, &D0,&3A;10
    defb &20,&03, &20,&0B, &20,&13, &20,&1B, &20,&23, &20,&2B, &20,&33, &20,&3B;11
    defb &70,&03, &70,&0B, &70,&13, &70,&1B, &70,&23, &70,&2B, &70,&33, &70,&3B;12
    defb &C0,&03, &C0,&0B, &C0,&13, &C0,&1B, &C0,&23, &C0,&2B, &C0,&33, &C0,&3B;13
    defb &10,&04, &10,&0C, &10,&14, &10,&1C, &10,&24, &10,&2C, &10,&34, &10,&3C;14
    defb &60,&04, &60,&0C, &60,&14, &60,&1C, &60,&24, &60,&2C, &60,&34, &60,&3C;15
    defb &B0,&04, &B0,&0C, &B0,&14, &B0,&1C, &B0,&24, &B0,&2C, &B0,&34, &B0,&3C;16
    defb &00,&05, &00,&0D, &00,&15, &00,&1D, &00,&25, &00,&2D, &00,&35, &00,&3D;17
    defb &50,&05, &50,&0D, &50,&15, &50,&1D, &50,&25, &50,&2D, &50,&35, &50,&3D;18
    defb &A0,&05, &A0,&0D, &A0,&15, &A0,&1D, &A0,&25, &A0,&2D, &A0,&35, &A0,&3D;19
    defb &F0,&05, &F0,&0D, &F0,&15, &F0,&1D, &F0,&25, &F0,&2D, &F0,&35, &F0,&3D;20
    defb &40,&06, &40,&0E, &40,&16, &40,&1E, &40,&26, &40,&2E, &40,&36, &40,&3E;21
    defb &90,&06, &90,&0E, &90,&16, &90,&1E, &90,&26, &90,&2E, &90,&36, &90,&3E;22
    defb &E0,&06, &E0,&0E, &E0,&16, &E0,&1E, &E0,&26, &E0,&2E, &E0,&36, &E0,&3E;23
    defb &30,&07, &30,&0F, &30,&17, &30,&1F, &30,&27, &30,&2F, &30,&37, &30,&3F;24
    defb &80,&07, &80,&0F, &80,&17, &80,&1F, &80,&27, &80,&2F, &80,&37, &80,&3F;25