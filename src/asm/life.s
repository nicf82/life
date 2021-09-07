; unsigned char asmevolve(unsigned char *cell)

.globl	_asmevolve

_asmevolve::

  ld bc, #(16*25) ;Cell counter (decrements), check if off-by-1

  ld	hl,#2
  add	hl,sp
  ld	a,(hl)
  ld	e,a
  inc	hl
  ld	a,(hl)
  ld	d,a  
  ex  de, hl ;hl contains *cell

  ; ld l, (hl)
  ; ret


  ld d, #1 ;bitmask - when its 255, this byte is complete
  ld e, #0 ;nbr count
  
_asme_nextbit::

  ;;; These 7 only work for bit 1 to 6, 0 and 7 need a neighboring byte

  ;Top byte
  sub hl, #16 ;Needs to be a 16 bit operation using sub - prob do it once and store the offset in memory

  ;Check top-left - bits 1 to 6
  ld a, (hl)
  and d
  jr z, _skip_tl
  inc e
  _skip_tl::

  ;Check top-center - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_tc
  inc e
  _skip_tc::

  ;Check top-right - bits 1 to 6
  ld a, (hl)
  sla d
  and d
  jr z, _skip_tr
  inc e
  _skip_tr::

  ;Middle byte
  add hl, #16

  ;Check centre-left
  ld a, (hl)
  and d ;Check the current bit
  jr z, _skip_cl
  inc e
  _skip_cl::

  ;Check centre-right - bits 1 to 6
  ld a, (hl)
  sla d
  sla d
  and d
  jr z, _skip_cr
  inc e
  _skip_cr::

_asme_endbit::
  ld l, e

  ret



  

