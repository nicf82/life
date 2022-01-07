_hl_sub_bc:
  scf
  ccf
  sbc hl, bc
  ret

_hl_add_bc:
  scf
  ccf
  adc hl, bc
  ret


;
; SDCC - Interfacing with Z80 assembler code
; ------------------------------------------
; > https://gist.github.com/Konamiman/af5645b9998c802753023cf1be8a2970
;


; unsigned char asmevolve(unsigned char *board)

_board_start:
  .dw #0x0000

_xy:
_y:
  .db #0x00
_x:
  .db #0x00

.globl	_asmevolve

_asmevolve::

  ld	hl,#2
  add	hl,sp
  ld	a,(hl)
  ld	e,a
  inc	hl
  ld	a,(hl)
  ld	d,a  
  ex  de, hl ;hl now contains *board

  ;Store the start of the board memory loc
  ld (_board_start), hl
  ld ix, (_board_start)


  ld e, #0 ;nbr counts - stores count for each col in 2-bit seminibbles - i.e. 0x00LLCCRR
  ld d, #0b00000001 ;bitmask - when its 128, this byte is complete

;Reads first column count without performing a life check as its just the margin column
_asme_fstbit:
  ;Check upper
  ld a, (ix)
  and d
  jr z, _skip_fu
  inc e
  _skip_fu:

  ;Check middle
  ld a, 10(ix)
  and d
  jr z, _skip_fc
  inc e
  _skip_fc:

  ;Check lower
  ld a, 20(ix)
  and d
  jr z, _skip_fl
  inc e
  _skip_fl:

_asme_sndbit:

  sla d  ;Shift bit left - #0b00000010
  
  ;Move col-count left
  sla e  ;;4CA7:   .#      CB 23
  sla e

  ;Check upper
  ld a, (ix)
  and d
  jr z, _skip_su
  inc e
  _skip_su:

  ;Check middle
  ld a, 10(ix)
  and d
  jr z, _skip_sc
  inc e
  _skip_sc:

  ;Check lower
  ld a, 20(ix)
  and d
  jr z, _skip_sl
  inc e
  _skip_sl:


_asme_nextbit:

  ; http://jgmalcolm.com/z80/advanced/shif
  ; rl arg1  - Rotates arg1 register to the left with the carry's value put into bit 0 and bit 7 is put into the carry.
  ; rla      - Rotates a register to the left with the carry's value put into bit 0 and bit 7 is put into the carry. One byte shorter than rl a.
  ; rlc arg1 - Rotates arg1 to the left with bit 7 being moved to bit 0 and also stored into the carry.
  ; rlca     - Rotates a to the left with bit 7 being moved to bit 0 and also stored into the carry. It's one byte shorter than rlc a.
  ; rr arg1  - Rotates arg1 to the right with the carry put in bit 7 and bit 0 put into the carry.
  ; rra      - Rotates a to the right with the carry put into bit 7 and bit 0 put into the carry flag. It's one byte shorter than rr a.
  ; rrc arg1 - Rotates arg1 to the right with bit 0 moved to bit 7 and also stored into the carry.
  ; rrca     - Rotates arg1 to the right with bit 0 moved to bit 7 and also stored into the carry. This is one byte shorter than rrc a.
  ; sla arg1 - Shifts arg1 register to the left with bit 7 moved to the carry flag and bit 0 reset (zeroed).
  ; sra arg1 - Shifts arg1 register to the right with bit 0 moved to the carry flag and bit 7 retaining it's original value.
  ; srl arg1 - Shifts arg1 register to the right with bit 0 moved to the carry flag and bit 7 zeroed.  

  rlc d  ;Rotate bit left - if this wraps, then we need to inc ix - #0b00000100
;  jr nc, _asme_after_colwrap_check_1
;   inc ix
;_asme_after_colwrap_check_1:

  ;Put count of previous 2 col's nbrs into l
  ld a, e
  and #3
  ld l, a
  rrc e ;Rotate right get the next seminibble
  rrc e
  ld a, e
  and #3
  add l
  ld l, a
  rlc e ;Rotale left again
  rlc e

  ;Move col nbr counts left
  sla e
  sla e

  ;Check upper
  ld a, (ix)
  and d
  jr z, _skip_u
  inc e
  _skip_u:

  ;Check middle
  ld a, 10(ix)
  and d
  jr z, _skip_c
  inc e
  _skip_c:

  ;Check lower
  ld a, 20(ix)
  and d
  jr z, _skip_l
  inc e
  _skip_l:

  ;Add new neighbour count into l
  ld a, e
  and #3
  add l
  ld l, a


  ;Put butmask back to middle bit to check life, dec ix if it wraps - #0b00000010
  rrc d
;   jr nc, _asme_after_colwrap_check_2
;   dec ix
; _asme_after_colwrap_check_2:

;life_check - HERE is where the survive/birth/death will go for this bit, e contains the nbr count
  ld a, 10(ix)
  and d
  jr z, _asme_cell_dead

  ;;Cell is alive - survives if nbrs is 2 or 3

  ;Check if survivor
  dec l   ;Dont include this live cell in nbrs cnt
  ld a, l
  and #0b11111110
  cp #2
  jr z, _asme_after_life_check ;Z is set if neighbours was 2 or 3 - cell survives
  
  ;Kill the cell
  ld a, d
  xor #0xFF ;Invert the current bit to kill it
  ld h, a ; eg 11111101
  ld a, 10(ix) ; eg 00001010
  and h ; eg 00001000 - killed 2nd bit
  ld 10(ix), a

  push hl
  call clrblk
  pop hl

  jr _asme_after_life_check

_asme_cell_dead:: ;Is a birth if neighbours is 3

  ;Check if birth
  ld a, l
  cp #3
  jr nz, _asme_after_life_check ;Z will be clear if neighbours was not 3 - not a birth

  ;Bring the cell to life
  ld a, 10(ix)
  or d
  ld 10(ix), a

  push hl
  call putblk
  pop hl

_asme_after_life_check::

  ;Put bitmask back to righthand column bit, inc ix if it wraps. Context: when we loop round to _asme_nextbit 
  ;we will first count the following bit's nbrs, then come back to this bit to check life - #0b00000100
  rlc d
;   jr nc, _asme_after_colwrap_check_3
;   inc ix
; _asme_after_colwrap_check_3:
  
  push hl
  call putmarker
  pop hl

  ld a, (_x) ;Inc the x coordinate, we will need to handle y after the last column
  inc a
  ld (_x), a

  cp a, #9 ;Currently checking if absolute x coordinate is 7, as we only do 1 byte the first of which is the margin
  jp nz, _asme_nextbit
  
  ret




;
; Stuff from vidmem.s - couldnt access it otherwise without some messing about
;

putmarker:
  
  ;Temporary - used to add a small marker to a cell

  push bc
    
  ld bc, (_xy)
  sla c
  sla c
  call get_screen_pos

	call get_next_line
	call get_next_line
	call get_next_line
  ld (hl), #0b00110011

  pop bc

  ret

putblk:
  
  ;Temporary - we should set/clear bytes at a time

  push bc
    
  ld bc, (_xy)
  sla c
  sla c
  call get_screen_pos

  ld (hl), #0x0E ;E leaves a border F would be a full block
	call get_next_line
  ld (hl), #0x0E
	call get_next_line
  ld (hl), #0x0E

  pop bc

  ret

clrblk:

  ;Temporary - we should set/clear bytes at a time

  push bc
  
  ld bc, (_xy)
  sla c
  sla c
  call get_screen_pos
  
  ld (hl), #0xEE
	call get_next_line
  ld (hl), #0xEE
	call get_next_line
  ld (hl), #0xEE

  pop bc

  ret


;Convert coordinates to screen memory position, result will be in hl
;XY coords in B,C - CPC in mode 1 is 320 wide, 4 pixels per byte so 80 bytes wide
get_screen_pos:
  push bc
    ld b,#0 			          ;Set b ot 0 because we will look at y first
    ld hl, #scr_addresses 	;Load mem location offset for line 0 into hl
    add hl,bc		            ;C contains Y, the line offset so add it to the base twice
    add hl,bc		            ;(twice because each line location is 2-bytes)
                            ;hl now points to the correct screen address for the line we wanted to select
    ld a,(hl)		            ;Load first (most sig) byte from hl memory loc into a
    inc hl	                  ;We are incing hl here
    ld h,(hl)               ;Load second byte (least sig) into h
    ld l,a	             		;Put first (most sig) byte into l, result is hl contains (hl) but with bytes reversed
  pop bc
  ld c,b                  ;Load X co-ord, ie the column into c
  ld b,#0xc0              ;set b to the base screen memory location
  add hl,bc               ;add bc containing the base address and x co-ord to the line address already in hl
ret

get_next_line:

	ld a,h   
	add #0x08		;Add 0x0800 to hl by adding 0x08 to h
	ld h,a

	;This handles wrapping around the memory top to bottom
	bit 7,h		;If bit 7 is non-zero, we have wrapped from 0xffXX and ended up at a 0x00XX location
	ret nz
	ld bc,#0xc050	;Not 100% sure why this isnt 0xc000, but 0xc050 is line 1 in the table below
	add hl,bc
ret

;Each word here is the offset of a line of the CPC screen, 8*25 = 200 lines

scr_addresses:
  .db 0x00,0x00, 0x00,0x08, 0x00,0x10, 0x00,0x18, 0x00,0x20, 0x00,0x28, 0x00,0x30, 0x00,0x38;1
  .db 0x50,0x00, 0x50,0x08, 0x50,0x10, 0x50,0x18, 0x50,0x20, 0x50,0x28, 0x50,0x30, 0x50,0x38;2
  .db 0xA0,0x00, 0xA0,0x08, 0xA0,0x10, 0xA0,0x18, 0xA0,0x20, 0xA0,0x28, 0xA0,0x30, 0xA0,0x38;3
  .db 0xF0,0x00, 0xF0,0x08, 0xF0,0x10, 0xF0,0x18, 0xF0,0x20, 0xF0,0x28, 0xF0,0x30, 0xF0,0x38;4
  .db 0x40,0x01, 0x40,0x09, 0x40,0x11, 0x40,0x19, 0x40,0x21, 0x40,0x29, 0x40,0x31, 0x40,0x39;5
  .db 0x90,0x01, 0x90,0x09, 0x90,0x11, 0x90,0x19, 0x90,0x21, 0x90,0x29, 0x90,0x31, 0x90,0x39;6
  .db 0xE0,0x01, 0xE0,0x09, 0xE0,0x11, 0xE0,0x19, 0xE0,0x21, 0xE0,0x29, 0xE0,0x31, 0xE0,0x39;7
  .db 0x30,0x02, 0x30,0x0A, 0x30,0x12, 0x30,0x1A, 0x30,0x22, 0x30,0x2A, 0x30,0x32, 0x30,0x3A;8
  .db 0x80,0x02, 0x80,0x0A, 0x80,0x12, 0x80,0x1A, 0x80,0x22, 0x80,0x2A, 0x80,0x32, 0x80,0x3A;9
  .db 0xD0,0x02, 0xD0,0x0A, 0xD0,0x12, 0xD0,0x1A, 0xD0,0x22, 0xD0,0x2A, 0xD0,0x32, 0xD0,0x3A;10
  .db 0x20,0x03, 0x20,0x0B, 0x20,0x13, 0x20,0x1B, 0x20,0x23, 0x20,0x2B, 0x20,0x33, 0x20,0x3B;11
  .db 0x70,0x03, 0x70,0x0B, 0x70,0x13, 0x70,0x1B, 0x70,0x23, 0x70,0x2B, 0x70,0x33, 0x70,0x3B;12
  .db 0xC0,0x03, 0xC0,0x0B, 0xC0,0x13, 0xC0,0x1B, 0xC0,0x23, 0xC0,0x2B, 0xC0,0x33, 0xC0,0x3B;13
  .db 0x10,0x04, 0x10,0x0C, 0x10,0x14, 0x10,0x1C, 0x10,0x24, 0x10,0x2C, 0x10,0x34, 0x10,0x3C;14
  .db 0x60,0x04, 0x60,0x0C, 0x60,0x14, 0x60,0x1C, 0x60,0x24, 0x60,0x2C, 0x60,0x34, 0x60,0x3C;15
  .db 0xB0,0x04, 0xB0,0x0C, 0xB0,0x14, 0xB0,0x1C, 0xB0,0x24, 0xB0,0x2C, 0xB0,0x34, 0xB0,0x3C;16
  .db 0x00,0x05, 0x00,0x0D, 0x00,0x15, 0x00,0x1D, 0x00,0x25, 0x00,0x2D, 0x00,0x35, 0x00,0x3D;17
  .db 0x50,0x05, 0x50,0x0D, 0x50,0x15, 0x50,0x1D, 0x50,0x25, 0x50,0x2D, 0x50,0x35, 0x50,0x3D;18
  .db 0xA0,0x05, 0xA0,0x0D, 0xA0,0x15, 0xA0,0x1D, 0xA0,0x25, 0xA0,0x2D, 0xA0,0x35, 0xA0,0x3D;19
  .db 0xF0,0x05, 0xF0,0x0D, 0xF0,0x15, 0xF0,0x1D, 0xF0,0x25, 0xF0,0x2D, 0xF0,0x35, 0xF0,0x3D;20
  .db 0x40,0x06, 0x40,0x0E, 0x40,0x16, 0x40,0x1E, 0x40,0x26, 0x40,0x2E, 0x40,0x36, 0x40,0x3E;21
  .db 0x90,0x06, 0x90,0x0E, 0x90,0x16, 0x90,0x1E, 0x90,0x26, 0x90,0x2E, 0x90,0x36, 0x90,0x3E;22
  .db 0xE0,0x06, 0xE0,0x0E, 0xE0,0x16, 0xE0,0x1E, 0xE0,0x26, 0xE0,0x2E, 0xE0,0x36, 0xE0,0x3E;23
  .db 0x30,0x07, 0x30,0x0F, 0x30,0x17, 0x30,0x1F, 0x30,0x27, 0x30,0x2F, 0x30,0x37, 0x30,0x3F;24
  .db 0x80,0x07, 0x80,0x0F, 0x80,0x17, 0x80,0x1F, 0x80,0x27, 0x80,0x2F, 0x80,0x37, 0x80,0x3F;25