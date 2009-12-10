/*
    This file is part of rdnssd_win32.
    Copyright (C) 2008-2009 Sebastien Vincent <sebastien.vincent@cppextrem.com>

    rdnssd_win32 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    rdnssd_win32 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with rdnssd_win32.  If not, see <http://www.gnu.org/licenses/>.
*/

/*
* Copyright (C) 2004 Internet Systems Consortium, Inc. ("ISC")
* Copyright (C) 1996-2001 Internet Software Consortium.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
* REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
* AND FITNESS. IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
* INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
* LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
* OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
* PERFORMANCE OF THIS SOFTWARE.
*/

#if defined(LIBC_SCCS) && !defined(lint)
static char rcsid[] =
    "$Id: inet_ntop.c,v 1.12.2.1 2004/03/09 06:11:47 marka Exp $";
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "inet_function.h"

#define NS_INT16SZ 2
#define NS_IN6ADDRSZ 16

/*
* WARNING: Don't even consider trying to compile this on a system where
* sizeof(int) < 4. sizeof(int) > 4 is fine; all the world's not a VAX.
*/

static const char *inet_ntop4(const unsigned char *src, char *dst,
                              size_t size);

#ifdef AF_INET6
static const char *inet_ntop6(const unsigned char *src, char *dst,
                              size_t size);
#endif

/* char *
 * isc_net_ntop(af, src, dst, size)
 * convert a network format address to presentation format.
 * return:
 * pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 * Paul Vixie, 1996.
 */
const char * inet_ntop2(int af, const void *src, char *dst, size_t size)
{
    switch(af)
    {
    case AF_INET:
        return (inet_ntop4(src, dst, size));
#ifdef AF_INET6
    case AF_INET6:
        return (inet_ntop6(src, dst, size));
#endif
    default:
        errno = EAFNOSUPPORT;
        return (NULL);
    }
    /* NOTREACHED */
}

/* const char *
 * inet_ntop4(src, dst, size)
 * format an IPv4 address
 * return:
 * `dst' (as a const)
 * notes:
 * (1) uses no statics
 * (2) takes a unsigned char* not an in_addr as input
 * author:
 * Paul Vixie, 1996.
 */
static const char *inet_ntop4(const unsigned char *src, char *dst, size_t size)
{
    static const char *fmt = "%u.%u.%u.%u";
    char tmp[sizeof("255.255.255.255")];

    if((size_t)sprintf(tmp, fmt, src[0], src[1], src[2], src[3]) >= size)
    {
        errno = ENOSPC;
        return (NULL);
    }

    strcpy(dst, tmp);

    return (dst);
}

/* const char *
 * isc_inet_ntop6(src, dst, size)
 * convert IPv6 binary address into presentation (printable) format
 * author:
 * Paul Vixie, 1996.
 */
#ifdef AF_INET6
static const char * inet_ntop6(const unsigned char *src, char *dst, size_t size)
{
    /*
     * Note that int32_t and int16_t need only be "at least" large enough
     * to contain a value of the specified size. On some systems, like
     * Crays, there is no such thing as an integer variable with 16 bits.
     * Keep this in mind if you think this function should have been coded
     * to use pointer overlays. All the world's not a VAX.
     */
    char tmp[sizeof "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255"];
    char* tp = NULL;
    unsigned int words[NS_IN6ADDRSZ / NS_INT16SZ];
    int i = 0;
    struct
    {
        int base, len;
    }best, cur;

    /*
     * Preprocess:
     * Copy the input (bytewise) array into a wordwise array.
     * Find the longest run of 0x00's in src[] for :: shorthanding.
     */
    memset(words, '\0', sizeof words);
    
    for(i = 0; i < NS_IN6ADDRSZ; i++)
    {
        words[i / 2] |= (src[i] << ((1 - (i % 2)) << 3));
    }

    best.base = -1;
    best.len = -1;
    cur.base = -1;
    cur.len = -1;

    for(i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
        if(words[i] == 0)
        {
            if(cur.base == -1)
            {
                cur.base = i, cur.len = 1;
            }
            else
            {
                cur.len++;
            }
        }
        else
        {
            if(cur.base != -1)
            {
                if(best.base == -1 || cur.len > best.len)
                {
                    best = cur;
                }
                cur.base = -1;
            }
        }
    }

    if(cur.base != -1)
    {
        if(best.base == -1 || cur.len > best.len)
        {
            best = cur;
        }
    }

    if(best.base != -1 && best.len < 2)
    {
        best.base = -1;
    }

    /*
     * Format the result.
     */
    tp = tmp;

    for(i = 0; i < (NS_IN6ADDRSZ / NS_INT16SZ); i++)
    {
        /* Are we inside the best run of 0x00's? */
        if(best.base != -1 && i >= best.base &&
                i < (best.base + best.len))
        {
            if(i == best.base)
            {
                *tp++ = ':';
            }
            continue;
        }

        /* Are we following an initial run of 0x00s or any real hex? */
        if(i != 0)
        {
            *tp++ = ':';
        }

        /* Is this address an encapsulated IPv4? */
        if(i == 6 && best.base == 0 &&
                (best.len == 6 || (best.len == 5 && words[5] == 0xffff)))
        {
            if(!inet_ntop4(src+12, tp, sizeof tmp - (tp - tmp)))
            {
                return (NULL);
            }

            tp += strlen(tp);
            break;
        }

        tp += sprintf(tp, "%x", words[i]);
    }

    /* Was it a trailing run of 0x00's? */
    if(best.base != -1 && (best.base + best.len) == (NS_IN6ADDRSZ / NS_INT16SZ))
    {
        *tp++ = ':';
    }

    *tp++ = '\0';

    /*
     * Check for overflow, copy, and we're done.
     */
    if((size_t)(tp - tmp) > size)
    {
        errno = ENOSPC;
        return (NULL);
    }

    strcpy(dst, tmp);
    return (dst);
}
#endif /* AF_INET6 */