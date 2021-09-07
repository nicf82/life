; unsigned char asmevolve(unsigned char *top)

.globl	_asmevolve

_asmevolve::

  ld	hl,#2
  add	hl,sp
  ld	a,(hl)
  ld	e,a  ;l contains the top bit
  inc	hl
  ld	a,(hl)
  ld	d,a  ;de contains *top
  
  ex de, hl

  ld l, (hl)

  ret



  

