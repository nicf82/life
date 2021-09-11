
_hl_sub_bc::
  scf
  ccf
  sbc hl, bc
  ret

_hl_add_bc::
  scf
  ccf
  adc hl, bc
  ret


; unsigned char asmevolve(unsigned char *cell)

_higher_cell::
  .dw #0x0000

_cell::
  .dw #0x0000

_lower_cell::
  .dw #0x0000


.globl	_asmevolve

_asmevolve::

  ld	hl,#2
  add	hl,sp
  ld	a,(hl)
  ld	e,a
  inc	hl
  ld	a,(hl)
  ld	d,a  
  ex  de, hl ;hl contains *cell

  ;Store the offsets of cell-10, cell and cell+10 (10*8 bits = 80 columns)
  ld (_cell), hl
  ld bc, #10     ;(X_MAX/8)
  call _hl_sub_bc
  ld (_higher_cell), hl
  call _hl_add_bc
  call _hl_add_bc
  ld (_lower_cell), hl
  call _hl_sub_bc

  ; ld bc, #0x0000 ;Current X and Y
  ld b, #0
  ld c, #0


  ld d, #0b00000010 ;bitmask - when its 255, this byte is complete
  ld e, #0 ;nbr count
  
_asme_nextbit::

  ;;; These 8 only work for bit 1 to 6, 0 and 7 need a neighboring byte

  ;Higher byte
  ld hl, (_higher_cell)
  srl d

  ;Check upper-left - bits 1 to 6
  ld a, (hl)
  and d
  jr z, _skip_ul
  ; ld l, #0x89     ;This was called
  ; ret
  inc e
  _skip_ul::

  ;Check upper-center - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_uc
  inc e
  _skip_uc::

  ;Check upper-right - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_ur
  inc e
  _skip_ur::


  ;Lower byte
  srl d
  srl d
  ld hl, (_lower_cell)

  ;Check lower-left - bits 1 to 6
  ld a, (hl)
  and d
  jr z, _skip_ll
  inc e
  _skip_ll::

  ;Check lower-center - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_lc
  inc e
  _skip_lc::

  ;Check lower-right - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_lr
  inc e
  _skip_lr::


  ;Middle byte
  srl d
  srl d
  ld hl, (_cell)

  ;Check middle-left
  ld a, (hl)
  and d ;Check the current bit
  jr z, _skip_cl
  inc e
  _skip_cl::

  ;Check middle-right - bits 1 to 6
  ld a, (hl)
  sla d
  sla d
  and d
  jr z, _skip_cr
  inc e
  _skip_cr::

  srl d ;Back to the original bit

  ; ld l, d
  ; ret

  ;life_check - HERE is where the survive/birth/death will go for this bit, e contains the nbr count
  ld a, (hl)
  and d
  jr z, _asme_cell_dead

  ;;Cell is alive - survives if nbrs is 2 or 3

  ;Check if survivor
  ld a, e
  and #0b11111110
  cp #2
  jr z, _asme_after_life_check ;Z will have been set if neighbours was 2 or 3 - cell survives
  
  ;Kill the cell
  ld a, d
  xor #0xFF ;Invert the current bit to kill it
  ld d, a
  
  ld a, (hl)
  or d
  ld (hl), a

  ld a, d
  xor #0xFF ;Un-invert the current bit
  ld d, a

  call clrblk

_asme_cell_dead:: ;Is a birth if neighbours is 3

  ;Check if birth
  ld a, e
  cp #3
  jr nz, _asme_after_life_check ;Z will be clear if neighbours was not 3 - not a birth

  ;Bring the cell to life
  ld a, (hl)
  or d
  ld (hl), a

  call putblk

_asme_after_life_check::


  ;Check if we are on bit 6 - the last one that can be processed with this current logic
  ld a, d
  and #0b01000000

_asme_endbit::
  ; ld l, #0x69
  ld l, e

  ret




;
; Stuff from vidmem.s - couldnt access it otherwise without some messing about
;
  

putblk:
  
  ;Temporary - we should set/clear bytes at a time

  push bc
  push hl
  
  rl c
  rl c
  call get_screen_pos

  ld (hl), #0xEE
	call get_next_line
  ld (hl), #0xEE
	call get_next_line
  ld (hl), #0xEE

  pop hl
  pop bc

  ret

clrblk:

  ;Temporary - we should set/clear bytes at a time

  push bc
  push hl
  
  rl c
  rl c
  call get_screen_pos
  
  ld (hl), #0x0E
	call get_next_line
  ld (hl), #0x0E
	call get_next_line
  ld (hl), #0x0E

  pop hl
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

;Eaxh word here is the offset of a line of the CPC screen, 8*25 = 200 lines

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