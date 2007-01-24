noinst_SCRIPTS=mod_wsman.so

.c.so:
	apxs  $(OPENWSMAN_CFLAGS) -I../ -I. -DHAVE_CONFIG_H $(OPENWSMAN_LIBS) -c $< 

mod_wsman.so : mod_wsman.c

CLEANFILES=*~ *.o *.so


