/*
 * Standard C string methods
 *
 * Copyright (C) 2009 Ahmed S. Darwish <darwish.07@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.
 *
 * This should be transformed to inline x86 assembly as appropriate,
 * let's just have them for now.
 */

#include <stdint.h>

int strlen(const char *str)
{
	const char *tmp;

	for (tmp = str; *tmp; tmp++)
		;
	return tmp - str;
}

char *strncpy(char *dst, const char *src, int n)
{
	char *tmp = dst;

	while (n) {
		*tmp = *src;
		if (*tmp)
			src++;
		tmp++;
		n--;
	}

	return dst;
}

/*
 * Optimized mem* methods
 * For more speed, we should let movsq and stosq read
 * arguments (source) be 8 byte aligned.
 */

void *memcpy(void *dst, void *src, int len)
{
	int tmp;

	asm(
	    "movl %[len], %[tmp];"
	    "andl $7, %[len];"
	    "rep  movsb;"
	    "movl %[tmp], %[len];"
	    "shrl $3, %[len];"
	    "rep  movsq;"
	    :
	    : "S"(src), "D"(dst), [len] "c"(len), [tmp] "r"(tmp)
	    : "memory");

	return dst;
}

void *memset(void *dst, int ch, int len)
{
	uint64_t tmp;
	uint64_t uch = ch;
	uint64_t ulen = len;

	uch &= 0xff;
	asm(
	    "mov %[len], %[tmp];"
	    "and $7, %[len];"
	    "rep  stosb;"
	    "mov %[tmp], %[len];"
	    "shr $3, %[len];"
	    /* Copy the right-most byte to the rest 7 bytes
	     * value extracted by: 0xffffffffffffffff/0xff */
	    "mov  %[ch], %[tmp];"
	    "mov  $0x0101010101010101, %[ch];"
	    "mul  %[tmp];"
	    "rep  stosq;"
	    :
	    : "D"(dst), [ch] "a"(uch), [len] "c"(ulen), [tmp] "r"(tmp)
	    : "memory");

	return dst;
}
