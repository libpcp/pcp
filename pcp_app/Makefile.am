AM_CPPFLAGS = $(PCP_CPPFLAGS)
AM_CFLAGS = $(PCP_CFLAGS)

bin_PROGRAMS = pcp

pcp_SOURCES = pcp_app.c
pcp_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/libpcp/include -I$(top_srcdir)/libpcp/src/net -I$(top_srcdir)/libpcp/src
pcp_LDADD = $(top_builddir)/libpcp/libpcp-client.la
pcp_LDFLAGS = -static

