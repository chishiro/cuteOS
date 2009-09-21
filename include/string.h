#ifndef STRING_H
#define STRING_H

/*
 * Standard C string definitions
 *
 * Copyright (C) 2009 Ahmed S. Darwish <darwish.07@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 2.
 */


int strlen(const char *str);
void strncpy(char *dst, const char *src, int n);
void memcpy(void *dst, void *src, int len);

#endif
