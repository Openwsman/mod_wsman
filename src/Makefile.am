noinst_SCRIPTS=mod_wsman.so

.c.so:
	apxs  $(OPENWSMAN_CFLAGS) $(OPENWSMAN_LIBS) -c $< 

mod_wsman.so : mod_wsman.c

CLEANFILES=*~ *.o *.so


