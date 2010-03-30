all : $(top_builddir)/src/mod_wsman.la

EXTRA_DIST += src/mod_wsman.c
CLEANFILES += src/*.lo src/*.o src/*.la src/*.slo

$(top_builddir)/src/mod_wsman.la : $(top_srcdir)/src/mod_wsman.c
	$(APXS) -c $(CFLAGS) $^ $(LDLIBS) -Wc,-Wall -Wc,-Werror -o $@

$(DESTDIR)$(LIBEXECDIR) :
	mkdir -p $@

install-exec-hook: all $(DESTDIR)$(LIBEXECDIR)
	$(APXS) -i -S LIBEXECDIR=$(DESTDIR)$(LIBEXECDIR) -n 'mod_wsman' $(top_builddir)/src/mod_wsman.la
