/****************************************************************************
 * libc/netdb/lib_dnsaddserver.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <nuttx/net/dns.h>

#include "netdb/lib_dns.h"

#ifdef CONFIG_NETDB_DNSCLIENT

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef CONFIG_NETDB_RESOLVCONF
/* The DNS server address */

union dns_server_u g_dns_server;
bool g_dns_address;     /* true: We have the address of the DNS server */
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dns_add_nameserver
 *
 * Description:
 *   Configure which DNS server to use for queries
 *
 ****************************************************************************/

#ifdef CONFIG_NETDB_RESOLVCONF
int dns_add_nameserver(FAR const struct sockaddr *addr, socklen_t addrlen)
{
  FAR FILE *stream;
  char line[DNS_MAX_LINE];
  int ret;

  stream = fopen(CONFIG_NETDB_RESOLVCONF, "at");
  if (stream == NULL)
    {
      int errcode = errno;
      ndbg("ERROR: Failed to open %s: %d\n",
           CONFIG_NETDB_RESOLVCONF, errcode);
      DEBUGASSERT(errcode > 0);
      return -errcode;
    }

#ifdef CONFIG_NET_IPv4
  /* Check for an IPv4 address */

  if (addr->sa_family == AF_INET)
    {
      if (socklen < sizeof(struct sockaddr_in))
        {
          ret = -EINVAL;
          goto errout;
        }
      else
        {
          FAR struct sockaddr_in *in4 = (FAR struct sockaddr_in *)addr;

          if (inet_ntop(AF_INET, &in4->sin_addr, line, DNS_MAX_LINE) == NULL)
            {
              ret = -errno;
              ndbg("ERROR: inet_ntop failed: %d\n", errcode);
              DEBUGASSERT(errcode < 0);
              goto errout;
            }
        }
    }
  else
#endif

#ifdef CONFIG_NET_IPv6
  /* Check for an IPv4 address */

  if (addr->sa_family == AF_INET6)
    {
      if (socklen < sizeof(struct sockaddr_in6))
        {
          ret = -EINVAL;
          goto errout;
        }
      else
        {
          FAR struct sockaddr_in6 *in6 = (FAR struct sockaddr_in6 *)addr;

          if (inet_ntop(AF_INET6, &in6->sin6_addr, line, DNS_MAX_LINE) == NULL)
            {
              ret = -errno;
              ndbg("ERROR: inet_ntop failed: %d\n", errcode);
              DEBUGASSERT(errcode < 0);
              goto errout;
            }
        }
    }
  else
#endif
    {
      nvdbg("ERROR: Unsupported family: %d\n",
            g_dns_server.addr.sa_family);
      ret = -ENOSYS;
      goto errout;
    }

  /* Write the new record to the end of the resolv.conf file */

  if (fprintf(stream, "%s %s\n", NETDB_DNS_KEYWORD, line) < 0)
    {
      ret = -errno;
      ndbg("ERROR: fprintf failed: %d\n", errcode);
      DEBUGASSERT(errcode < 0);
      goto errout;
    }

  ret = OK;

errout:
  fclose(stream);
  return ret;
}

#else /* CONFIG_NETDB_RESOLVCONF */

int dns_add_nameserver(FAR const struct sockaddr *addr, socklen_t addrlen)
{
  FAR uint16_t *pport;
  size_t copylen;

  DEBUGASSERT(addr != NULL);

  /* Copy the new server IP address into our private global data structure */

#ifdef CONFIG_NET_IPv4
  /* Check for an IPv4 address */

  if (addr->sa_family == AF_INET)
    {
      /* Set up for the IPv4 address copy */

      copylen = sizeof(struct sockaddr_in);
      pport   = &g_dns_server.ipv4.sin_port;
    }
  else
#endif

#ifdef CONFIG_NET_IPv6
  /* Check for an IPv6 address */

  if (addr->sa_family == AF_INET6)
    {
      /* Set up for the IPv6 address copy */

      copylen = sizeof(struct sockaddr_in6);
      pport   = &g_dns_server.ipv6.sin6_port;
    }
  else
#endif
    {
      nvdbg("ERROR: Unsupported family: %d\n", addr->sa_family);
      return -ENOSYS;
    }

  /* Copy the IP address */

  if (addrlen < copylen)
    {
      nvdbg("ERROR: Invalid addrlen %ld for family %d\n",
            (long)addrlen, addr->sa_family);
      return -EINVAL;
    }

  memcpy(&g_dns_server.addr, addr, copylen);

  /* A port number of zero means to use the default DNS server port number */

  if (*pport == 0)
    {
      *pport = HTONS(DNS_DEFAULT_PORT);
    }

  /* We now have a valid DNS address */

  g_dns_address = true;
  return OK;
#endif /* CONFIG_NETDB_RESOLVCONF */
}

#endif /* CONFIG_NETDB_DNSCLIENT */
