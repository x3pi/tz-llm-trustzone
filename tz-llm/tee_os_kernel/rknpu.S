
ramdisk-dir/rknpu.srv:	file format elf64-littleaarch64

Disassembly of section .text:

0000000000002084 <_start>:
    2084: 1d 00 80 d2  	mov	x29, #0
    2088: 1e 00 80 d2  	mov	x30, #0
    208c: e0 03 00 91  	mov	x0, sp
    2090: 1f 20 03 d5  	nop
    2094: 61 6c 01 10  	adr	x1, #11660
    2098: 1f ec 7c 92  	and	sp, x0, #0xfffffffffffffff0
    209c: 01 00 00 14  	b	0x20a0 <_start_c>

00000000000020a0 <_start_c>:
    20a0: e2 03 00 aa  	mov	x2, x0
    20a4: 00 00 00 d0  	adrp	x0, 0x4000 <_start_c+0xc>
    20a8: 03 00 00 d0  	adrp	x3, 0x4000 <_start_c+0x10>
    20ac: 04 00 00 d0  	adrp	x4, 0x4000 <_start_c+0x14>
    20b0: e5 03 1f aa  	mov	x5, xzr
    20b4: 00 d8 47 f9  	ldr	x0, [x0, #4016]
    20b8: 63 dc 47 f9  	ldr	x3, [x3, #4024]
    20bc: 41 84 40 b8  	ldr	w1, [x2], #8
    20c0: 84 e0 47 f9  	ldr	x4, [x4, #4032]
    20c4: f7 06 00 14  	b	0x3ca0 <__libc_start_main@plt>

00000000000020c8 <__do_init>:
    20c8: ff 83 00 d1  	sub	sp, sp, #32
    20cc: fd 7b 01 a9  	stp	x29, x30, [sp, #16]
    20d0: fd 43 00 91  	add	x29, sp, #16
    20d4: 08 00 00 d0  	adrp	x8, 0x4000 <__do_init+0x14>
    20d8: 08 e5 47 f9  	ldr	x8, [x8, #4040]
    20dc: 08 01 00 f1  	subs	x8, x8, #0
    20e0: e8 17 9f 1a  	cset	w8, eq
    20e4: a8 c3 1f b8  	stur	w8, [x29, #-4]
    20e8: 28 00 00 90  	adrp	x8, 0x6000 <__do_init+0x30>
    20ec: 08 01 43 39  	ldrb	w8, [x8, #192]
    20f0: 68 00 00 36  	tbz	w8, #0, 0x20fc <__do_init+0x34>
    20f4: 01 00 00 14  	b	0x20f8 <__do_init+0x30>
    20f8: 0d 00 00 14  	b	0x212c <__do_init+0x64>
    20fc: a8 c3 5f b8  	ldur	w8, [x29, #-4]
    2100: 29 00 80 52  	mov	w9, #1
    2104: 2a 00 00 90  	adrp	x10, 0x6000 <__do_init+0x4c>
    2108: 49 01 03 39  	strb	w9, [x10, #192]
    210c: 08 01 00 37  	tbnz	w8, #0, 0x212c <__do_init+0x64>
    2110: 01 00 00 14  	b	0x2114 <__do_init+0x4c>
    2114: 1f 20 03 d5  	nop
    2118: 40 7b ff 10  	adr	x0, #-4248
    211c: 1f 20 03 d5  	nop
    2120: 41 fd 01 10  	adr	x1, #16296
    2124: e3 06 00 94  	bl	0x3cb0 <__register_frame_info@plt>
    2128: 01 00 00 14  	b	0x212c <__do_init+0x64>
    212c: fd 7b 41 a9  	ldp	x29, x30, [sp, #16]
    2130: ff 83 00 91  	add	sp, sp, #32
    2134: c0 03 5f d6  	ret

0000000000002138 <__do_fini>:
    2138: ff 83 00 d1  	sub	sp, sp, #32
    213c: fd 7b 01 a9  	stp	x29, x30, [sp, #16]
    2140: fd 43 00 91  	add	x29, sp, #16
    2144: 08 00 00 d0  	adrp	x8, 0x4000 <__do_fini+0x14>
    2148: 08 e9 47 f9  	ldr	x8, [x8, #4048]
    214c: 08 01 00 f1  	subs	x8, x8, #0
    2150: e8 17 9f 1a  	cset	w8, eq
    2154: e8 07 00 b9  	str	w8, [sp, #4]
    2158: 08 00 00 d0  	adrp	x8, 0x4000 <__do_fini+0x28>
    215c: 08 ed 47 f9  	ldr	x8, [x8, #4056]
    2160: 08 01 00 f1  	subs	x8, x8, #0
    2164: e8 17 9f 1a  	cset	w8, eq
    2168: e8 0b 00 b9  	str	w8, [sp, #8]
    216c: 08 00 00 d0  	adrp	x8, 0x4000 <__do_fini+0x3c>
    2170: 08 f1 47 f9  	ldr	x8, [x8, #4064]
    2174: 08 01 00 f1  	subs	x8, x8, #0
    2178: e8 17 9f 1a  	cset	w8, eq
    217c: a8 c3 1f b8  	stur	w8, [x29, #-4]
    2180: 28 00 00 90  	adrp	x8, 0x6000 <__do_fini+0x58>
    2184: 08 21 44 39  	ldrb	w8, [x8, #264]
    2188: 68 00 00 36  	tbz	w8, #0, 0x2194 <__do_fini+0x5c>
    218c: 01 00 00 14  	b	0x2190 <__do_fini+0x58>
    2190: 17 00 00 14  	b	0x21ec <__do_fini+0xb4>
    2194: e8 07 40 b9  	ldr	w8, [sp, #4]
    2198: 29 00 80 52  	mov	w9, #1
    219c: 2a 00 00 90  	adrp	x10, 0x6000 <__do_fini+0x74>
    21a0: 49 21 04 39  	strb	w9, [x10, #264]
    21a4: 88 00 00 37  	tbnz	w8, #0, 0x21b4 <__do_fini+0x7c>
    21a8: 01 00 00 14  	b	0x21ac <__do_fini+0x74>
    21ac: c5 06 00 94  	bl	0x3cc0 <__at_fini@plt>
    21b0: 01 00 00 14  	b	0x21b4 <__do_fini+0x7c>
    21b4: e8 0b 40 b9  	ldr	w8, [sp, #8]
    21b8: c8 00 00 37  	tbnz	w8, #0, 0x21d0 <__do_fini+0x98>
    21bc: 01 00 00 14  	b	0x21c0 <__do_fini+0x88>
    21c0: 28 00 00 90  	adrp	x8, 0x6000 <__do_fini+0x98>
    21c4: 00 5d 40 f9  	ldr	x0, [x8, #184]
    21c8: c2 06 00 94  	bl	0x3cd0 <__cxa_finalize@plt>
    21cc: 01 00 00 14  	b	0x21d0 <__do_fini+0x98>
    21d0: a8 c3 5f b8  	ldur	w8, [x29, #-4]
    21d4: c8 00 00 37  	tbnz	w8, #0, 0x21ec <__do_fini+0xb4>
    21d8: 01 00 00 14  	b	0x21dc <__do_fini+0xa4>
    21dc: 1f 20 03 d5  	nop
    21e0: 00 75 ff 10  	adr	x0, #-4448
    21e4: bf 06 00 94  	bl	0x3ce0 <__deregister_frame_info@plt>
    21e8: 01 00 00 14  	b	0x21ec <__do_fini+0xb4>
    21ec: fd 7b 41 a9  	ldp	x29, x30, [sp, #16]
    21f0: ff 83 00 91  	add	sp, sp, #32
    21f4: c0 03 5f d6  	ret

00000000000021f8 <mem_allocate>:
    21f8: fd 7b bc a9  	stp	x29, x30, [sp, #-64]!
    21fc: 00 04 80 52  	mov	w0, #32
    2200: f7 0b 00 f9  	str	x23, [sp, #16]
    2204: f6 57 02 a9  	stp	x22, x21, [sp, #32]
    2208: fd 03 00 91  	mov	x29, sp
    220c: f4 4f 03 a9  	stp	x20, x19, [sp, #48]
    2210: f3 03 05 aa  	mov	x19, x5
    2214: f4 03 03 aa  	mov	x20, x3
    2218: f5 03 02 aa  	mov	x21, x2
    221c: f7 03 01 aa  	mov	x23, x1
    2220: b4 06 00 94  	bl	0x3cf0 <malloc@plt>
    2224: e0 01 00 b4  	cbz	x0, 0x2260 <mem_allocate+0x68>
    2228: f6 03 00 aa  	mov	x22, x0
    222c: e0 03 17 aa  	mov	x0, x23
    2230: e1 03 16 aa  	mov	x1, x22
    2234: b3 06 00 94  	bl	0x3d00 <chcore_alloc_dma_mem@plt>
    2238: 40 02 00 b4  	cbz	x0, 0x2280 <mem_allocate+0x88>
    223c: c0 22 41 a9  	ldp	x0, x8, [x22, #16]
    2240: f7 0b 40 f9  	ldr	x23, [sp, #16]
    2244: a8 02 00 f9  	str	x8, [x21]
    2248: 80 02 00 f9  	str	x0, [x20]
    224c: 76 02 00 f9  	str	x22, [x19]
    2250: f4 4f 43 a9  	ldp	x20, x19, [sp, #48]
    2254: f6 57 42 a9  	ldp	x22, x21, [sp, #32]
    2258: fd 7b c4 a8  	ldp	x29, x30, [sp], #64
    225c: c0 03 5f d6  	ret
    2260: 1f 20 03 d5  	nop
    2264: 40 54 ff 30  	adr	x0, #-5495
    2268: e1 ff ff d0  	adrp	x1, 0x0 <mem_allocate+0x68>
    226c: 21 f4 2e 91  	add	x1, x1, #3005
    2270: e3 ff ff d0  	adrp	x3, 0x0 <mem_allocate+0x70>
    2274: 63 08 2d 91  	add	x3, x3, #2882
    2278: 62 08 80 52  	mov	w2, #67
    227c: a5 06 00 94  	bl	0x3d10 <__assert_fail@plt>
    2280: e0 ff ff d0  	adrp	x0, 0x0 <mem_allocate+0x80>
    2284: 00 44 2f 91  	add	x0, x0, #3025
    2288: e1 ff ff d0  	adrp	x1, 0x0 <mem_allocate+0x88>
    228c: 21 f4 2e 91  	add	x1, x1, #3005
    2290: e3 ff ff d0  	adrp	x3, 0x0 <mem_allocate+0x90>
    2294: 63 08 2d 91  	add	x3, x3, #2882
    2298: c2 08 80 52  	mov	w2, #70
    229c: 9d 06 00 94  	bl	0x3d10 <__assert_fail@plt>

00000000000022a0 <mem_destroy>:
    22a0: e0 03 01 aa  	mov	x0, x1
    22a4: 9f 06 00 14  	b	0x3d20 <chcore_free_dma_mem@plt>

00000000000022a8 <npu_open>:
    22a8: ff 43 01 d1  	sub	sp, sp, #80
    22ac: e9 ff ff d0  	adrp	x9, 0x0 <mem_destroy+0x4>
    22b0: 29 01 38 91  	add	x9, x9, #3584
    22b4: 1f 20 03 d5  	nop
    22b8: 88 59 ff 10  	adr	x8, #-5328
    22bc: fd 7b 04 a9  	stp	x29, x30, [sp, #64]
    22c0: fd 03 01 91  	add	x29, sp, #64
    22c4: 21 01 c0 3d  	ldr	q1, [x9]
    22c8: 09 00 94 52  	mov	w9, #40960
    22cc: 00 01 c0 3d  	ldr	q0, [x8]
    22d0: a8 b5 bf 52  	mov	w8, #-38993920
    22d4: 89 b5 bf 72  	movk	w9, #64940, lsl #16
    22d8: a0 23 00 d1  	sub	x0, x29, #8
    22dc: e1 83 00 91  	add	x1, sp, #32
    22e0: e2 03 00 91  	mov	x2, sp
    22e4: bf 83 1f f8  	stur	xzr, [x29, #-8]
    22e8: e0 0b 80 3d  	str	q0, [sp, #32]
    22ec: e8 1b 00 f9  	str	x8, [sp, #48]
    22f0: e1 03 80 3d  	str	q1, [sp]
    22f4: e9 0b 00 f9  	str	x9, [sp, #16]
    22f8: e4 02 00 94  	bl	0x2e88 <rknpu_init>
    22fc: a0 83 5f f8  	ldur	x0, [x29, #-8]
    2300: fd 7b 44 a9  	ldp	x29, x30, [sp, #64]
    2304: ff 43 01 91  	add	sp, sp, #80
    2308: c0 03 5f d6  	ret

000000000000230c <npu_close>:
    230c: e0 03 1f 2a  	mov	w0, wzr
    2310: c0 03 5f d6  	ret

0000000000002314 <npu_reset>:
    2314: 22 04 00 14  	b	0x339c <rknpu_soft_reset>

0000000000002318 <gen_matmul_task>:
    2318: 29 0c 40 39  	ldrb	w9, [x1, #3]
    231c: 88 00 88 d2  	mov	x8, #16388
    2320: 2b 08 40 39  	ldrb	w11, [x1, #2]
    2324: 8c 01 82 d2  	mov	x12, #4108
    2328: 2d 04 40 39  	ldrb	w13, [x1, #1]
    232c: c8 01 a0 f2  	movk	x8, #14, lsl #16
    2330: 29 09 40 92  	and	x9, x9, #0x7
    2334: 2e 0c 40 79  	ldrh	w14, [x1, #6]
    2338: 6b 6d 1c 53  	lsl	w11, w11, #4
    233c: 2f 24 40 39  	ldrb	w15, [x1, #9]
    2340: 6b 09 7c 92  	and	x11, x11, #0x70
    2344: 30 10 40 39  	ldrb	w16, [x1, #4]
    2348: 2b 09 79 b3  	bfi	x11, x9, #7, #3
    234c: 29 20 40 39  	ldrb	w9, [x1, #8]
    2350: ab 0d 40 b3  	bfxil	x11, x13, #0, #4
    2354: ce 6d 1c 53  	lsl	w14, w14, #4
    2358: ef 09 00 12  	and	w15, w15, #0x7
    235c: 2c 40 e0 f2  	movk	x12, #513, lsl #48
    2360: ce 25 7c 92  	and	x14, x14, #0x3ff0
    2364: 2f 09 1d 33  	bfi	w15, w9, #3, #3
    2368: 28 00 e2 f2  	movk	x8, #4097, lsl #48
    236c: 8a 01 82 d2  	mov	x10, #4108
    2370: c9 bd 70 d3  	lsl	x9, x14, #16
    2374: 8d 01 82 d2  	mov	x13, #4108
    2378: 6c 25 70 b3  	bfi	x12, x11, #16, #10
    237c: 2a 40 e0 f2  	movk	x10, #513, lsl #48
    2380: 2d 40 e0 f2  	movk	x13, #513, lsl #48
    2384: 09 1e 60 b3  	bfi	x9, x16, #32, #8
    2388: 29 01 0a aa  	orr	x9, x9, x10
    238c: ed 15 70 b3  	bfi	x13, x15, #16, #6
    2390: 29 11 00 91  	add	x9, x9, #4
    2394: ab 21 00 91  	add	x11, x13, #8
    2398: 08 30 00 a9  	stp	x8, x12, [x0]
    239c: 28 1c 40 79  	ldrh	w8, [x1, #14]
    23a0: 2d 18 40 79  	ldrh	w13, [x1, #12]
    23a4: 2e 14 40 79  	ldrh	w14, [x1, #10]
    23a8: 09 2c 01 a9  	stp	x9, x11, [x0, #16]
    23ac: 0b 3d 10 53  	lsl	w11, w8, #16
    23b0: 2f 44 40 79  	ldrh	w15, [x1, #34]
    23b4: 6b 41 40 51  	sub	w11, w11, #16, lsl #12  // =65536
    23b8: ac 29 40 92  	and	x12, x13, #0x7ff
    23bc: 0b 3d 40 b3  	bfxil	x11, x8, #0, #16
    23c0: c9 29 00 12  	and	w9, w14, #0x7ff
    23c4: 8d 01 82 d2  	mov	x13, #4108
    23c8: 29 7d 60 d3  	lsl	x9, x9, #32
    23cc: 28 20 40 79  	ldrh	w8, [x1, #16]
    23d0: 2d 40 e0 f2  	movk	x13, #513, lsl #48
    23d4: 89 29 70 b3  	bfi	x9, x12, #16, #11
    23d8: 6d 7d 70 b3  	bfi	x13, x11, #16, #32
    23dc: 2c ac 42 29  	ldp	w12, w11, [x1, #20]
    23e0: 08 29 40 92  	and	x8, x8, #0x7ff
    23e4: 8e 01 82 d2  	mov	x14, #4108
    23e8: 2e 40 e0 f2  	movk	x14, #513, lsl #48
    23ec: 29 01 0a aa  	orr	x9, x9, x10
    23f0: 29 51 00 91  	add	x9, x9, #20
    23f4: ad 61 00 91  	add	x13, x13, #24
    23f8: 8c 45 40 92  	and	x12, x12, #0x3ffff
    23fc: 0e 29 70 b3  	bfi	x14, x8, #16, #11
    2400: 88 01 82 d2  	mov	x8, #4108
    2404: ce 71 00 91  	add	x14, x14, #28
    2408: 28 40 e0 f2  	movk	x8, #513, lsl #48
    240c: 09 34 02 a9  	stp	x9, x13, [x0, #32]
    2410: 88 45 70 b3  	bfi	x8, x12, #16, #18
    2414: 29 1c 40 b9  	ldr	w9, [x1, #28]
    2418: 0c 01 7b b2  	orr	x12, x8, #0x20
    241c: 2d 80 40 39  	ldrb	w13, [x1, #32]
    2420: 30 a8 40 39  	ldrb	w16, [x1, #42]
    2424: 08 20 04 91  	add	x8, x0, #264
    2428: 29 49 40 92  	and	x9, x9, #0x7ffff
    242c: 20 40 44 fc  	ldur	d0, [x1, #68]
    2430: 0e 30 03 a9  	stp	x14, x12, [x0, #48]
    2434: 8c 01 82 d2  	mov	x12, #4108
    2438: 2c 40 e0 f2  	movk	x12, #513, lsl #48
    243c: 8e 01 82 d2  	mov	x14, #4108
    2440: 6c 7d 70 b3  	bfi	x12, x11, #16, #32
    2444: 2b 84 40 39  	ldrb	w11, [x1, #33]
    2448: 2e 40 e0 f2  	movk	x14, #513, lsl #48
    244c: ad 11 00 12  	and	w13, w13, #0x1f
    2450: 2e 49 70 b3  	bfi	x14, x9, #16, #19
    2454: 8c 91 00 91  	add	x12, x12, #36
    2458: 6b 11 00 12  	and	w11, w11, #0x1f
    245c: 00 76 07 2f  	bic	v0.2s, #240, lsl #24
    2460: 69 bd 70 d3  	lsl	x9, x11, #16
    2464: cb a1 00 91  	add	x11, x14, #40
    2468: 29 61 0d aa  	orr	x9, x9, x13, lsl #24
    246c: 2d 90 40 39  	ldrb	w13, [x1, #36]
    2470: 2e 94 40 39  	ldrb	w14, [x1, #37]
    2474: e9 35 40 b3  	bfxil	x9, x15, #0, #14
    2478: 0c 2c 04 a9  	stp	x12, x11, [x0, #64]
    247c: 2c a0 40 39  	ldrb	w12, [x1, #40]
    2480: 2b 4c 40 79  	ldrh	w11, [x1, #38]
    2484: 8f 01 82 d2  	mov	x15, #4108
    2488: ae 6d 1c 33  	bfi	w14, w13, #4, #28
    248c: 2f 40 e0 f2  	movk	x15, #513, lsl #48
    2490: cd 1d 40 92  	and	x13, x14, #0xff
    2494: 2e a4 40 39  	ldrb	w14, [x1, #41]
    2498: 8c 01 1d 53  	ubfiz	w12, w12, #3, #1
    249c: 6b 31 40 92  	and	x11, x11, #0x1fff
    24a0: 49 41 09 aa  	orr	x9, x10, x9, lsl #16
    24a4: cc 01 1f 33  	bfi	w12, w14, #1, #1
    24a8: af 1d 70 b3  	bfi	x15, x13, #16, #8
    24ac: 8d 01 82 d2  	mov	x13, #4108
    24b0: 0c 02 00 33  	bfxil	w12, w16, #0, #1
    24b4: 2d 40 e0 f2  	movk	x13, #513, lsl #48
    24b8: 29 b1 00 91  	add	x9, x9, #44
    24bc: ee d1 00 91  	add	x14, x15, #52
    24c0: 6d 31 70 b3  	bfi	x13, x11, #16, #13
    24c4: 4c 41 0c aa  	orr	x12, x10, x12, lsl #16
    24c8: ab e1 00 91  	add	x11, x13, #56
    24cc: 8c 01 7a b2  	orr	x12, x12, #0x40
    24d0: 2d 5c 40 79  	ldrh	w13, [x1, #46]
    24d4: 09 38 05 a9  	stp	x9, x14, [x0, #80]
    24d8: 29 58 40 79  	ldrh	w9, [x1, #44]
    24dc: 2e 64 40 79  	ldrh	w14, [x1, #50]
    24e0: 90 01 82 d2  	mov	x16, #4108
    24e4: 0b 30 06 a9  	stp	x11, x12, [x0, #96]
    24e8: 8b 01 82 d2  	mov	x11, #4108
    24ec: 2b 40 e0 f2  	movk	x11, #513, lsl #48
    24f0: 2c 60 40 79  	ldrh	w12, [x1, #48]
    24f4: 2b 3d 60 b3  	bfi	x11, x9, #32, #16
    24f8: 89 01 82 d2  	mov	x9, #4108
    24fc: 29 40 e0 f2  	movk	x9, #513, lsl #48
    2500: 6b 11 01 91  	add	x11, x11, #68
    2504: a9 3d 60 b3  	bfi	x9, x13, #32, #16
    2508: 30 40 e0 f2  	movk	x16, #513, lsl #48
    250c: 2d 21 01 91  	add	x13, x9, #72
    2510: 89 01 82 d2  	mov	x9, #4108
    2514: 29 40 e0 f2  	movk	x9, #513, lsl #48
    2518: d0 3d 60 b3  	bfi	x16, x14, #32, #16
    251c: 89 3d 60 b3  	bfi	x9, x12, #32, #16
    2520: 2c d0 40 39  	ldrb	w12, [x1, #52]
    2524: 2f 31 01 91  	add	x15, x9, #76
    2528: 09 0a 80 52  	mov	w9, #80
    252c: 0e 02 09 aa  	orr	x14, x16, x9
    2530: 30 e0 40 39  	ldrb	w16, [x1, #56]
    2534: 8c 01 40 92  	and	x12, x12, #0x1
    2538: 0b 34 07 a9  	stp	x11, x13, [x0, #112]
    253c: 2b e4 40 39  	ldrb	w11, [x1, #57]
    2540: 8d 01 82 d2  	mov	x13, #4108
    2544: 2d 40 e0 f2  	movk	x13, #513, lsl #48
    2548: 0f 38 08 a9  	stp	x15, x14, [x0, #128]
    254c: 8d 01 70 b3  	bfi	x13, x12, #16, #1
    2550: 2c 6c 40 79  	ldrh	w12, [x1, #54]
    2554: 0b 6e 1c 33  	bfi	w11, w16, #4, #28
    2558: 8e 01 82 d2  	mov	x14, #4108
    255c: 6b 1d 40 92  	and	x11, x11, #0xff
    2560: 2e 40 e0 f2  	movk	x14, #513, lsl #48
    2564: 8e 3d 70 b3  	bfi	x14, x12, #16, #16
    2568: 8c 01 82 d2  	mov	x12, #4108
    256c: 2f 3c 40 b9  	ldr	w15, [x1, #60]
    2570: 2c 40 e0 f2  	movk	x12, #513, lsl #48
    2574: ad 51 01 91  	add	x13, x13, #84
    2578: 6c 1d 70 b3  	bfi	x12, x11, #16, #8
    257c: 8b 01 82 d2  	mov	x11, #4108
    2580: ce 61 01 91  	add	x14, x14, #88
    2584: 2b 40 e0 f2  	movk	x11, #513, lsl #48
    2588: 8c 71 01 91  	add	x12, x12, #92
    258c: eb 7d 70 b3  	bfi	x11, x15, #16, #32
    2590: 2f 80 40 79  	ldrh	w15, [x1, #64]
    2594: 6b 91 01 91  	add	x11, x11, #100
    2598: 0d 38 09 a9  	stp	x13, x14, [x0, #144]
    259c: 2d 0c 41 39  	ldrb	w13, [x1, #67]
    25a0: 2e 08 41 39  	ldrb	w14, [x1, #66]
    25a4: 0c 2c 0a a9  	stp	x12, x11, [x0, #160]
    25a8: 8b 01 82 d2  	mov	x11, #4108
    25ac: ac 0d 40 92  	and	x12, x13, #0xf
    25b0: 2b 40 e0 f2  	movk	x11, #513, lsl #48
    25b4: cd 0d 00 12  	and	w13, w14, #0xf
    25b8: ee ff ff d0  	adrp	x14, 0x0 <gen_matmul_task+0x298>
    25bc: ad 7d 60 d3  	lsl	x13, x13, #32
    25c0: eb 3d 70 b3  	bfi	x11, x15, #16, #16
    25c4: 8d 0d 70 b3  	bfi	x13, x12, #16, #4
    25c8: 6b a1 01 91  	add	x11, x11, #104
    25cc: ac 01 0a aa  	orr	x12, x13, x10
    25d0: 2d 9c 40 79  	ldrh	w13, [x1, #78]
    25d4: 8c b1 01 91  	add	x12, x12, #108
    25d8: c1 d9 c3 3d  	ldr	q1, [x14, #3936]
    25dc: 2e 98 40 79  	ldrh	w14, [x1, #76]
    25e0: 2f 54 40 b9  	ldr	w15, [x1, #84]
    25e4: 0b 30 0b a9  	stp	x11, x12, [x0, #176]
    25e8: ab 29 40 92  	and	x11, x13, #0x7ff
    25ec: cc 29 00 12  	and	w12, w14, #0x7ff
    25f0: 8d 01 82 d2  	mov	x13, #4108
    25f4: 8c 7d 60 d3  	lsl	x12, x12, #32
    25f8: 2d 40 e0 f2  	movk	x13, #513, lsl #48
    25fc: 6c 29 70 b3  	bfi	x12, x11, #16, #11
    2600: 2b a0 40 79  	ldrh	w11, [x1, #80]
    2604: 8c 01 0a aa  	orr	x12, x12, x10
    2608: ea 7d 70 b3  	bfi	x10, x15, #16, #32
    260c: 00 a4 30 2f  	ushll	v0.2d, v0.2s, #16
    2610: ee ff ff d0  	adrp	x14, 0x0 <gen_matmul_task+0x2f0>
    2614: 6d 3d 70 b3  	bfi	x13, x11, #16, #16
    2618: 8c e1 01 91  	add	x12, x12, #120
    261c: ab f1 01 91  	add	x11, x13, #124
    2620: ed ff ff d0  	adrp	x13, 0x0 <gen_matmul_task+0x300>
    2624: 4a 11 04 91  	add	x10, x10, #260
    2628: 6f 10 40 39  	ldrb	w15, [x3, #4]
    262c: 00 1c a1 4e  	orr	v0.16b, v0.16b, v1.16b
    2630: 70 0c 40 39  	ldrb	w16, [x3, #3]
    2634: 00 30 80 3d  	str	q0, [x0, #192]
    2638: c0 b5 c3 3d  	ldr	q0, [x14, #3792]
    263c: 0c 2c 0d a9  	stp	x12, x11, [x0, #208]
    2640: a1 c1 c3 3d  	ldr	q1, [x13, #3840]
    2644: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x324>
    2648: 0a 78 00 f9  	str	x10, [x0, #240]
    264c: ea ff ff d0  	adrp	x10, 0x0 <gen_matmul_task+0x32c>
    2650: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x330>
    2654: 00 38 80 3d  	str	q0, [x0, #224]
    2658: 4d 08 40 79  	ldrh	w13, [x2, #4]
    265c: 01 80 8f 3c  	stur	q1, [x0, #248]
    2660: 60 dd c3 3d  	ldr	q0, [x11, #3952]
    2664: 41 9d c3 3d  	ldr	q1, [x10, #3696]
    2668: ea ff ff d0  	adrp	x10, 0x0 <gen_matmul_task+0x348>
    266c: 82 a1 c3 3d  	ldr	q2, [x12, #3712]
    2670: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x350>
    2674: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x354>
    2678: 4e 04 40 79  	ldrh	w14, [x2, #2]
    267c: 00 05 00 ad  	stp	q0, q1, [x8]
    2680: 40 f9 c3 3d  	ldr	q0, [x10, #4064]
    2684: 81 c5 c3 3d  	ldr	q1, [x12, #3856]
    2688: ea ff ff d0  	adrp	x10, 0x0 <gen_matmul_task+0x368>
    268c: 63 ad c3 3d  	ldr	q3, [x11, #3760]
    2690: ef 09 03 53  	lsl	w15, w15, #29
    2694: 4b 04 40 39  	ldrb	w11, [x2, #1]
    2698: 4c 00 40 39  	ldrb	w12, [x2]
    269c: 00 05 02 ad  	stp	q0, q1, [x8, #64]
    26a0: 40 d5 c3 3d  	ldr	q0, [x10, #3920]
    26a4: ea ff ff d0  	adrp	x10, 0x0 <gen_matmul_task+0x384>
    26a8: 6b 01 40 92  	and	x11, x11, #0x1
    26ac: 02 0d 01 ad  	stp	q2, q3, [x8, #32]
    26b0: 8c 09 40 92  	and	x12, x12, #0x7
    26b4: 41 8d c3 3d  	ldr	q1, [x10, #3632]
    26b8: 8a 9d 68 d3  	lsl	x10, x12, #24
    26bc: 6a 01 70 b3  	bfi	x10, x11, #16, #1
    26c0: ab bd 70 d3  	lsl	x11, x13, #16
    26c4: 0c 02 86 d2  	mov	x12, #12304
    26c8: cb 3d 60 b3  	bfi	x11, x14, #32, #16
    26cc: 2c 00 e1 f2  	movk	x12, #2049, lsl #48
    26d0: ed ff ff d0  	adrp	x13, 0x0 <gen_matmul_task+0x3b0>
    26d4: 6b 01 0c aa  	orr	x11, x11, x12
    26d8: 4a 01 0c aa  	orr	x10, x10, x12
    26dc: 6b 01 7e b2  	orr	x11, x11, #0x4
    26e0: 4e 0c 40 79  	ldrh	w14, [x2, #6]
    26e4: 00 05 03 ad  	stp	q0, q1, [x8, #96]
    26e8: a0 f5 c3 3d  	ldr	q0, [x13, #4048]
    26ec: 6d 04 40 39  	ldrb	w13, [x3, #1]
    26f0: e1 c7 00 0f  	movi	v1.2s, #31, msl #8
    26f4: 0a ac 18 a9  	stp	x10, x11, [x0, #392]
    26f8: 6a 00 40 39  	ldrb	w10, [x3]
    26fc: cc 3d 70 b3  	bfi	x12, x14, #16, #16
    2700: 6e 14 40 39  	ldrb	w14, [x3, #5]
    2704: 6b 08 40 39  	ldrb	w11, [x3, #2]
    2708: ad 71 1d 53  	lsl	w13, w13, #3
    270c: 4a 0d 40 92  	and	x10, x10, #0xf
    2710: ad 05 7d 92  	and	x13, x13, #0x18
    2714: ce 09 00 12  	and	w14, w14, #0x7
    2718: 00 68 80 3d  	str	q0, [x0, #416]
    271c: 6b 79 1f 53  	lsl	w11, w11, #1
    2720: 8c 01 7d b2  	orr	x12, x12, #0x8
    2724: 4d 0d 7b b3  	bfi	x13, x10, #5, #4
    2728: 6a 18 40 39  	ldrb	w10, [x3, #6]
    272c: cf 09 06 33  	bfi	w15, w14, #26, #3
    2730: ee 00 a0 d2  	mov	x14, #458752
    2734: 6d 09 40 b3  	bfxil	x13, x11, #0, #3
    2738: 0e 80 df f2  	movk	x14, #64512, lsl #32
    273c: eb 01 0a 2a  	orr	w11, w15, w10
    2740: 0d 02 40 b3  	bfxil	x13, x16, #0, #1
    2744: 8a 01 88 d2  	mov	x10, #16396
    2748: 8f 01 88 d2  	mov	x15, #16396
    274c: 2a 00 e2 f2  	movk	x10, #4097, lsl #48
    2750: cb 41 0b 8a  	and	x11, x14, x11, lsl #16
    2754: 2f 00 e2 f2  	movk	x15, #4097, lsl #48
    2758: 6b 01 0a aa  	orr	x11, x11, x10
    275c: af 21 70 b3  	bfi	x15, x13, #16, #9
    2760: 6b 11 00 91  	add	x11, x11, #4
    2764: 6d 20 40 79  	ldrh	w13, [x3, #16]
    2768: 0c cc 00 f9  	str	x12, [x0, #408]
    276c: 6c 08 40 b9  	ldr	w12, [x3, #8]
    2770: 4e 21 00 91  	add	x14, x10, #8
    2774: 0f 2c 1b a9  	stp	x15, x11, [x0, #432]
    2778: 6b 0c 40 b9  	ldr	w11, [x3, #12]
    277c: a0 01 27 1e  	fmov	s0, w13
    2780: 6d 24 40 79  	ldrh	w13, [x3, #18]
    2784: 8f 01 88 d2  	mov	x15, #16396
    2788: 70 68 40 39  	ldrb	w16, [x3, #26]
    278c: 6b 6d 1c 53  	lsl	w11, w11, #4
    2790: 2f 00 e2 f2  	movk	x15, #4097, lsl #48
    2794: 6b fd 44 d3  	lsr	x11, x11, #4
    2798: 8f 7d 70 b3  	bfi	x15, x12, #16, #32
    279c: a0 1d 0c 4e  	mov	v0.s[1], w13
    27a0: 8d 01 88 d2  	mov	x13, #16396
    27a4: 2d 00 e2 f2  	movk	x13, #4097, lsl #48
    27a8: ec 51 00 91  	add	x12, x15, #20
    27ac: 6d 6d 6c b3  	bfi	x13, x11, #20, #28
    27b0: ef ff ff d0  	adrp	x15, 0x0 <gen_matmul_task+0x490>
    27b4: ab 61 00 91  	add	x11, x13, #24
    27b8: 6d 60 40 39  	ldrb	w13, [x3, #24]
    27bc: 0e 30 1c a9  	stp	x14, x12, [x0, #448]
    27c0: 6e 5c 40 39  	ldrb	w14, [x3, #23]
    27c4: 00 1c 21 0e  	and	v0.8b, v0.8b, v1.8b
    27c8: e1 a9 c3 3d  	ldr	q1, [x15, #3744]
    27cc: 0b e8 00 f9  	str	x11, [x0, #464]
    27d0: 6b 64 40 39  	ldrb	w11, [x3, #25]
    27d4: 6c 28 40 79  	ldrh	w12, [x3, #20]
    27d8: 6f 58 40 39  	ldrb	w15, [x3, #22]
    27dc: 6b 01 1a 53  	ubfiz	w11, w11, #6, #1
    27e0: ab 01 1c 33  	bfi	w11, w13, #4, #1
    27e4: 6d 6c 40 39  	ldrb	w13, [x3, #27]
    27e8: cb 01 1f 33  	bfi	w11, w14, #1, #1
    27ec: 6e 70 40 39  	ldrb	w14, [x3, #28]
    27f0: 8c 31 40 92  	and	x12, x12, #0x1fff
    27f4: eb 01 00 33  	bfxil	w11, w15, #0, #1
    27f8: 6f 74 40 39  	ldrb	w15, [x3, #29]
    27fc: ad 09 40 92  	and	x13, x13, #0x7
    2800: ce 69 1b 53  	lsl	w14, w14, #5
    2804: 4b 41 0b aa  	orr	x11, x10, x11, lsl #16
    2808: 8c 31 70 b3  	bfi	x12, x12, #16, #13
    280c: ce 09 7b 92  	and	x14, x14, #0xe0
    2810: ef 75 1e 53  	lsl	w15, w15, #2
    2814: ae 09 78 b3  	bfi	x14, x13, #8, #3
    2818: 0d 7a 1f 53  	lsl	w13, w16, #1
    281c: 4c 41 0c aa  	orr	x12, x10, x12, lsl #16
    2820: 50 b1 00 91  	add	x16, x10, #44
    2824: 8c 05 7c b2  	orr	x12, x12, #0x30
    2828: ee 11 40 b3  	bfxil	x14, x15, #0, #5
    282c: 6b d1 00 91  	add	x11, x11, #52
    2830: ae 05 40 b3  	bfxil	x14, x13, #0, #2
    2834: 4f 01 7a b2  	orr	x15, x10, #0x40
    2838: cd fd 41 d3  	lsr	x13, x14, #1
    283c: 10 b0 1e a9  	stp	x16, x12, [x0, #488]
    2840: 8c 01 88 d2  	mov	x12, #16396
    2844: 6e 3c 40 79  	ldrh	w14, [x3, #30]
    2848: 2c 00 e2 f2  	movk	x12, #4097, lsl #48
    284c: 0b fc 00 f9  	str	x11, [x0, #504]
    2850: ac 25 6f b3  	bfi	x12, x13, #17, #10
    2854: 6d 44 40 79  	ldrh	w13, [x3, #34]
    2858: 8b 11 01 91  	add	x11, x12, #68
    285c: cc 31 40 92  	and	x12, x14, #0x1fff
    2860: 6e 40 40 79  	ldrh	w14, [x3, #32]
    2864: 0f 08 01 f9  	str	x15, [x0, #528]
    2868: 6f 94 40 39  	ldrb	w15, [x3, #37]
    286c: 0b 0c 01 f9  	str	x11, [x0, #536]
    2870: 8b 01 88 d2  	mov	x11, #16396
    2874: 2b 00 e2 f2  	movk	x11, #4097, lsl #48
    2878: 70 9c 40 39  	ldrb	w16, [x3, #39]
    287c: 8b 31 70 b3  	bfi	x11, x12, #16, #13
    2880: ac 31 40 92  	and	x12, x13, #0x1fff
    2884: cd 31 00 12  	and	w13, w14, #0x1fff
    2888: 6e 90 40 39  	ldrb	w14, [x3, #36]
    288c: ad 7d 60 d3  	lsl	x13, x13, #32
    2890: 6b 31 01 91  	add	x11, x11, #76
    2894: 8d 31 70 b3  	bfi	x13, x12, #16, #13
    2898: 6c 98 40 39  	ldrb	w12, [x3, #38]
    289c: ce 01 1a 53  	ubfiz	w14, w14, #6, #1
    28a0: ad 01 0a aa  	orr	x13, x13, x10
    28a4: ee 01 1c 33  	bfi	w14, w15, #4, #1
    28a8: a9 01 09 aa  	orr	x9, x13, x9
    28ac: 8e 01 1f 33  	bfi	w14, w12, #1, #1
    28b0: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x590>
    28b4: 0e 02 00 33  	bfxil	w14, w16, #0, #1
    28b8: 0b 14 01 f9  	str	x11, [x0, #552]
    28bc: 00 a4 30 2f  	ushll	v0.2d, v0.2s, #16
    28c0: 6b ac 40 39  	ldrb	w11, [x3, #43]
    28c4: 4d 41 0e aa  	orr	x13, x10, x14, lsl #16
    28c8: 09 18 01 f9  	str	x9, [x0, #560]
    28cc: ad 51 01 91  	add	x13, x13, #84
    28d0: 69 b0 40 39  	ldrb	w9, [x3, #44]
    28d4: 00 1c a1 4e  	orr	v0.16b, v0.16b, v1.16b
    28d8: 6b 01 40 92  	and	x11, x11, #0x1
    28dc: 00 35 80 3d  	str	q0, [x8, #208]
    28e0: 80 f1 c3 3d  	ldr	q0, [x12, #4032]
    28e4: 0d 1c 01 f9  	str	x13, [x0, #568]
    28e8: 6c a8 40 39  	ldrb	w12, [x3, #42]
    28ec: 6d a4 40 39  	ldrb	w13, [x3, #41]
    28f0: 29 01 40 92  	and	x9, x9, #0x1
    28f4: 29 d9 77 d3  	lsl	x9, x9, #9
    28f8: ee ff ff d0  	adrp	x14, 0x0 <gen_matmul_task+0x5d8>
    28fc: 69 01 78 b3  	bfi	x9, x11, #8, #1
    2900: 8b 61 19 53  	lsl	w11, w12, #7
    2904: ac 79 1f 53  	lsl	w12, w13, #1
    2908: 6b 01 79 92  	and	x11, x11, #0x80
    290c: 6d a0 40 39  	ldrb	w13, [x3, #40]
    2910: 29 01 0b aa  	orr	x9, x9, x11
    2914: 4b 91 01 91  	add	x11, x10, #100
    2918: 00 80 80 3d  	str	q0, [x0, #512]
    291c: 89 05 40 b3  	bfxil	x9, x12, #0, #2
    2920: 6c b4 40 39  	ldrb	w12, [x3, #45]
    2924: a9 01 40 b3  	bfxil	x9, x13, #0, #1
    2928: c0 c9 c3 3d  	ldr	q0, [x14, #3872]
    292c: 6e 5c 40 79  	ldrh	w14, [x3, #46]
    2930: ed ff ff d0  	adrp	x13, 0x0 <gen_matmul_task+0x610>
    2934: 69 41 09 aa  	orr	x9, x11, x9, lsl #16
    2938: 8c 01 00 12  	and	w12, w12, #0x1
    293c: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x61c>
    2940: 00 90 80 3d  	str	q0, [x0, #576]
    2944: a1 e5 c3 3d  	ldr	q1, [x13, #3984]
    2948: 4f 21 01 91  	add	x15, x10, #72
    294c: 09 2c 01 f9  	str	x9, [x0, #600]
    2950: c9 bd 70 d3  	lsl	x9, x14, #16
    2954: 89 01 60 b3  	bfi	x9, x12, #32, #1
    2958: 60 89 c3 3d  	ldr	q0, [x11, #3616]
    295c: 29 01 0a aa  	orr	x9, x9, x10
    2960: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x640>
    2964: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x644>
    2968: 29 e1 01 91  	add	x9, x9, #120
    296c: 00 04 13 ad  	stp	q0, q1, [x0, #608]
    2970: 60 b9 c3 3d  	ldr	q0, [x11, #3808]
    2974: eb ff ff f0  	adrp	x11, 0x1000 <gen_matmul_task+0x658>
    2978: 81 b1 c3 3d  	ldr	q1, [x12, #3776]
    297c: 09 40 01 f9  	str	x9, [x0, #640]
    2980: e9 ff ff d0  	adrp	x9, 0x0 <gen_matmul_task+0x660>
    2984: 6c 30 40 b9  	ldr	w12, [x3, #48]
    2988: 0f 10 01 f9  	str	x15, [x0, #544]
    298c: 4f 05 7b b2  	orr	x15, x10, #0x60
    2990: 00 05 0c ad  	stp	q0, q1, [x8, #384]
    2994: 60 05 c0 3d  	ldr	q0, [x11, #16]
    2998: 21 cd c3 3d  	ldr	q1, [x9, #3888]
    299c: 09 14 80 52  	mov	w9, #160
    29a0: 49 01 09 aa  	orr	x9, x10, x9
    29a4: 8b 6d 1c 53  	lsl	w11, w12, #4
    29a8: 6b fd 44 d3  	lsr	x11, x11, #4
    29ac: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x68c>
    29b0: 00 05 0d ad  	stp	q0, q1, [x8, #416]
    29b4: 09 64 01 f9  	str	x9, [x0, #712]
    29b8: e9 ff ff d0  	adrp	x9, 0x0 <gen_matmul_task+0x698>
    29bc: 6a 6d 6c b3  	bfi	x10, x11, #20, #28
    29c0: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x6a0>
    29c4: 80 91 c3 3d  	ldr	q0, [x12, #3648]
    29c8: ec ff ff d0  	adrp	x12, 0x0 <gen_matmul_task+0x6a8>
    29cc: 21 e9 c3 3d  	ldr	q1, [x9, #4000]
    29d0: e9 ff ff d0  	adrp	x9, 0x0 <gen_matmul_task+0x6b0>
    29d4: 62 95 c3 3d  	ldr	q2, [x11, #3664]
    29d8: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x6b8>
    29dc: 83 bd c3 3d  	ldr	q3, [x12, #3824]
    29e0: 4a d1 02 91  	add	x10, x10, #180
    29e4: 00 85 0e ad  	stp	q0, q1, [x8, #464]
    29e8: 20 fd c3 3d  	ldr	q0, [x9, #4080]
    29ec: e9 ff ff d0  	adrp	x9, 0x0 <gen_matmul_task+0x6cc>
    29f0: 61 e1 c3 3d  	ldr	q1, [x11, #3968]
    29f4: eb ff ff d0  	adrp	x11, 0x0 <gen_matmul_task+0x6d4>
    29f8: 02 8d 0f ad  	stp	q2, q3, [x8, #496]
    29fc: 0f 28 01 f9  	str	x15, [x0, #592]
    2a00: 22 a5 c3 3d  	ldr	q2, [x9, #3728]
    2a04: 09 01 80 d2  	mov	x9, #8
    2a08: a9 01 a0 f2  	movk	x9, #13, lsl #16
    2a0c: 00 85 10 ad  	stp	q0, q1, [x8, #528]
    2a10: 60 99 c3 3d  	ldr	q0, [x11, #3680]
    2a14: 29 10 e0 f2  	movk	x9, #129, lsl #48
    2a18: 0a 68 01 f9  	str	x10, [x0, #720]
    2a1c: 02 81 11 ad  	stp	q2, q0, [x8, #560]
    2a20: 09 ac 01 f9  	str	x9, [x0, #856]
    2a24: c0 03 5f d6  	ret

0000000000002a28 <gen_matmul_fp16>:
    2a28: ff c3 02 d1  	sub	sp, sp, #176
    2a2c: 08 40 80 52  	mov	w8, #512
    2a30: 2a 00 80 52  	mov	w10, #1
    2a34: 48 00 a0 72  	movk	w8, #2, lsl #16
    2a38: 29 20 80 52  	mov	w9, #257
    2a3c: 29 00 a0 72  	movk	w9, #1, lsl #16
    2a40: 2c 20 80 52  	mov	w12, #257
    2a44: 0b 04 40 79  	ldrh	w11, [x0, #2]
    2a48: ea a3 00 79  	strh	w10, [sp, #80]
    2a4c: e8 13 04 b8  	stur	w8, [sp, #65]
    2a50: 08 00 40 79  	ldrh	w8, [x0]
    2a54: e9 4b 00 b9  	str	w9, [sp, #72]
    2a58: 09 08 40 79  	ldrh	w9, [x0, #4]
    2a5c: ec c3 00 79  	strh	w12, [sp, #96]
    2a60: 6c 79 1f 53  	lsl	w12, w11, #1
    2a64: 0a 05 00 11  	add	w10, w8, #1
    2a68: fd 7b 0a a9  	stp	x29, x30, [sp, #160]
    2a6c: fd 83 02 91  	add	x29, sp, #160
    2a70: e8 9b 00 79  	strh	w8, [sp, #76]
    2a74: ec 5f 00 b9  	str	w12, [sp, #92]
    2a78: 8c 7d 09 1b  	mul	w12, w12, w9
    2a7c: ea 8f 00 79  	strh	w10, [sp, #70]
    2a80: 6a 7d 08 1b  	mul	w10, w11, w8
    2a84: eb 9f 00 79  	strh	w11, [sp, #78]
    2a88: 5f 35 00 72  	tst	w10, #0x3fff
    2a8c: 4a 79 0e 53  	ubfx	w10, w10, #14, #17
    2a90: 4a 05 8a 1a  	cinc	w10, w10, ne
    2a94: e8 33 01 79  	strh	w8, [sp, #152]
    2a98: 5f 2d 00 71  	cmp	w10, #11
    2a9c: e9 c7 00 79  	strh	w9, [sp, #98]
    2aa0: e8 b3 0a 29  	stp	w8, w12, [sp, #84]
    2aa4: a9 00 00 54  	b.ls	0x2ab8 <gen_matmul_fp16+0x90>
    2aa8: 00 00 80 12  	mov	w0, #-1
    2aac: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2ab0: ff c3 02 91  	add	sp, sp, #176
    2ab4: c0 03 5f d6  	ret
    2ab8: 7f 11 40 71  	cmp	w11, #4, lsl #12        // =16384
    2abc: 48 0a 00 54  	b.hi	0x2c04 <gen_matmul_fp16+0x1dc>
    2ac0: 6d 7d 05 53  	lsr	w13, w11, #5
    2ac4: 7f 11 00 72  	tst	w11, #0x1f
    2ac8: ad 05 8d 1a  	cinc	w13, w13, ne
    2acc: 8c 01 80 52  	mov	w12, #12
    2ad0: 2e 20 80 52  	mov	w14, #257
    2ad4: 8c 01 0a 4b  	sub	w12, w12, w10
    2ad8: eb 23 01 79  	strh	w11, [sp, #144]
    2adc: 0b 80 40 39  	ldrb	w11, [x0, #32]
    2ae0: ed cf 00 79  	strh	w13, [sp, #102]
    2ae4: 20 84 00 0f  	movi	v0.4h, #1
    2ae8: 0d 3c 41 29  	ldp	w13, w15, [x0, #8]
    2aec: ee d3 00 79  	strh	w14, [sp, #104]
    2af0: ee e1 a1 d2  	mov	x14, #252641280
    2af4: ec 93 01 39  	strb	w12, [sp, #100]
    2af8: 0c 35 1e 12  	and	w12, w8, #0xfffc
    2afc: 8e 00 c0 f2  	movk	x14, #4, lsl #32
    2b00: 8c 11 00 51  	sub	w12, w12, #4
    2b04: ed 7f 00 b9  	str	w13, [sp, #124]
    2b08: ed 01 80 52  	mov	w13, #15
    2b0c: 4d 00 a0 72  	movk	w13, #2, lsl #16
    2b10: ef 97 00 b9  	str	w15, [sp, #148]
    2b14: ee 43 00 f9  	str	x14, [sp, #128]
    2b18: 7f 01 00 71  	cmp	w11, #0
    2b1c: 4e 00 80 52  	mov	w14, #2
    2b20: af 00 80 52  	mov	w15, #5
    2b24: ed 03 00 b9  	str	w13, [sp]
    2b28: 4d 40 80 52  	mov	w13, #514
    2b2c: 8c 7d 1f 33  	bfxil	w12, w12, #31, #1
    2b30: ee 01 8e 1a  	csel	w14, w15, w14, eq
    2b34: 6b 01 00 12  	and	w11, w11, #0x1
    2b38: ea 97 01 39  	strb	w10, [sp, #101]
    2b3c: 2a 00 80 52  	mov	w10, #1
    2b40: ed 53 00 78  	sturh	w13, [sp, #5]
    2b44: ed c3 00 32  	mov	w13, #16843009
    2b48: ec 8b 00 b9  	str	w12, [sp, #136]
    2b4c: 4c 20 80 52  	mov	w12, #258
    2b50: ee 13 00 39  	strb	w14, [sp, #4]
    2b54: 0e 10 40 b9  	ldr	w14, [x0, #16]
    2b58: eb b7 00 39  	strb	w11, [sp, #45]
    2b5c: 4b 15 8a 1a  	cinc	w11, w10, eq
    2b60: ed 63 01 b8  	stur	w13, [sp, #22]
    2b64: 6d 00 80 52  	mov	w13, #3
    2b68: e8 1f 01 79  	strh	w8, [sp, #142]
    2b6c: ec 73 00 79  	strh	w12, [sp, #56]
    2b70: 0c 05 00 51  	sub	w12, w8, #1
    2b74: 29 05 00 51  	sub	w9, w9, #1
    2b78: ee 23 01 29  	stp	w14, w8, [sp, #8]
    2b7c: ee c3 00 b2  	mov	x14, #72340172838076673
    2b80: ad 05 9f 1a  	csinc	w13, w13, wzr, eq
    2b84: 08 21 cb 1a  	lsl	w8, w8, w11
    2b88: 00 0c 40 f9  	ldr	x0, [x0, #24]
    2b8c: e1 03 01 91  	add	x1, sp, #64
    2b90: e2 e3 00 91  	add	x2, sp, #56
    2b94: e3 03 00 91  	mov	x3, sp
    2b98: ea ab 01 39  	strb	w10, [sp, #106]
    2b9c: e0 c3 06 fc  	stur	d0, [sp, #108]
    2ba0: ff d3 01 39  	strb	wzr, [sp, #116]
    2ba4: ff 63 07 b8  	stur	wzr, [sp, #118]
    2ba8: ea 1b 01 79  	strh	w10, [sp, #140]
    2bac: ec 77 00 79  	strh	w12, [sp, #58]
    2bb0: ff 7b 00 79  	strh	wzr, [sp, #60]
    2bb4: e9 7f 00 79  	strh	w9, [sp, #62]
    2bb8: ff 23 00 79  	strh	wzr, [sp, #16]
    2bbc: ec 27 00 79  	strh	w12, [sp, #18]
    2bc0: e9 2b 00 79  	strh	w9, [sp, #20]
    2bc4: ee 43 02 f8  	stur	x14, [sp, #36]
    2bc8: ee b3 00 39  	strb	w14, [sp, #44]
    2bcc: ea 5f 00 79  	strh	w10, [sp, #46]
    2bd0: ed 6f 00 39  	strb	w13, [sp, #27]
    2bd4: ed 73 00 39  	strb	w13, [sp, #28]
    2bd8: ed 77 00 39  	strb	w13, [sp, #29]
    2bdc: ea 6b 00 39  	strb	w10, [sp, #26]
    2be0: ff 47 00 79  	strh	wzr, [sp, #34]
    2be4: ec 43 00 79  	strh	w12, [sp, #32]
    2be8: e9 3f 00 79  	strh	w9, [sp, #30]
    2bec: e8 33 00 b9  	str	w8, [sp, #48]
    2bf0: ca fd ff 97  	bl	0x2318 <gen_matmul_task>
    2bf4: e0 03 1f 2a  	mov	w0, wzr
    2bf8: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2bfc: ff c3 02 91  	add	sp, sp, #176
    2c00: c0 03 5f d6  	ret
    2c04: 20 00 80 12  	mov	w0, #-2
    2c08: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2c0c: ff c3 02 91  	add	sp, sp, #176
    2c10: c0 03 5f d6  	ret

0000000000002c14 <gen_matmul_int8>:
    2c14: ff c3 02 d1  	sub	sp, sp, #176
    2c18: 29 20 80 52  	mov	w9, #257
    2c1c: 2b 00 80 52  	mov	w11, #1
    2c20: 29 00 a0 72  	movk	w9, #1, lsl #16
    2c24: 08 00 40 79  	ldrh	w8, [x0]
    2c28: 2c 20 80 52  	mov	w12, #257
    2c2c: 0a 04 40 79  	ldrh	w10, [x0, #2]
    2c30: eb a3 00 79  	strh	w11, [sp, #80]
    2c34: e9 4b 00 b9  	str	w9, [sp, #72]
    2c38: 09 08 40 79  	ldrh	w9, [x0, #4]
    2c3c: 0b 05 00 11  	add	w11, w8, #1
    2c40: ec c3 00 79  	strh	w12, [sp, #96]
    2c44: 4c 7d 08 1b  	mul	w12, w10, w8
    2c48: fd 7b 0a a9  	stp	x29, x30, [sp, #160]
    2c4c: 2d 7d 0a 1b  	mul	w13, w9, w10
    2c50: fd 83 02 91  	add	x29, sp, #160
    2c54: eb 8f 00 79  	strh	w11, [sp, #70]
    2c58: 9f 39 00 72  	tst	w12, #0x7fff
    2c5c: 8b 7d 0f 53  	lsr	w11, w12, #15
    2c60: ff 13 04 b8  	stur	wzr, [sp, #65]
    2c64: 6b 05 8b 1a  	cinc	w11, w11, ne
    2c68: e8 9b 00 79  	strh	w8, [sp, #76]
    2c6c: 7f 2d 00 71  	cmp	w11, #11
    2c70: ea 9f 00 79  	strh	w10, [sp, #78]
    2c74: e8 33 01 79  	strh	w8, [sp, #152]
    2c78: e9 c7 00 79  	strh	w9, [sp, #98]
    2c7c: ed 2b 0b 29  	stp	w13, w10, [sp, #88]
    2c80: e8 57 00 b9  	str	w8, [sp, #84]
    2c84: a9 00 00 54  	b.ls	0x2c98 <gen_matmul_int8+0x84>
    2c88: 00 00 80 12  	mov	w0, #-1
    2c8c: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2c90: ff c3 02 91  	add	sp, sp, #176
    2c94: c0 03 5f d6  	ret
    2c98: 5f 21 40 71  	cmp	w10, #8, lsl #12        // =32768
    2c9c: e8 08 00 54  	b.hi	0x2db8 <gen_matmul_int8+0x1a4>
    2ca0: 8c 01 80 52  	mov	w12, #12
    2ca4: 2e 20 80 52  	mov	w14, #257
    2ca8: 8c 01 0b 4b  	sub	w12, w12, w11
    2cac: 4d 7d 06 53  	lsr	w13, w10, #6
    2cb0: 5f 15 00 72  	tst	w10, #0x3f
    2cb4: eb 97 01 39  	strb	w11, [sp, #101]
    2cb8: ee d3 00 79  	strh	w14, [sp, #104]
    2cbc: 0e 35 1e 12  	and	w14, w8, #0xfffc
    2cc0: ec 93 01 39  	strb	w12, [sp, #100]
    2cc4: ab 05 8d 1a  	cinc	w11, w13, ne
    2cc8: 0c 3c 41 29  	ldp	w12, w15, [x0, #8]
    2ccc: ed e1 a1 d2  	mov	x13, #252641280
    2cd0: 20 84 00 0f  	movi	v0.4h, #1
    2cd4: 8d 00 c0 f2  	movk	x13, #4, lsl #32
    2cd8: eb cf 00 79  	strh	w11, [sp, #102]
    2cdc: 2b 00 80 52  	mov	w11, #1
    2ce0: e8 1f 01 79  	strh	w8, [sp, #142]
    2ce4: ec 7f 00 b9  	str	w12, [sp, #124]
    2ce8: cc 11 00 51  	sub	w12, w14, #4
    2cec: ed 43 00 f9  	str	x13, [sp, #128]
    2cf0: 8d 00 80 52  	mov	w13, #4
    2cf4: eb ab 01 39  	strb	w11, [sp, #106]
    2cf8: 29 05 00 51  	sub	w9, w9, #1
    2cfc: 8c 7d 1f 33  	bfxil	w12, w12, #31, #1
    2d00: eb 1b 01 79  	strh	w11, [sp, #140]
    2d04: ed 0b 00 79  	strh	w13, [sp, #4]
    2d08: ed c3 00 b2  	mov	x13, #72340172838076673
    2d0c: ea 23 01 79  	strh	w10, [sp, #144]
    2d10: 0a 05 00 51  	sub	w10, w8, #1
    2d14: ec 8b 00 b9  	str	w12, [sp, #136]
    2d18: ec 01 80 52  	mov	w12, #15
    2d1c: 4c 00 a0 72  	movk	w12, #2, lsl #16
    2d20: ed 43 02 f8  	stur	x13, [sp, #36]
    2d24: ed e0 80 52  	mov	w13, #1799
    2d28: eb 6b 00 39  	strb	w11, [sp, #26]
    2d2c: eb 83 00 32  	mov	w11, #65537
    2d30: e1 03 01 91  	add	x1, sp, #64
    2d34: ec 03 00 b9  	str	w12, [sp]
    2d38: ec c3 00 32  	mov	w12, #16843009
    2d3c: e2 e3 00 91  	add	x2, sp, #56
    2d40: e3 03 00 91  	mov	x3, sp
    2d44: e0 c3 06 fc  	stur	d0, [sp, #108]
    2d48: ec 63 01 b8  	stur	w12, [sp, #22]
    2d4c: 0c 10 40 b9  	ldr	w12, [x0, #16]
    2d50: 00 0c 40 f9  	ldr	x0, [x0, #24]
    2d54: ff d3 01 39  	strb	wzr, [sp, #116]
    2d58: ff 63 07 b8  	stur	wzr, [sp, #118]
    2d5c: ec 23 01 29  	stp	w12, w8, [sp, #8]
    2d60: ec 00 80 52  	mov	w12, #7
    2d64: 08 71 1d 53  	lsl	w8, w8, #3
    2d68: ef 97 00 b9  	str	w15, [sp, #148]
    2d6c: ff 73 00 79  	strh	wzr, [sp, #56]
    2d70: ea 77 00 79  	strh	w10, [sp, #58]
    2d74: ff 7b 00 79  	strh	wzr, [sp, #60]
    2d78: e9 7f 00 79  	strh	w9, [sp, #62]
    2d7c: ff 1b 00 39  	strb	wzr, [sp, #6]
    2d80: ff 23 00 79  	strh	wzr, [sp, #16]
    2d84: ed b3 01 78  	sturh	w13, [sp, #27]
    2d88: ea 27 00 79  	strh	w10, [sp, #18]
    2d8c: e9 2b 00 79  	strh	w9, [sp, #20]
    2d90: ec 77 00 39  	strb	w12, [sp, #29]
    2d94: ff 47 00 79  	strh	wzr, [sp, #34]
    2d98: ea 43 00 79  	strh	w10, [sp, #32]
    2d9c: e9 3f 00 79  	strh	w9, [sp, #30]
    2da0: eb a3 05 29  	stp	w11, w8, [sp, #44]
    2da4: 5d fd ff 97  	bl	0x2318 <gen_matmul_task>
    2da8: e0 03 1f 2a  	mov	w0, wzr
    2dac: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2db0: ff c3 02 91  	add	sp, sp, #176
    2db4: c0 03 5f d6  	ret
    2db8: 20 00 80 12  	mov	w0, #-2
    2dbc: fd 7b 4a a9  	ldp	x29, x30, [sp, #160]
    2dc0: ff c3 02 91  	add	sp, sp, #176
    2dc4: c0 03 5f d6  	ret

0000000000002dc8 <feature_data>:
    2dc8: 88 04 00 51  	sub	w8, w4, #1
    2dcc: 09 0d c3 1a  	sdiv	w9, w8, w3
    2dd0: 2a 15 01 1b  	madd	w10, w9, w1, w5
    2dd4: 28 a1 03 1b  	msub	w8, w9, w3, w8
    2dd8: 4a 05 00 51  	sub	w10, w10, #1
    2ddc: 4a 19 02 1b  	madd	w10, w10, w2, w6
    2de0: 49 05 00 51  	sub	w9, w10, #1
    2de4: 20 21 03 1b  	madd	w0, w9, w3, w8
    2de8: c0 03 5f d6  	ret

0000000000002dec <weight_fp16>:
    2dec: 28 04 00 51  	sub	w8, w1, #1
    2df0: 4a 04 00 51  	sub	w10, w2, #1
    2df4: 09 3d 00 11  	add	w9, w8, #15
    2df8: 1f 01 00 71  	cmp	w8, #0
    2dfc: 29 b1 88 1a  	csel	w9, w9, w8, lt
    2e00: 4b 7d 00 11  	add	w11, w10, #31
    2e04: 2c 7d 04 53  	lsr	w12, w9, #4
    2e08: 5f 01 00 71  	cmp	w10, #0
    2e0c: 6b b1 8a 1a  	csel	w11, w11, w10, lt
    2e10: 29 59 1c 12  	and	w9, w9, #0x7fffff0
    2e14: 0c 7c 0c 1b  	mul	w12, w0, w12
    2e18: 6d 69 1b 12  	and	w13, w11, #0xffffffe0
    2e1c: 08 01 09 4b  	sub	w8, w8, w9
    2e20: 49 01 0d 4b  	sub	w9, w10, w13
    2e24: 6a 6d 1c 53  	lsl	w10, w11, #4
    2e28: 28 15 08 0b  	add	w8, w9, w8, lsl #5
    2e2c: 49 59 17 12  	and	w9, w10, #0xfffffe00
    2e30: 08 11 0c 0b  	add	w8, w8, w12, lsl #4
    2e34: 00 01 09 0b  	add	w0, w8, w9
    2e38: c0 03 5f d6  	ret

0000000000002e3c <weight_int8>:
    2e3c: 28 04 00 51  	sub	w8, w1, #1
    2e40: 4a 04 00 51  	sub	w10, w2, #1
    2e44: 09 7d 00 11  	add	w9, w8, #31
    2e48: 1f 01 00 71  	cmp	w8, #0
    2e4c: 29 b1 88 1a  	csel	w9, w9, w8, lt
    2e50: 4b 7d 00 11  	add	w11, w10, #31
    2e54: 2c 7d 05 53  	lsr	w12, w9, #5
    2e58: 5f 01 00 71  	cmp	w10, #0
    2e5c: 29 55 1b 12  	and	w9, w9, #0x7ffffe0
    2e60: 6b b1 8a 1a  	csel	w11, w11, w10, lt
    2e64: 08 01 09 4b  	sub	w8, w8, w9
    2e68: 6d 69 1b 53  	lsl	w13, w11, #5
    2e6c: 69 69 1b 12  	and	w9, w11, #0xffffffe0
    2e70: 88 21 00 1b  	madd	w8, w12, w0, w8
    2e74: ab 55 16 12  	and	w11, w13, #0xfffffc00
    2e78: 49 01 09 4b  	sub	w9, w10, w9
    2e7c: 69 01 09 0b  	add	w9, w11, w9
    2e80: 20 15 08 0b  	add	w0, w9, w8, lsl #5
    2e84: c0 03 5f d6  	ret

0000000000002e88 <rknpu_init>:
    2e88: fd 7b bb a9  	stp	x29, x30, [sp, #-80]!
    2e8c: f4 4f 04 a9  	stp	x20, x19, [sp, #64]
    2e90: f3 03 00 aa  	mov	x19, x0
    2e94: 00 04 80 52  	mov	w0, #32
    2e98: fa 67 01 a9  	stp	x26, x25, [sp, #16]
    2e9c: f8 5f 02 a9  	stp	x24, x23, [sp, #32]
    2ea0: fd 03 00 91  	mov	x29, sp
    2ea4: f6 57 03 a9  	stp	x22, x21, [sp, #48]
    2ea8: f5 03 02 aa  	mov	x21, x2
    2eac: f6 03 01 aa  	mov	x22, x1
    2eb0: 90 03 00 94  	bl	0x3cf0 <malloc@plt>
    2eb4: 80 1c 00 b4  	cbz	x0, 0x3244 <rknpu_init+0x3bc>
    2eb8: f7 ff ff d0  	adrp	x23, 0x0 <rknpu_init+0x28>
    2ebc: f7 e2 2b 91  	add	x23, x23, #2808
    2ec0: f8 ff ff d0  	adrp	x24, 0x0 <rknpu_init+0x30>
    2ec4: 18 5f 31 91  	add	x24, x24, #3159
    2ec8: f4 03 00 aa  	mov	x20, x0
    2ecc: e0 03 17 aa  	mov	x0, x23
    2ed0: e1 03 18 aa  	mov	x1, x24
    2ed4: 02 0d 80 52  	mov	w2, #104
    2ed8: 96 03 00 94  	bl	0x3d30 <printf@plt>
    2edc: e0 03 17 aa  	mov	x0, x23
    2ee0: e1 03 18 aa  	mov	x1, x24
    2ee4: 62 0d 80 52  	mov	w2, #107
    2ee8: 92 03 00 94  	bl	0x3d30 <printf@plt>
    2eec: c0 02 40 f9  	ldr	x0, [x22]
    2ef0: 21 00 a0 52  	mov	w1, #65536
    2ef4: 93 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    2ef8: 60 16 f8 37  	tbnz	w0, #31, 0x31c4 <rknpu_init+0x33c>
    2efc: 21 00 a0 52  	mov	w1, #65536
    2f00: 62 00 80 52  	mov	w2, #3
    2f04: 93 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    2f08: e0 16 00 b4  	cbz	x0, 0x31e4 <rknpu_init+0x35c>
    2f0c: f7 03 00 aa  	mov	x23, x0
    2f10: e0 ff ff d0  	adrp	x0, 0x0 <rknpu_init+0x80>
    2f14: 00 e0 2b 91  	add	x0, x0, #2808
    2f18: e1 ff ff d0  	adrp	x1, 0x0 <rknpu_init+0x88>
    2f1c: 21 5c 31 91  	add	x1, x1, #3159
    2f20: 62 0e 80 52  	mov	w2, #115
    2f24: 83 03 00 94  	bl	0x3d30 <printf@plt>
    2f28: a0 02 40 f9  	ldr	x0, [x21]
    2f2c: 01 20 80 52  	mov	w1, #256
    2f30: 97 02 00 f9  	str	x23, [x20]
    2f34: 83 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    2f38: 60 16 f8 37  	tbnz	w0, #31, 0x3204 <rknpu_init+0x37c>
    2f3c: 21 00 a0 52  	mov	w1, #65536
    2f40: 62 00 80 52  	mov	w2, #3
    2f44: f7 03 00 2a  	mov	w23, w0
    2f48: 82 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    2f4c: c0 16 00 b4  	cbz	x0, 0x3224 <rknpu_init+0x39c>
    2f50: f9 ff ff d0  	adrp	x25, 0x0 <rknpu_init+0xc0>
    2f54: 39 e3 2b 91  	add	x25, x25, #2808
    2f58: fa ff ff d0  	adrp	x26, 0x0 <rknpu_init+0xc8>
    2f5c: 5a 5f 31 91  	add	x26, x26, #3159
    2f60: f8 03 00 aa  	mov	x24, x0
    2f64: e0 03 19 aa  	mov	x0, x25
    2f68: e1 03 1a aa  	mov	x1, x26
    2f6c: 82 0f 80 52  	mov	w2, #124
    2f70: 70 03 00 94  	bl	0x3d30 <printf@plt>
    2f74: e0 03 19 aa  	mov	x0, x25
    2f78: e1 03 1a aa  	mov	x1, x26
    2f7c: e2 0f 80 52  	mov	w2, #127
    2f80: 6c 03 00 94  	bl	0x3d30 <printf@plt>
    2f84: e0 03 19 aa  	mov	x0, x25
    2f88: e1 03 1a aa  	mov	x1, x26
    2f8c: 42 10 80 52  	mov	w2, #130
    2f90: 68 03 00 94  	bl	0x3d30 <printf@plt>
    2f94: e0 03 17 2a  	mov	w0, w23
    2f98: e1 03 18 aa  	mov	x1, x24
    2f9c: 02 20 80 52  	mov	w2, #256
    2fa0: 70 03 00 94  	bl	0x3d60 <chcore_auto_unmap_pmo@plt>
    2fa4: e0 03 17 2a  	mov	w0, w23
    2fa8: 21 00 80 52  	mov	w1, #1
    2fac: 71 03 00 94  	bl	0x3d70 <usys_revoke_cap@plt>
    2fb0: e0 03 19 aa  	mov	x0, x25
    2fb4: e1 03 1a aa  	mov	x1, x26
    2fb8: a2 10 80 52  	mov	w2, #133
    2fbc: 5d 03 00 94  	bl	0x3d30 <printf@plt>
    2fc0: e0 03 19 aa  	mov	x0, x25
    2fc4: e1 03 1a aa  	mov	x1, x26
    2fc8: 62 0d 80 52  	mov	w2, #107
    2fcc: 59 03 00 94  	bl	0x3d30 <printf@plt>
    2fd0: c0 06 40 f9  	ldr	x0, [x22, #8]
    2fd4: 21 00 a0 52  	mov	w1, #65536
    2fd8: 5a 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    2fdc: 40 0f f8 37  	tbnz	w0, #31, 0x31c4 <rknpu_init+0x33c>
    2fe0: 21 00 a0 52  	mov	w1, #65536
    2fe4: 62 00 80 52  	mov	w2, #3
    2fe8: 5a 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    2fec: c0 0f 00 b4  	cbz	x0, 0x31e4 <rknpu_init+0x35c>
    2ff0: f7 03 00 aa  	mov	x23, x0
    2ff4: e0 ff ff d0  	adrp	x0, 0x0 <rknpu_init+0x164>
    2ff8: 00 e0 2b 91  	add	x0, x0, #2808
    2ffc: e1 ff ff d0  	adrp	x1, 0x0 <rknpu_init+0x16c>
    3000: 21 5c 31 91  	add	x1, x1, #3159
    3004: 62 0e 80 52  	mov	w2, #115
    3008: 4a 03 00 94  	bl	0x3d30 <printf@plt>
    300c: a0 06 40 f9  	ldr	x0, [x21, #8]
    3010: 01 20 80 52  	mov	w1, #256
    3014: 97 06 00 f9  	str	x23, [x20, #8]
    3018: 4a 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    301c: 40 0f f8 37  	tbnz	w0, #31, 0x3204 <rknpu_init+0x37c>
    3020: 21 00 a0 52  	mov	w1, #65536
    3024: 62 00 80 52  	mov	w2, #3
    3028: f7 03 00 2a  	mov	w23, w0
    302c: 49 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    3030: a0 0f 00 b4  	cbz	x0, 0x3224 <rknpu_init+0x39c>
    3034: f9 ff ff b0  	adrp	x25, 0x0 <rknpu_init+0x1a0>
    3038: 39 e3 2b 91  	add	x25, x25, #2808
    303c: fa ff ff b0  	adrp	x26, 0x0 <rknpu_init+0x1a8>
    3040: 5a 5f 31 91  	add	x26, x26, #3159
    3044: f8 03 00 aa  	mov	x24, x0
    3048: e0 03 19 aa  	mov	x0, x25
    304c: e1 03 1a aa  	mov	x1, x26
    3050: 82 0f 80 52  	mov	w2, #124
    3054: 37 03 00 94  	bl	0x3d30 <printf@plt>
    3058: e0 03 19 aa  	mov	x0, x25
    305c: e1 03 1a aa  	mov	x1, x26
    3060: e2 0f 80 52  	mov	w2, #127
    3064: 33 03 00 94  	bl	0x3d30 <printf@plt>
    3068: e0 03 19 aa  	mov	x0, x25
    306c: e1 03 1a aa  	mov	x1, x26
    3070: 42 10 80 52  	mov	w2, #130
    3074: 2f 03 00 94  	bl	0x3d30 <printf@plt>
    3078: e0 03 17 2a  	mov	w0, w23
    307c: e1 03 18 aa  	mov	x1, x24
    3080: 02 20 80 52  	mov	w2, #256
    3084: 37 03 00 94  	bl	0x3d60 <chcore_auto_unmap_pmo@plt>
    3088: e0 03 17 2a  	mov	w0, w23
    308c: 21 00 80 52  	mov	w1, #1
    3090: 38 03 00 94  	bl	0x3d70 <usys_revoke_cap@plt>
    3094: e0 03 19 aa  	mov	x0, x25
    3098: e1 03 1a aa  	mov	x1, x26
    309c: a2 10 80 52  	mov	w2, #133
    30a0: 24 03 00 94  	bl	0x3d30 <printf@plt>
    30a4: e0 03 19 aa  	mov	x0, x25
    30a8: e1 03 1a aa  	mov	x1, x26
    30ac: 62 0d 80 52  	mov	w2, #107
    30b0: 20 03 00 94  	bl	0x3d30 <printf@plt>
    30b4: c0 0a 40 f9  	ldr	x0, [x22, #16]
    30b8: 21 00 a0 52  	mov	w1, #65536
    30bc: 21 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    30c0: 20 08 f8 37  	tbnz	w0, #31, 0x31c4 <rknpu_init+0x33c>
    30c4: 21 00 a0 52  	mov	w1, #65536
    30c8: 62 00 80 52  	mov	w2, #3
    30cc: 21 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    30d0: a0 08 00 b4  	cbz	x0, 0x31e4 <rknpu_init+0x35c>
    30d4: f6 03 00 aa  	mov	x22, x0
    30d8: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_init+0x244>
    30dc: 00 e0 2b 91  	add	x0, x0, #2808
    30e0: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x24c>
    30e4: 21 5c 31 91  	add	x1, x1, #3159
    30e8: 62 0e 80 52  	mov	w2, #115
    30ec: 11 03 00 94  	bl	0x3d30 <printf@plt>
    30f0: a0 0a 40 f9  	ldr	x0, [x21, #16]
    30f4: 01 20 80 52  	mov	w1, #256
    30f8: 96 0a 00 f9  	str	x22, [x20, #16]
    30fc: 11 03 00 94  	bl	0x3d40 <usys_create_device_pmo@plt>
    3100: 20 08 f8 37  	tbnz	w0, #31, 0x3204 <rknpu_init+0x37c>
    3104: 21 00 a0 52  	mov	w1, #65536
    3108: 62 00 80 52  	mov	w2, #3
    310c: f5 03 00 2a  	mov	w21, w0
    3110: 10 03 00 94  	bl	0x3d50 <chcore_auto_map_pmo@plt>
    3114: 80 08 00 b4  	cbz	x0, 0x3224 <rknpu_init+0x39c>
    3118: f7 ff ff b0  	adrp	x23, 0x0 <rknpu_init+0x284>
    311c: f7 e2 2b 91  	add	x23, x23, #2808
    3120: f8 ff ff b0  	adrp	x24, 0x0 <rknpu_init+0x28c>
    3124: 18 5f 31 91  	add	x24, x24, #3159
    3128: f6 03 00 aa  	mov	x22, x0
    312c: e0 03 17 aa  	mov	x0, x23
    3130: e1 03 18 aa  	mov	x1, x24
    3134: 82 0f 80 52  	mov	w2, #124
    3138: fe 02 00 94  	bl	0x3d30 <printf@plt>
    313c: e0 03 17 aa  	mov	x0, x23
    3140: e1 03 18 aa  	mov	x1, x24
    3144: e2 0f 80 52  	mov	w2, #127
    3148: fa 02 00 94  	bl	0x3d30 <printf@plt>
    314c: e0 03 17 aa  	mov	x0, x23
    3150: e1 03 18 aa  	mov	x1, x24
    3154: 42 10 80 52  	mov	w2, #130
    3158: f6 02 00 94  	bl	0x3d30 <printf@plt>
    315c: e0 03 15 2a  	mov	w0, w21
    3160: e1 03 16 aa  	mov	x1, x22
    3164: 02 20 80 52  	mov	w2, #256
    3168: fe 02 00 94  	bl	0x3d60 <chcore_auto_unmap_pmo@plt>
    316c: e0 03 15 2a  	mov	w0, w21
    3170: 21 00 80 52  	mov	w1, #1
    3174: ff 02 00 94  	bl	0x3d70 <usys_revoke_cap@plt>
    3178: e0 03 17 aa  	mov	x0, x23
    317c: e1 03 18 aa  	mov	x1, x24
    3180: a2 10 80 52  	mov	w2, #133
    3184: eb 02 00 94  	bl	0x3d30 <printf@plt>
    3188: e0 03 17 aa  	mov	x0, x23
    318c: e1 03 18 aa  	mov	x1, x24
    3190: 02 11 80 52  	mov	w2, #136
    3194: e7 02 00 94  	bl	0x3d30 <printf@plt>
    3198: 1f 20 03 d5  	nop
    319c: a8 f4 fe 10  	adr	x8, #-8556
    31a0: 74 02 00 f9  	str	x20, [x19]
    31a4: e0 03 1f 2a  	mov	w0, wzr
    31a8: f6 57 43 a9  	ldp	x22, x21, [sp, #48]
    31ac: 88 0e 00 f9  	str	x8, [x20, #24]
    31b0: f4 4f 44 a9  	ldp	x20, x19, [sp, #64]
    31b4: f8 5f 42 a9  	ldp	x24, x23, [sp, #32]
    31b8: fa 67 41 a9  	ldp	x26, x25, [sp, #16]
    31bc: fd 7b c5 a8  	ldp	x29, x30, [sp], #80
    31c0: c0 03 5f d6  	ret
    31c4: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_init+0x330>
    31c8: 00 08 2b 91  	add	x0, x0, #2754
    31cc: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x338>
    31d0: 21 bc 2c 91  	add	x1, x1, #2863
    31d4: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_init+0x340>
    31d8: 63 5c 31 91  	add	x3, x3, #3159
    31dc: c2 0d 80 52  	mov	w2, #110
    31e0: cc 02 00 94  	bl	0x3d10 <__assert_fail@plt>
    31e4: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_init+0x350>
    31e8: 00 e0 32 91  	add	x0, x0, #3256
    31ec: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x358>
    31f0: 21 bc 2c 91  	add	x1, x1, #2863
    31f4: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_init+0x360>
    31f8: 63 5c 31 91  	add	x3, x3, #3159
    31fc: 22 0e 80 52  	mov	w2, #113
    3200: c4 02 00 94  	bl	0x3d10 <__assert_fail@plt>
    3204: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_init+0x370>
    3208: 00 08 2b 91  	add	x0, x0, #2754
    320c: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x378>
    3210: 21 bc 2c 91  	add	x1, x1, #2863
    3214: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_init+0x380>
    3218: 63 5c 31 91  	add	x3, x3, #3159
    321c: 02 0f 80 52  	mov	w2, #120
    3220: bc 02 00 94  	bl	0x3d10 <__assert_fail@plt>
    3224: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_init+0x390>
    3228: 00 e0 32 91  	add	x0, x0, #3256
    322c: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x398>
    3230: 21 bc 2c 91  	add	x1, x1, #2863
    3234: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_init+0x3a0>
    3238: 63 5c 31 91  	add	x3, x3, #3159
    323c: 62 0f 80 52  	mov	w2, #123
    3240: b4 02 00 94  	bl	0x3d10 <__assert_fail@plt>
    3244: 1f 20 03 d5  	nop
    3248: 40 c3 fe 10  	adr	x0, #-10136
    324c: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_init+0x3b8>
    3250: 21 bc 2c 91  	add	x1, x1, #2863
    3254: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_init+0x3c0>
    3258: 63 5c 31 91  	add	x3, x3, #3159
    325c: c2 0c 80 52  	mov	w2, #102
    3260: ac 02 00 94  	bl	0x3d10 <__assert_fail@plt>

0000000000003264 <rknpu_submit>:
    3264: fd 7b bf a9  	stp	x29, x30, [sp, #-16]!
    3268: 28 38 40 b9  	ldr	w8, [x1, #56]
    326c: fd 03 00 91  	mov	x29, sp
    3270: 1f 05 00 71  	cmp	w8, #1
    3274: e0 00 00 54  	b.eq	0x3290 <rknpu_submit+0x2c>
    3278: 1f 11 00 71  	cmp	w8, #4
    327c: e0 00 00 54  	b.eq	0x3298 <rknpu_submit+0x34>
    3280: 1f 09 00 71  	cmp	w8, #2
    3284: c1 07 00 54  	b.ne	0x337c <rknpu_submit+0x118>
    3288: 29 00 80 52  	mov	w9, #1
    328c: 04 00 00 14  	b	0x329c <rknpu_submit+0x38>
    3290: e9 03 1f 2a  	mov	w9, wzr
    3294: 02 00 00 14  	b	0x329c <rknpu_submit+0x38>
    3298: 49 00 80 52  	mov	w9, #2
    329c: 08 0c 40 f9  	ldr	x8, [x0, #24]
    32a0: 2b 0c 40 b9  	ldr	w11, [x1, #12]
    32a4: 0a 11 40 b9  	ldr	w10, [x8, #16]
    32a8: 7f 01 0a 6b  	cmp	w11, w10
    32ac: 88 05 00 54  	b.hi	0x335c <rknpu_submit+0xf8>
    32b0: ea 03 09 2a  	mov	w10, w9
    32b4: 2b 0c 40 f9  	ldr	x11, [x1, #24]
    32b8: 4a f1 7d d3  	lsl	x10, x10, #3
    32bc: 0e 05 80 52  	mov	w14, #40
    32c0: 2c 00 0a 8b  	add	x12, x1, x10
    32c4: 30 00 40 b9  	ldr	w16, [x1]
    32c8: 6b 01 40 f9  	ldr	x11, [x11]
    32cc: 0a 68 6a f8  	ldr	x10, [x0, x10]
    32d0: 8d 31 48 29  	ldp	w13, w12, [x12, #64]
    32d4: ad 7d 40 93  	sxtw	x13, w13
    32d8: ab 2d 0e 9b  	madd	x11, x13, x14, x11
    32dc: cd 01 80 52  	mov	w13, #14
    32e0: 2d 05 04 33  	bfi	w13, w9, #28, #2
    32e4: 09 39 40 29  	ldp	w9, w14, [x8]
    32e8: 6f 11 40 f9  	ldr	x15, [x11, #32]
    32ec: 4d 05 10 b9  	str	w13, [x10, #4100]
    32f0: 4d 05 30 b9  	str	w13, [x10, #12292]
    32f4: ed ff ff d0  	adrp	x13, 0x1000 <rknpu_submit+0x88>
    32f8: 4f 11 00 b9  	str	w15, [x10, #16]
    32fc: 6b 19 40 b9  	ldr	w11, [x11, #24]
    3300: 5f 09 00 b9  	str	wzr, [x10, #8]
    3304: a0 15 40 fd  	ldr	d0, [x13, #40]
    3308: 2d 14 40 f9  	ldr	x13, [x1, #40]
    330c: 2b 01 0b 0b  	add	w11, w9, w11
    3310: 6b 0d 00 11  	add	w11, w11, #3
    3314: 40 11 00 fd  	str	d0, [x10, #32]
    3318: 69 09 c9 1a  	udiv	w9, w11, w9
    331c: cb 00 80 52  	mov	w11, #6
    3320: 0b 0a 02 33  	bfxil	w11, w16, #2, #1
    3324: 6b 21 ce 1a  	lsl	w11, w11, w14
    3328: 6b 01 0c 2a  	orr	w11, w11, w12
    332c: 29 05 00 51  	sub	w9, w9, #1
    3330: 4b 35 06 29  	stp	w11, w13, [x10, #48]
    3334: 49 15 00 b9  	str	w9, [x10, #20]
    3338: 08 25 41 29  	ldp	w8, w9, [x8, #8]
    333c: 49 69 69 b8  	ldr	w9, [x10, x9]
    3340: 08 01 09 0a  	and	w8, w8, w9
    3344: 1f 01 0c 6b  	cmp	w8, w12
    3348: 4a 00 00 54  	b.ge	0x3350 <rknpu_submit+0xec>
    334c: 00 00 00 14  	b	0x334c <rknpu_submit+0xe8>
    3350: e0 03 1f 2a  	mov	w0, wzr
    3354: fd 7b c1 a8  	ldp	x29, x30, [sp], #16
    3358: c0 03 5f d6  	ret
    335c: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_submit+0xec>
    3360: 00 3c 2b 91  	add	x0, x0, #2767
    3364: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_submit+0xf4>
    3368: 21 bc 2c 91  	add	x1, x1, #2863
    336c: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_submit+0xfc>
    3370: 63 4c 34 91  	add	x3, x3, #3347
    3374: e2 17 80 52  	mov	w2, #191
    3378: 66 02 00 94  	bl	0x3d10 <__assert_fail@plt>
    337c: e0 ff ff b0  	adrp	x0, 0x0 <rknpu_submit+0x10c>
    3380: 00 d8 2b 91  	add	x0, x0, #2806
    3384: e1 ff ff b0  	adrp	x1, 0x0 <rknpu_submit+0x114>
    3388: 21 bc 2c 91  	add	x1, x1, #2863
    338c: e3 ff ff b0  	adrp	x3, 0x0 <rknpu_submit+0x11c>
    3390: 63 88 31 91  	add	x3, x3, #3170
    3394: 82 14 80 52  	mov	w2, #164
    3398: 5e 02 00 94  	bl	0x3d10 <__assert_fail@plt>

000000000000339c <rknpu_soft_reset>:
    339c: e0 03 1f 2a  	mov	w0, wzr
    33a0: c0 03 5f d6  	ret

00000000000033a4 <matmul_fp16>:
    33a4: f4 4f bf a9  	stp	x20, x19, [sp, #-16]!
    33a8: 1f 04 00 71  	cmp	w0, #1
    33ac: eb 0c 00 54  	b.lt	0x3548 <matmul_fp16+0x1a4>
    33b0: 5f 04 00 71  	cmp	w2, #1
    33b4: ab 0c 00 54  	b.lt	0x3548 <matmul_fp16+0x1a4>
    33b8: 3f 00 00 71  	cmp	w1, #0
    33bc: ad 0c 00 54  	b.le	0x3550 <matmul_fp16+0x1ac>
    33c0: ea 03 01 2a  	mov	w10, w1
    33c4: e8 03 1f aa  	mov	x8, xzr
    33c8: e9 03 1f aa  	mov	x9, xzr
    33cc: eb 03 02 2a  	mov	w11, w2
    33d0: ec 03 00 2a  	mov	w12, w0
    33d4: 4d 6d 7c 92  	and	x13, x10, #0xfffffff0
    33d8: 6e 40 00 91  	add	x14, x3, #16
    33dc: 4f f9 7f d3  	lsl	x15, x10, #1
    33e0: 90 40 00 91  	add	x16, x4, #16
    33e4: 06 00 00 14  	b	0x33fc <matmul_fp16+0x58>
    33e8: 29 05 00 91  	add	x9, x9, #1
    33ec: ce 01 0f 8b  	add	x14, x14, x15
    33f0: 08 01 0a 8b  	add	x8, x8, x10
    33f4: 3f 01 0c eb  	cmp	x9, x12
    33f8: 80 0a 00 54  	b.eq	0x3548 <matmul_fp16+0x1a4>
    33fc: 22 7d 0b 9b  	mul	x2, x9, x11
    3400: f1 03 1f aa  	mov	x17, xzr
    3404: e0 03 1f aa  	mov	x0, xzr
    3408: e6 03 10 aa  	mov	x6, x16
    340c: 09 00 00 14  	b	0x3430 <matmul_fp16+0x8c>
    3410: 00 c0 23 1e  	fcvt	h0, s0
    3414: 07 00 02 8b  	add	x7, x0, x2
    3418: 00 04 00 91  	add	x0, x0, #1
    341c: c6 00 0f 8b  	add	x6, x6, x15
    3420: 31 02 0a 8b  	add	x17, x17, x10
    3424: 1f 00 0b eb  	cmp	x0, x11
    3428: a0 78 27 7c  	str	h0, [x5, x7, lsl #1]
    342c: e0 fd ff 54  	b.eq	0x33e8 <matmul_fp16+0x44>
    3430: 00 e4 00 2f  	movi	d0, #0000000000000000
    3434: 3f 40 00 71  	cmp	w1, #16
    3438: 62 00 00 54  	b.hs	0x3444 <matmul_fp16+0xa0>
    343c: f4 03 1f aa  	mov	x20, xzr
    3440: 35 00 00 14  	b	0x3514 <matmul_fp16+0x170>
    3444: e7 03 0d aa  	mov	x7, x13
    3448: f3 03 06 aa  	mov	x19, x6
    344c: f4 03 0e aa  	mov	x20, x14
    3450: 81 02 df 3c  	ldur	q1, [x20, #-16]
    3454: e7 40 00 f1  	subs	x7, x7, #16
    3458: 62 02 df 3c  	ldur	q2, [x19, #-16]
    345c: 23 78 21 0e  	fcvtl	v3.4s, v1.4h
    3460: 21 78 21 4e  	fcvtl2	v1.4s, v1.8h
    3464: 44 78 21 0e  	fcvtl	v4.4s, v2.4h
    3468: 42 78 21 4e  	fcvtl2	v2.4s, v2.8h
    346c: 63 dc 24 6e  	fmul	v3.4s, v3.4s, v4.4s
    3470: 21 dc 22 6e  	fmul	v1.4s, v1.4s, v2.4s
    3474: 64 04 0c 5e  	mov	s4, v3.s[1]
    3478: 00 28 23 1e  	fadd	s0, s0, s3
    347c: 65 04 14 5e  	mov	s5, v3.s[2]
    3480: 63 04 1c 5e  	mov	s3, v3.s[3]
    3484: 22 04 0c 5e  	mov	s2, v1.s[1]
    3488: 00 28 24 1e  	fadd	s0, s0, s4
    348c: 64 06 c2 3c  	ldr	q4, [x19], #32
    3490: 00 28 25 1e  	fadd	s0, s0, s5
    3494: 25 04 14 5e  	mov	s5, v1.s[2]
    3498: 86 78 21 0e  	fcvtl	v6.4s, v4.4h
    349c: 00 28 23 1e  	fadd	s0, s0, s3
    34a0: 83 06 c2 3c  	ldr	q3, [x20], #32
    34a4: 00 28 21 1e  	fadd	s0, s0, s1
    34a8: 21 04 1c 5e  	mov	s1, v1.s[3]
    34ac: 00 28 22 1e  	fadd	s0, s0, s2
    34b0: 62 78 21 0e  	fcvtl	v2.4s, v3.4h
    34b4: 00 28 25 1e  	fadd	s0, s0, s5
    34b8: 42 dc 26 6e  	fmul	v2.4s, v2.4s, v6.4s
    34bc: 00 28 21 1e  	fadd	s0, s0, s1
    34c0: 41 04 0c 5e  	mov	s1, v2.s[1]
    34c4: 45 04 14 5e  	mov	s5, v2.s[2]
    34c8: 00 28 22 1e  	fadd	s0, s0, s2
    34cc: 42 04 1c 5e  	mov	s2, v2.s[3]
    34d0: 00 28 21 1e  	fadd	s0, s0, s1
    34d4: 61 78 21 4e  	fcvtl2	v1.4s, v3.8h
    34d8: 83 78 21 4e  	fcvtl2	v3.4s, v4.8h
    34dc: 00 28 25 1e  	fadd	s0, s0, s5
    34e0: 21 dc 23 6e  	fmul	v1.4s, v1.4s, v3.4s
    34e4: 00 28 22 1e  	fadd	s0, s0, s2
    34e8: 22 04 0c 5e  	mov	s2, v1.s[1]
    34ec: 23 04 14 5e  	mov	s3, v1.s[2]
    34f0: 00 28 21 1e  	fadd	s0, s0, s1
    34f4: 21 04 1c 5e  	mov	s1, v1.s[3]
    34f8: 00 28 22 1e  	fadd	s0, s0, s2
    34fc: 00 28 23 1e  	fadd	s0, s0, s3
    3500: 00 28 21 1e  	fadd	s0, s0, s1
    3504: 61 fa ff 54  	b.ne	0x3450 <matmul_fp16+0xac>
    3508: f4 03 0d aa  	mov	x20, x13
    350c: bf 01 0a eb  	cmp	x13, x10
    3510: 00 f8 ff 54  	b.eq	0x3410 <matmul_fp16+0x6c>
    3514: 87 02 11 8b  	add	x7, x20, x17
    3518: 13 01 14 8b  	add	x19, x8, x20
    351c: 54 01 14 cb  	sub	x20, x10, x20
    3520: 87 04 07 8b  	add	x7, x4, x7, lsl #1
    3524: 73 04 13 8b  	add	x19, x3, x19, lsl #1
    3528: 61 26 40 7c  	ldr	h1, [x19], #2
    352c: e2 24 40 7c  	ldr	h2, [x7], #2
    3530: 94 06 00 f1  	subs	x20, x20, #1
    3534: 21 40 e2 1e  	fcvt	s1, h1
    3538: 42 40 e2 1e  	fcvt	s2, h2
    353c: 20 00 02 1f  	fmadd	s0, s1, s2, s0
    3540: 41 ff ff 54  	b.ne	0x3528 <matmul_fp16+0x184>
    3544: b3 ff ff 17  	b	0x3410 <matmul_fp16+0x6c>
    3548: f4 4f c1 a8  	ldp	x20, x19, [sp], #16
    354c: c0 03 5f d6  	ret
    3550: 48 7c a0 9b  	umull	x8, w2, w0
    3554: e0 03 05 aa  	mov	x0, x5
    3558: e1 03 1f 2a  	mov	w1, wzr
    355c: 02 f9 7f d3  	lsl	x2, x8, #1
    3560: f4 4f c1 a8  	ldp	x20, x19, [sp], #16
    3564: 07 02 00 14  	b	0x3d80 <memset@plt>

0000000000003568 <rand_float>:
    3568: fd 7b bf a9  	stp	x29, x30, [sp, #-16]!
    356c: fd 03 00 91  	mov	x29, sp
    3570: 08 02 00 94  	bl	0x3d90 <rand@plt>
    3574: 00 66 01 0f  	movi	v0.2s, #48, lsl #24
    3578: 01 00 22 1e  	scvtf	s1, w0
    357c: 20 08 20 1e  	fmul	s0, s1, s0
    3580: fd 7b c1 a8  	ldp	x29, x30, [sp], #16
    3584: c0 03 5f d6  	ret

0000000000003588 <main>:
    3588: ff 03 07 d1  	sub	sp, sp, #448
    358c: f4 4f 1b a9  	stp	x20, x19, [sp, #432]
    3590: f3 03 01 aa  	mov	x19, x1
    3594: f4 03 00 2a  	mov	w20, w0
    3598: 1f 20 03 d5  	nop
    359c: e0 b6 fe 50  	adr	x0, #-10530
    35a0: e1 ff ff b0  	adrp	x1, 0x0 <main+0xc>
    35a4: 21 80 34 91  	add	x1, x1, #3360
    35a8: 82 0a 80 52  	mov	w2, #84
    35ac: e9 23 15 6d  	stp	d9, d8, [sp, #336]
    35b0: fd 7b 16 a9  	stp	x29, x30, [sp, #352]
    35b4: fd 83 05 91  	add	x29, sp, #352
    35b8: fc 6f 17 a9  	stp	x28, x27, [sp, #368]
    35bc: fa 67 18 a9  	stp	x26, x25, [sp, #384]
    35c0: f8 5f 19 a9  	stp	x24, x23, [sp, #400]
    35c4: f6 57 1a a9  	stp	x22, x21, [sp, #416]
    35c8: da 01 00 94  	bl	0x3d30 <printf@plt>
    35cc: 9f 12 00 71  	cmp	w20, #4
    35d0: 21 04 00 54  	b.ne	0x3654 <main+0xcc>
    35d4: 60 06 40 f9  	ldr	x0, [x19, #8]
    35d8: f2 01 00 94  	bl	0x3da0 <atoi@plt>
    35dc: f4 03 00 2a  	mov	w20, w0
    35e0: 60 0a 40 f9  	ldr	x0, [x19, #16]
    35e4: ef 01 00 94  	bl	0x3da0 <atoi@plt>
    35e8: f5 03 00 2a  	mov	w21, w0
    35ec: 60 0e 40 f9  	ldr	x0, [x19, #24]
    35f0: ec 01 00 94  	bl	0x3da0 <atoi@plt>
    35f4: 74 03 00 34  	cbz	w20, 0x3660 <main+0xd8>
    35f8: 9f 06 00 72  	tst	w20, #0x3
    35fc: 84 1a 41 7a  	ccmp	w20, #1, #4, ne
    3600: e8 07 9f 1a  	cset	w8, ne
    3604: 9f 02 06 71  	cmp	w20, #384
    3608: c8 02 00 54  	b.hi	0x3660 <main+0xd8>
    360c: a8 02 00 35  	cbnz	w8, 0x3660 <main+0xd8>
    3610: a8 06 00 51  	sub	w8, w21, #1
    3614: 08 7d 0d 53  	lsr	w8, w8, #13
    3618: 28 04 00 35  	cbnz	w8, 0x369c <main+0x114>
    361c: a8 12 00 12  	and	w8, w21, #0x1f
    3620: e8 03 00 35  	cbnz	w8, 0x369c <main+0x114>
    3624: e1 03 00 2a  	mov	w1, w0
    3628: 08 00 84 12  	mov	w8, #-8193
    362c: 08 00 08 0b  	add	w8, w0, w8
    3630: 1f 09 40 31  	cmn	w8, #2, lsl #12         // =8192
    3634: a3 00 00 54  	b.lo	0x3648 <main+0xc0>
    3638: 3f 04 00 71  	cmp	w1, #1
    363c: 80 03 00 54  	b.eq	0x36ac <main+0x124>
    3640: 28 0c 00 12  	and	w8, w1, #0xf
    3644: 48 03 00 34  	cbz	w8, 0x36ac <main+0x124>
    3648: e0 ff ff b0  	adrp	x0, 0x0 <main+0xb4>
    364c: 00 54 2f 91  	add	x0, x0, #3029
    3650: 07 00 00 14  	b	0x366c <main+0xe4>
    3654: e0 ff ff b0  	adrp	x0, 0x0 <main+0xc0>
    3658: 00 14 30 91  	add	x0, x0, #3077
    365c: 03 00 00 14  	b	0x3668 <main+0xe0>
    3660: e0 ff ff b0  	adrp	x0, 0x0 <main+0xcc>
    3664: 00 fc 2b 91  	add	x0, x0, #2815
    3668: e1 03 14 2a  	mov	w1, w20
    366c: b1 01 00 94  	bl	0x3d30 <printf@plt>
    3670: 1a 00 80 12  	mov	w26, #-1
    3674: e0 03 1a 2a  	mov	w0, w26
    3678: f4 4f 5b a9  	ldp	x20, x19, [sp, #432]
    367c: f6 57 5a a9  	ldp	x22, x21, [sp, #416]
    3680: f8 5f 59 a9  	ldp	x24, x23, [sp, #400]
    3684: fa 67 58 a9  	ldp	x26, x25, [sp, #384]
    3688: fc 6f 57 a9  	ldp	x28, x27, [sp, #368]
    368c: fd 7b 56 a9  	ldp	x29, x30, [sp, #352]
    3690: e9 23 55 6d  	ldp	d9, d8, [sp, #336]
    3694: ff 03 07 91  	add	sp, sp, #448
    3698: c0 03 5f d6  	ret
    369c: e0 ff ff b0  	adrp	x0, 0x0 <main+0x108>
    36a0: 00 f4 32 91  	add	x0, x0, #3261
    36a4: e1 03 15 2a  	mov	w1, w21
    36a8: f1 ff ff 17  	b	0x366c <main+0xe4>
    36ac: f7 03 01 aa  	mov	x23, x1
    36b0: fe fa ff 97  	bl	0x22a8 <npu_open>
    36b4: a2 c3 00 d1  	sub	x2, x29, #48
    36b8: a3 e3 00 d1  	sub	x3, x29, #56
    36bc: a5 03 01 d1  	sub	x5, x29, #64
    36c0: 01 80 80 52  	mov	w1, #1024
    36c4: e4 03 1f 2a  	mov	w4, wzr
    36c8: f6 03 00 aa  	mov	x22, x0
    36cc: cb fa ff 97  	bl	0x21f8 <mem_allocate>
    36d0: fa 03 00 aa  	mov	x26, x0
    36d4: a2 23 01 d1  	sub	x2, x29, #72
    36d8: a3 43 01 d1  	sub	x3, x29, #80
    36dc: a5 63 01 d1  	sub	x5, x29, #88
    36e0: e0 03 16 aa  	mov	x0, x22
    36e4: 01 80 80 52  	mov	w1, #1024
    36e8: 04 01 80 52  	mov	w4, #8
    36ec: c3 fa ff 97  	bl	0x21f8 <mem_allocate>
    36f0: a8 7e 14 1b  	mul	w8, w21, w20
    36f4: fc 03 00 aa  	mov	x28, x0
    36f8: a2 83 01 d1  	sub	x2, x29, #96
    36fc: a3 a3 01 d1  	sub	x3, x29, #104
    3700: 1b f9 7f d3  	lsl	x27, x8, #1
    3704: a5 c3 01 d1  	sub	x5, x29, #112
    3708: e0 03 16 aa  	mov	x0, x22
    370c: e1 03 1b aa  	mov	x1, x27
    3710: e4 03 1f 2a  	mov	w4, wzr
    3714: b9 fa ff 97  	bl	0x21f8 <mem_allocate>
    3718: f3 7e 15 1b  	mul	w19, w23, w21
    371c: f8 03 00 aa  	mov	x24, x0
    3720: a2 e3 01 d1  	sub	x2, x29, #120
    3724: a3 03 02 d1  	sub	x3, x29, #128
    3728: 61 fa 7f d3  	lsl	x1, x19, #1
    372c: a5 23 02 d1  	sub	x5, x29, #136
    3730: e0 03 16 aa  	mov	x0, x22
    3734: e4 03 1f 2a  	mov	w4, wzr
    3738: e1 0f 00 f9  	str	x1, [sp, #24]
    373c: af fa ff 97  	bl	0x21f8 <mem_allocate>
    3740: e8 7e 14 1b  	mul	w8, w23, w20
    3744: f9 03 00 aa  	mov	x25, x0
    3748: a2 43 02 d1  	sub	x2, x29, #144
    374c: a3 63 02 d1  	sub	x3, x29, #152
    3750: 01 f9 7f d3  	lsl	x1, x8, #1
    3754: a5 83 02 d1  	sub	x5, x29, #160
    3758: e0 03 16 aa  	mov	x0, x22
    375c: e4 03 1f 2a  	mov	w4, wzr
    3760: f7 17 00 f9  	str	x23, [sp, #40]
    3764: f7 03 1a aa  	mov	x23, x26
    3768: e1 0b 00 f9  	str	x1, [sp, #16]
    376c: a3 fa ff 97  	bl	0x21f8 <mem_allocate>
    3770: e0 13 00 f9  	str	x0, [sp, #32]
    3774: a1 03 5a f8  	ldur	x1, [x29, #-96]
    3778: a2 03 57 f8  	ldur	x2, [x29, #-144]
    377c: e0 ff ff b0  	adrp	x0, 0x0 <main+0x1e8>
    3780: 00 a4 2d 91  	add	x0, x0, #2921
    3784: a3 83 58 f8  	ldur	x3, [x29, #-120]
    3788: 6a 01 00 94  	bl	0x3d30 <printf@plt>
    378c: fa 25 00 b4  	cbz	x26, 0x3c48 <main+0x6c0>
    3790: dc 25 00 b4  	cbz	x28, 0x3c48 <main+0x6c0>
    3794: b8 25 00 b4  	cbz	x24, 0x3c48 <main+0x6c0>
    3798: 99 25 00 b4  	cbz	x25, 0x3c48 <main+0x6c0>
    379c: e8 13 40 f9  	ldr	x8, [sp, #32]
    37a0: 48 25 00 b4  	cbz	x8, 0x3c48 <main+0x6c0>
    37a4: e0 03 16 aa  	mov	x0, x22
    37a8: db fa ff 97  	bl	0x2314 <npu_reset>
    37ac: a8 03 5a f8  	ldur	x8, [x29, #-96]
    37b0: e0 63 02 91  	add	x0, sp, #152
    37b4: a9 83 58 f8  	ldur	x9, [x29, #-120]
    37b8: f4 33 01 79  	strh	w20, [sp, #152]
    37bc: ea 17 40 f9  	ldr	x10, [sp, #40]
    37c0: f5 37 01 79  	strh	w21, [sp, #154]
    37c4: e8 27 14 29  	stp	w8, w9, [sp, #160]
    37c8: 28 00 80 52  	mov	w8, #1
    37cc: ea 3b 01 79  	strh	w10, [sp, #156]
    37d0: aa 03 57 f8  	ldur	x10, [x29, #-144]
    37d4: 1f 20 03 d5  	nop
    37d8: c9 49 01 10  	adr	x9, #10552
    37dc: e8 e3 02 39  	strb	w8, [sp, #184]
    37e0: ea ab 00 b9  	str	w10, [sp, #168]
    37e4: e9 5b 00 f9  	str	x9, [sp, #176]
    37e8: 90 fc ff 97  	bl	0x2a28 <gen_matmul_fp16>
    37ec: e0 00 00 34  	cbz	w0, 0x3808 <main+0x280>
    37f0: fa 03 00 2a  	mov	w26, w0
    37f4: e0 ff ff b0  	adrp	x0, 0x0 <main+0x260>
    37f8: 00 e0 33 91  	add	x0, x0, #3320
    37fc: e1 03 1a 2a  	mov	w1, w26
    3800: 4c 01 00 94  	bl	0x3d30 <printf@plt>
    3804: fa 00 00 14  	b	0x3bec <main+0x664>
    3808: 1f 20 03 d5  	nop
    380c: 21 48 01 10  	adr	x1, #10500
    3810: e0 03 17 aa  	mov	x0, x23
    3814: 02 70 80 52  	mov	w2, #896
    3818: f6 07 00 f9  	str	x22, [sp, #8]
    381c: 65 01 00 94  	bl	0x3db0 <memcpy@plt>
    3820: e8 ff ff d0  	adrp	x8, 0x1000 <main+0x290>
    3824: e9 ff ff b0  	adrp	x9, 0x0 <main+0x290>
    3828: e0 03 18 aa  	mov	x0, x24
    382c: e1 03 1f 2a  	mov	w1, wzr
    3830: e2 03 1b aa  	mov	x2, x27
    3834: 00 01 c0 3d  	ldr	q0, [x8]
    3838: 21 d1 c3 3d  	ldr	q1, [x9, #3904]
    383c: a8 03 5d f8  	ldur	x8, [x29, #-48]
    3840: 80 07 00 ad  	stp	q0, q1, [x28]
    3844: 88 13 00 f9  	str	x8, [x28, #32]
    3848: 4e 01 00 94  	bl	0x3d80 <memset@plt>
    384c: e0 03 19 aa  	mov	x0, x25
    3850: e1 03 1f 2a  	mov	w1, wzr
    3854: e2 0f 40 f9  	ldr	x2, [sp, #24]
    3858: 4a 01 00 94  	bl	0x3d80 <memset@plt>
    385c: e0 13 40 f9  	ldr	x0, [sp, #32]
    3860: e1 03 1f 2a  	mov	w1, wzr
    3864: e2 0b 40 f9  	ldr	x2, [sp, #16]
    3868: 46 01 00 94  	bl	0x3d80 <memset@plt>
    386c: e0 03 1f aa  	mov	x0, xzr
    3870: 54 01 00 94  	bl	0x3dc0 <time@plt>
    3874: 57 01 00 94  	bl	0x3dd0 <srand@plt>
    3878: 08 66 01 0f  	movi	v8.2s, #48, lsl #24
    387c: f6 03 1f aa  	mov	x22, xzr
    3880: 09 90 64 1e  	fmov	d9, #10.00000000
    3884: 1f 20 03 d5  	nop
    3888: 5c 60 01 10  	adr	x28, #11272
    388c: 41 01 00 94  	bl	0x3d90 <rand@plt>
    3890: 00 00 22 1e  	scvtf	s0, w0
    3894: 00 08 28 1e  	fmul	s0, s0, s8
    3898: 00 c0 22 1e  	fcvt	d0, s0
    389c: 00 08 69 1e  	fmul	d0, d0, d9
    38a0: 08 00 78 1e  	fcvtzs	w8, d0
    38a4: 00 01 22 1e  	scvtf	s0, w8
    38a8: 00 c0 23 1e  	fcvt	h0, s0
    38ac: 80 6b 36 7c  	str	h0, [x28, x22]
    38b0: d6 0a 00 91  	add	x22, x22, #2
    38b4: 7f 03 16 eb  	cmp	x27, x22
    38b8: a1 fe ff 54  	b.ne	0x388c <main+0x304>
    38bc: 13 02 00 34  	cbz	w19, 0x38fc <main+0x374>
    38c0: 08 66 01 0f  	movi	v8.2s, #48, lsl #24
    38c4: 09 90 64 1e  	fmov	d9, #10.00000000
    38c8: 16 30 00 f0  	adrp	x22, 0x606000 <usys_revoke_cap+0x50d4>
    38cc: d6 42 12 91  	add	x22, x22, #1168
    38d0: 30 01 00 94  	bl	0x3d90 <rand@plt>
    38d4: 00 00 22 1e  	scvtf	s0, w0
    38d8: 73 06 00 f1  	subs	x19, x19, #1
    38dc: 00 08 28 1e  	fmul	s0, s0, s8
    38e0: 00 c0 22 1e  	fcvt	d0, s0
    38e4: 00 08 69 1e  	fmul	d0, d0, d9
    38e8: 08 00 78 1e  	fcvtzs	w8, d0
    38ec: 00 01 22 1e  	scvtf	s0, w8
    38f0: 00 c0 23 1e  	fcvt	h0, s0
    38f4: c0 26 00 7c  	str	h0, [x22], #2
    38f8: c1 fe ff 54  	b.ne	0x38d0 <main+0x348>
    38fc: e8 17 40 f9  	ldr	x8, [sp, #40]
    3900: a8 03 00 34  	cbz	w8, 0x3974 <main+0x3ec>
    3904: 35 07 00 34  	cbz	w21, 0x39e8 <main+0x460>
    3908: a8 06 00 11  	add	w8, w21, #1
    390c: f3 03 1f aa  	mov	x19, xzr
    3910: e9 03 15 2a  	mov	w9, w21
    3914: 16 05 00 d1  	sub	x22, x8, #1
    3918: 3a 00 80 52  	mov	w26, #1
    391c: 17 30 00 f0  	adrp	x23, 0x606000 <usys_revoke_cap+0x5128>
    3920: f7 42 12 91  	add	x23, x23, #1168
    3924: e9 0f 00 f9  	str	x9, [sp, #24]
    3928: e8 03 1f aa  	mov	x8, xzr
    392c: 69 02 08 0b  	add	w9, w19, w8
    3930: 1b 05 00 91  	add	x27, x8, #1
    3934: e0 03 15 2a  	mov	w0, w21
    3938: e1 03 1a 2a  	mov	w1, w26
    393c: e2 03 1b 2a  	mov	w2, w27
    3940: e8 5a 69 7c  	ldr	h8, [x23, w9, uxtw #1]
    3944: 2a fd ff 97  	bl	0x2dec <weight_fp16>
    3948: e8 03 1b aa  	mov	x8, x27
    394c: df 02 1b eb  	cmp	x22, x27
    3950: 28 db 20 7c  	str	h8, [x25, w0, sxtw #1]
    3954: c1 fe ff 54  	b.ne	0x392c <main+0x3a4>
    3958: e9 17 40 f9  	ldr	x9, [sp, #40]
    395c: 48 07 00 11  	add	w8, w26, #1
    3960: 5f 03 09 6b  	cmp	w26, w9
    3964: e9 0f 40 f9  	ldr	x9, [sp, #24]
    3968: fa 03 08 2a  	mov	w26, w8
    396c: 73 02 09 8b  	add	x19, x19, x9
    3970: c1 fd ff 54  	b.ne	0x3928 <main+0x3a0>
    3974: b5 03 00 34  	cbz	w21, 0x39e8 <main+0x460>
    3978: a8 06 00 11  	add	w8, w21, #1
    397c: 9f 06 00 71  	cmp	w20, #1
    3980: f3 03 1f aa  	mov	x19, xzr
    3984: 97 86 9f 1a  	csinc	w23, w20, wzr, hi
    3988: fb 03 15 2a  	mov	w27, w21
    398c: 16 05 00 d1  	sub	x22, x8, #1
    3990: 39 00 80 52  	mov	w25, #1
    3994: e8 03 1f aa  	mov	x8, xzr
    3998: 69 02 08 0b  	add	w9, w19, w8
    399c: 1a 05 00 91  	add	x26, x8, #1
    39a0: e0 03 15 2a  	mov	w0, w21
    39a4: e1 03 14 2a  	mov	w1, w20
    39a8: 22 00 80 52  	mov	w2, #1
    39ac: 03 01 80 52  	mov	w3, #8
    39b0: 88 5b 69 7c  	ldr	h8, [x28, w9, uxtw #1]
    39b4: e4 03 1a 2a  	mov	w4, w26
    39b8: e5 03 19 2a  	mov	w5, w25
    39bc: 26 00 80 52  	mov	w6, #1
    39c0: 02 fd ff 97  	bl	0x2dc8 <feature_data>
    39c4: e8 03 1a aa  	mov	x8, x26
    39c8: df 02 1a eb  	cmp	x22, x26
    39cc: 08 db 20 7c  	str	h8, [x24, w0, sxtw #1]
    39d0: 41 fe ff 54  	b.ne	0x3998 <main+0x410>
    39d4: 28 07 00 11  	add	w8, w25, #1
    39d8: 3f 03 17 6b  	cmp	w25, w23
    39dc: 73 02 1b 8b  	add	x19, x19, x27
    39e0: f9 03 08 2a  	mov	w25, w8
    39e4: 81 fd ff 54  	b.ne	0x3994 <main+0x40c>
    39e8: f7 17 40 f9  	ldr	x23, [sp, #40]
    39ec: 18 30 04 f0  	adrp	x24, 0x8606000 <matrixA+0x1ed68>
    39f0: 18 43 12 91  	add	x24, x24, #1168
    39f4: 1f 20 03 d5  	nop
    39f8: c3 54 01 10  	adr	x3, #10904
    39fc: 04 30 00 f0  	adrp	x4, 0x606000 <usys_revoke_cap+0x5208>
    3a00: 84 40 12 91  	add	x4, x4, #1168
    3a04: e0 03 14 2a  	mov	w0, w20
    3a08: e1 03 15 2a  	mov	w1, w21
    3a0c: e2 03 17 2a  	mov	w2, w23
    3a10: e5 03 18 aa  	mov	x5, x24
    3a14: 64 fe ff 97  	bl	0x33a4 <matmul_fp16>
    3a18: a8 03 5b f8  	ldur	x8, [x29, #-80]
    3a1c: e9 ff ff b0  	adrp	x9, 0x0 <main+0x488>
    3a20: ea ff ff d0  	adrp	x10, 0x1000 <main+0x490>
    3a24: a0 a3 00 d1  	sub	x0, x29, #40
    3a28: e1 03 1f aa  	mov	x1, xzr
    3a2c: ff 7f 05 a9  	stp	xzr, xzr, [sp, #80]
    3a30: ff 23 04 a9  	stp	xzr, x8, [sp, #64]
    3a34: 20 ed c3 3d  	ldr	q0, [x9, #4016]
    3a38: 41 11 40 fd  	ldr	d1, [x10, #32]
    3a3c: 1f 20 03 d5  	nop
    3a40: 48 b0 fe 10  	adr	x8, #-10744
    3a44: ff 33 00 f9  	str	xzr, [sp, #96]
    3a48: e0 0f 80 3d  	str	q0, [sp, #48]
    3a4c: e1 37 00 fd  	str	d1, [sp, #104]
    3a50: 00 05 40 ad  	ldp	q0, q1, [x8]
    3a54: ff 4b 00 f9  	str	xzr, [sp, #144]
    3a58: e0 87 03 ad  	stp	q0, q1, [sp, #112]
    3a5c: e1 00 00 94  	bl	0x3de0 <gettimeofday@plt>
    3a60: b3 db 7d a9  	ldp	x19, x22, [x29, #-40]
    3a64: e1 c3 00 91  	add	x1, sp, #48
    3a68: e0 03 1f aa  	mov	x0, xzr
    3a6c: fe fd ff 97  	bl	0x3264 <rknpu_submit>
    3a70: fa 03 00 2a  	mov	w26, w0
    3a74: a0 a3 00 d1  	sub	x0, x29, #40
    3a78: e1 03 1f aa  	mov	x1, xzr
    3a7c: d9 00 00 94  	bl	0x3de0 <gettimeofday@plt>
    3a80: a8 a7 7d a9  	ldp	x8, x9, [x29, #-40]
    3a84: 0a 48 88 52  	mov	w10, #16960
    3a88: e0 ff ff b0  	adrp	x0, 0x0 <main+0x4f4>
    3a8c: 00 3c 36 91  	add	x0, x0, #3471
    3a90: ea 01 a0 72  	movk	w10, #15, lsl #16
    3a94: 08 01 13 cb  	sub	x8, x8, x19
    3a98: 29 01 16 cb  	sub	x9, x9, x22
    3a9c: 08 25 0a 9b  	madd	x8, x8, x10, x9
    3aa0: 49 8f a8 52  	mov	w9, #1148846080
    3aa4: 21 01 27 1e  	fmov	s1, w9
    3aa8: 00 01 23 9e  	ucvtf	s0, x8
    3aac: 08 80 84 52  	mov	w8, #9216
    3ab0: 88 2e a9 72  	movk	w8, #18804, lsl #16
    3ab4: 02 01 27 1e  	fmov	s2, w8
    3ab8: 01 18 21 1e  	fdiv	s1, s0, s1
    3abc: 42 18 20 1e  	fdiv	s2, s2, s0
    3ac0: 20 c0 22 1e  	fcvt	d0, s1
    3ac4: 41 c0 22 1e  	fcvt	d1, s2
    3ac8: 9a 00 00 94  	bl	0x3d30 <printf@plt>
    3acc: e0 ff ff b0  	adrp	x0, 0x0 <main+0x538>
    3ad0: 00 3c 2d 91  	add	x0, x0, #2895
    3ad4: e1 03 1a 2a  	mov	w1, w26
    3ad8: 96 00 00 94  	bl	0x3d30 <printf@plt>
    3adc: da dc ff 37  	tbnz	w26, #31, 0x3674 <main+0xec>
    3ae0: e0 ff ff b0  	adrp	x0, 0x0 <main+0x54c>
    3ae4: 00 94 34 91  	add	x0, x0, #3365
    3ae8: c2 00 00 94  	bl	0x3df0 <puts@plt>
    3aec: e3 03 17 aa  	mov	x3, x23
    3af0: ff 0a 00 71  	cmp	w23, #2
    3af4: 63 06 00 54  	b.lo	0x3bc0 <main+0x638>
    3af8: 9f 06 00 71  	cmp	w20, #1
    3afc: f3 03 1f aa  	mov	x19, xzr
    3b00: 88 86 9f 1a  	csinc	w8, w20, wzr, hi
    3b04: 76 04 00 d1  	sub	x22, x3, #1
    3b08: 39 00 80 52  	mov	w25, #1
    3b0c: fb ff ff b0  	adrp	x27, 0x0 <main+0x578>
    3b10: 7b bf 36 91  	add	x27, x27, #3503
    3b14: e8 1b 00 b9  	str	w8, [sp, #24]
    3b18: 07 00 00 14  	b	0x3b34 <main+0x5ac>
    3b1c: e9 1b 40 b9  	ldr	w9, [sp, #24]
    3b20: 28 07 00 11  	add	w8, w25, #1
    3b24: 73 02 03 8b  	add	x19, x19, x3
    3b28: 3f 03 09 6b  	cmp	w25, w9
    3b2c: f9 03 08 2a  	mov	w25, w8
    3b30: 80 04 00 54  	b.eq	0x3bc0 <main+0x638>
    3b34: f7 03 1f aa  	mov	x23, xzr
    3b38: 05 00 00 14  	b	0x3b4c <main+0x5c4>
    3b3c: f7 03 1c aa  	mov	x23, x28
    3b40: e3 17 40 f9  	ldr	x3, [sp, #40]
    3b44: df 02 1c eb  	cmp	x22, x28
    3b48: a0 fe ff 54  	b.eq	0x3b1c <main+0x594>
    3b4c: fc 06 00 91  	add	x28, x23, #1
    3b50: e0 03 03 2a  	mov	w0, w3
    3b54: e1 03 14 2a  	mov	w1, w20
    3b58: 22 00 80 52  	mov	w2, #1
    3b5c: 03 01 80 52  	mov	w3, #8
    3b60: e4 03 1c 2a  	mov	w4, w28
    3b64: e5 03 19 2a  	mov	w5, w25
    3b68: 26 00 80 52  	mov	w6, #1
    3b6c: 97 fc ff 97  	bl	0x2dc8 <feature_data>
    3b70: 68 02 17 0b  	add	w8, w19, w23
    3b74: e9 13 40 f9  	ldr	x9, [sp, #32]
    3b78: 22 d9 60 7c  	ldr	h2, [x9, w0, sxtw #1]
    3b7c: 03 5b 68 7c  	ldr	h3, [x24, w8, uxtw #1]
    3b80: 40 40 e2 1e  	fcvt	s0, h2
    3b84: 61 40 e2 1e  	fcvt	s1, h3
    3b88: 00 20 21 1e  	fcmp	s0, s1
    3b8c: 80 fd ff 54  	b.eq	0x3b3c <main+0x5b4>
    3b90: 68 00 26 1e  	fmov	w8, s3
    3b94: 49 00 26 1e  	fmov	w9, s2
    3b98: 60 c0 e2 1e  	fcvt	d0, h3
    3b9c: 41 c0 e2 1e  	fcvt	d1, h2
    3ba0: e0 03 1b aa  	mov	x0, x27
    3ba4: e1 03 19 2a  	mov	w1, w25
    3ba8: 03 3d 00 13  	sxth	w3, w8
    3bac: 24 3d 00 13  	sxth	w4, w9
    3bb0: e2 03 1c 2a  	mov	w2, w28
    3bb4: 5f 00 00 94  	bl	0x3d30 <printf@plt>
    3bb8: 1a 00 80 12  	mov	w26, #-1
    3bbc: e0 ff ff 17  	b	0x3b3c <main+0x5b4>
    3bc0: fa 00 00 35  	cbnz	w26, 0x3bdc <main+0x654>
    3bc4: e0 ff ff b0  	adrp	x0, 0x0 <main+0x630>
    3bc8: 00 20 32 91  	add	x0, x0, #3208
    3bcc: e1 03 14 2a  	mov	w1, w20
    3bd0: e2 03 15 2a  	mov	w2, w21
    3bd4: e4 03 15 2a  	mov	w4, w21
    3bd8: 56 00 00 94  	bl	0x3d30 <printf@plt>
    3bdc: e0 ff ff b0  	adrp	x0, 0x0 <main+0x648>
    3be0: 00 94 34 91  	add	x0, x0, #3365
    3be4: 83 00 00 94  	bl	0x3df0 <puts@plt>
    3be8: f6 07 40 f9  	ldr	x22, [sp, #8]
    3bec: a1 03 5c b8  	ldur	w1, [x29, #-64]
    3bf0: e0 03 16 aa  	mov	x0, x22
    3bf4: a2 83 5c f8  	ldur	x2, [x29, #-56]
    3bf8: aa f9 ff 97  	bl	0x22a0 <mem_destroy>
    3bfc: a1 83 5a b8  	ldur	w1, [x29, #-88]
    3c00: e0 03 16 aa  	mov	x0, x22
    3c04: a2 03 5b f8  	ldur	x2, [x29, #-80]
    3c08: a6 f9 ff 97  	bl	0x22a0 <mem_destroy>
    3c0c: a1 03 59 b8  	ldur	w1, [x29, #-112]
    3c10: e0 03 16 aa  	mov	x0, x22
    3c14: a2 83 59 f8  	ldur	x2, [x29, #-104]
    3c18: a2 f9 ff 97  	bl	0x22a0 <mem_destroy>
    3c1c: a1 83 57 b8  	ldur	w1, [x29, #-136]
    3c20: e0 03 16 aa  	mov	x0, x22
    3c24: a2 03 58 f8  	ldur	x2, [x29, #-128]
    3c28: 9e f9 ff 97  	bl	0x22a0 <mem_destroy>
    3c2c: a1 03 56 b8  	ldur	w1, [x29, #-160]
    3c30: e0 03 16 aa  	mov	x0, x22
    3c34: a2 83 56 f8  	ldur	x2, [x29, #-152]
    3c38: 9a f9 ff 97  	bl	0x22a0 <mem_destroy>
    3c3c: e0 03 16 aa  	mov	x0, x22
    3c40: b3 f9 ff 97  	bl	0x230c <npu_close>
    3c44: 8c fe ff 17  	b	0x3674 <main+0xec>
    3c48: e0 ff ff b0  	adrp	x0, 0x0 <main+0x6b4>
    3c4c: 00 88 2e 91  	add	x0, x0, #2978
    3c50: 68 00 00 94  	bl	0x3df0 <puts@plt>
    3c54: 20 00 80 52  	mov	w0, #1
    3c58: 6a 00 00 94  	bl	0x3e00 <exit@plt>

Disassembly of section .init:

0000000000003c5c <_init>:
    3c5c: fd 7b bf a9  	stp	x29, x30, [sp, #-16]!
    3c60: fd 03 00 91  	mov	x29, sp

0000000000003c64 <$x.0>:
    3c64: fd 7b c1 a8  	ldp	x29, x30, [sp], #16
    3c68: c0 03 5f d6  	ret

Disassembly of section .fini:

0000000000003c6c <_fini>:
    3c6c: fd 7b bf a9  	stp	x29, x30, [sp, #-16]!
    3c70: fd 03 00 91  	mov	x29, sp

0000000000003c74 <$x.1>:
    3c74: fd 7b c1 a8  	ldp	x29, x30, [sp], #16
    3c78: c0 03 5f d6  	ret

Disassembly of section .plt:

0000000000003c80 <.plt>:
    3c80: f0 7b bf a9  	stp	x16, x30, [sp, #-16]!
    3c84: 10 00 00 b0  	adrp	x16, 0x4000 <.plt+0x8>
    3c88: 11 fe 47 f9  	ldr	x17, [x16, #4088]
    3c8c: 10 e2 3f 91  	add	x16, x16, #4088
    3c90: 20 02 1f d6  	br	x17
    3c94: 1f 20 03 d5  	nop
    3c98: 1f 20 03 d5  	nop
    3c9c: 1f 20 03 d5  	nop

0000000000003ca0 <__libc_start_main@plt>:
    3ca0: 10 00 00 d0  	adrp	x16, 0x5000 <__libc_start_main@plt+0x8>
    3ca4: 11 02 40 f9  	ldr	x17, [x16]
    3ca8: 10 02 00 91  	add	x16, x16, #0
    3cac: 20 02 1f d6  	br	x17

0000000000003cb0 <__register_frame_info@plt>:
    3cb0: 10 00 00 d0  	adrp	x16, 0x5000 <__register_frame_info@plt+0x8>
    3cb4: 11 06 40 f9  	ldr	x17, [x16, #8]
    3cb8: 10 22 00 91  	add	x16, x16, #8
    3cbc: 20 02 1f d6  	br	x17

0000000000003cc0 <__at_fini@plt>:
    3cc0: 10 00 00 d0  	adrp	x16, 0x5000 <__at_fini@plt+0x8>
    3cc4: 11 0a 40 f9  	ldr	x17, [x16, #16]
    3cc8: 10 42 00 91  	add	x16, x16, #16
    3ccc: 20 02 1f d6  	br	x17

0000000000003cd0 <__cxa_finalize@plt>:
    3cd0: 10 00 00 d0  	adrp	x16, 0x5000 <__cxa_finalize@plt+0x8>
    3cd4: 11 0e 40 f9  	ldr	x17, [x16, #24]
    3cd8: 10 62 00 91  	add	x16, x16, #24
    3cdc: 20 02 1f d6  	br	x17

0000000000003ce0 <__deregister_frame_info@plt>:
    3ce0: 10 00 00 d0  	adrp	x16, 0x5000 <__deregister_frame_info@plt+0x8>
    3ce4: 11 12 40 f9  	ldr	x17, [x16, #32]
    3ce8: 10 82 00 91  	add	x16, x16, #32
    3cec: 20 02 1f d6  	br	x17

0000000000003cf0 <malloc@plt>:
    3cf0: 10 00 00 d0  	adrp	x16, 0x5000 <malloc@plt+0x8>
    3cf4: 11 16 40 f9  	ldr	x17, [x16, #40]
    3cf8: 10 a2 00 91  	add	x16, x16, #40
    3cfc: 20 02 1f d6  	br	x17

0000000000003d00 <chcore_alloc_dma_mem@plt>:
    3d00: 10 00 00 d0  	adrp	x16, 0x5000 <chcore_alloc_dma_mem@plt+0x8>
    3d04: 11 1a 40 f9  	ldr	x17, [x16, #48]
    3d08: 10 c2 00 91  	add	x16, x16, #48
    3d0c: 20 02 1f d6  	br	x17

0000000000003d10 <__assert_fail@plt>:
    3d10: 10 00 00 d0  	adrp	x16, 0x5000 <__assert_fail@plt+0x8>
    3d14: 11 1e 40 f9  	ldr	x17, [x16, #56]
    3d18: 10 e2 00 91  	add	x16, x16, #56
    3d1c: 20 02 1f d6  	br	x17

0000000000003d20 <chcore_free_dma_mem@plt>:
    3d20: 10 00 00 d0  	adrp	x16, 0x5000 <chcore_free_dma_mem@plt+0x8>
    3d24: 11 22 40 f9  	ldr	x17, [x16, #64]
    3d28: 10 02 01 91  	add	x16, x16, #64
    3d2c: 20 02 1f d6  	br	x17

0000000000003d30 <printf@plt>:
    3d30: 10 00 00 d0  	adrp	x16, 0x5000 <printf@plt+0x8>
    3d34: 11 26 40 f9  	ldr	x17, [x16, #72]
    3d38: 10 22 01 91  	add	x16, x16, #72
    3d3c: 20 02 1f d6  	br	x17

0000000000003d40 <usys_create_device_pmo@plt>:
    3d40: 10 00 00 d0  	adrp	x16, 0x5000 <usys_create_device_pmo@plt+0x8>
    3d44: 11 2a 40 f9  	ldr	x17, [x16, #80]
    3d48: 10 42 01 91  	add	x16, x16, #80
    3d4c: 20 02 1f d6  	br	x17

0000000000003d50 <chcore_auto_map_pmo@plt>:
    3d50: 10 00 00 d0  	adrp	x16, 0x5000 <chcore_auto_map_pmo@plt+0x8>
    3d54: 11 2e 40 f9  	ldr	x17, [x16, #88]
    3d58: 10 62 01 91  	add	x16, x16, #88
    3d5c: 20 02 1f d6  	br	x17

0000000000003d60 <chcore_auto_unmap_pmo@plt>:
    3d60: 10 00 00 d0  	adrp	x16, 0x5000 <chcore_auto_unmap_pmo@plt+0x8>
    3d64: 11 32 40 f9  	ldr	x17, [x16, #96]
    3d68: 10 82 01 91  	add	x16, x16, #96
    3d6c: 20 02 1f d6  	br	x17

0000000000003d70 <usys_revoke_cap@plt>:
    3d70: 10 00 00 d0  	adrp	x16, 0x5000 <usys_revoke_cap@plt+0x8>
    3d74: 11 36 40 f9  	ldr	x17, [x16, #104]
    3d78: 10 a2 01 91  	add	x16, x16, #104
    3d7c: 20 02 1f d6  	br	x17

0000000000003d80 <memset@plt>:
    3d80: 10 00 00 d0  	adrp	x16, 0x5000 <memset@plt+0x8>
    3d84: 11 3a 40 f9  	ldr	x17, [x16, #112]
    3d88: 10 c2 01 91  	add	x16, x16, #112
    3d8c: 20 02 1f d6  	br	x17

0000000000003d90 <rand@plt>:
    3d90: 10 00 00 d0  	adrp	x16, 0x5000 <rand@plt+0x8>
    3d94: 11 3e 40 f9  	ldr	x17, [x16, #120]
    3d98: 10 e2 01 91  	add	x16, x16, #120
    3d9c: 20 02 1f d6  	br	x17

0000000000003da0 <atoi@plt>:
    3da0: 10 00 00 d0  	adrp	x16, 0x5000 <atoi@plt+0x8>
    3da4: 11 42 40 f9  	ldr	x17, [x16, #128]
    3da8: 10 02 02 91  	add	x16, x16, #128
    3dac: 20 02 1f d6  	br	x17

0000000000003db0 <memcpy@plt>:
    3db0: 10 00 00 d0  	adrp	x16, 0x5000 <memcpy@plt+0x8>
    3db4: 11 46 40 f9  	ldr	x17, [x16, #136]
    3db8: 10 22 02 91  	add	x16, x16, #136
    3dbc: 20 02 1f d6  	br	x17

0000000000003dc0 <time@plt>:
    3dc0: 10 00 00 d0  	adrp	x16, 0x5000 <time@plt+0x8>
    3dc4: 11 4a 40 f9  	ldr	x17, [x16, #144]
    3dc8: 10 42 02 91  	add	x16, x16, #144
    3dcc: 20 02 1f d6  	br	x17

0000000000003dd0 <srand@plt>:
    3dd0: 10 00 00 d0  	adrp	x16, 0x5000 <srand@plt+0x8>
    3dd4: 11 4e 40 f9  	ldr	x17, [x16, #152]
    3dd8: 10 62 02 91  	add	x16, x16, #152
    3ddc: 20 02 1f d6  	br	x17

0000000000003de0 <gettimeofday@plt>:
    3de0: 10 00 00 d0  	adrp	x16, 0x5000 <gettimeofday@plt+0x8>
    3de4: 11 52 40 f9  	ldr	x17, [x16, #160]
    3de8: 10 82 02 91  	add	x16, x16, #160
    3dec: 20 02 1f d6  	br	x17

0000000000003df0 <puts@plt>:
    3df0: 10 00 00 d0  	adrp	x16, 0x5000 <puts@plt+0x8>
    3df4: 11 56 40 f9  	ldr	x17, [x16, #168]
    3df8: 10 a2 02 91  	add	x16, x16, #168
    3dfc: 20 02 1f d6  	br	x17

0000000000003e00 <exit@plt>:
    3e00: 10 00 00 d0  	adrp	x16, 0x5000 <exit@plt+0x8>
    3e04: 11 5a 40 f9  	ldr	x17, [x16, #176]
    3e08: 10 c2 02 91  	add	x16, x16, #176
    3e0c: 20 02 1f d6  	br	x17
