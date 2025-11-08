	.section	.text,"ax",@progbits
	.assume	adl = 1
	.section	.text,"ax",@progbits
	.global	___isleap
___isleap:
	call	__frameset0
	ld	de, (ix + 6)
	ld	bc, 100
	push	de
	pop	hl
	call	__irems
	add	hl, bc
	or	a, a
	sbc	hl, bc
	jr	nz, BB0_2
	ld	bc, 400
	ex	de, hl
	call	__irems
	jr	BB0_3
BB0_2:
	ld	bc, 3
	ex	de, hl
	call	__iand
BB0_3:
	add	hl, bc
	or	a, a
	sbc	hl, bc
	jr	z, BB0_5
	ld	a, 0
	jr	BB0_6
BB0_5:
	ld	a, -1
BB0_6:
	pop	ix
	ret
	.section	.text,"ax",@progbits

	.extern	__iand
	.extern	__irems
	.extern	__frameset0
