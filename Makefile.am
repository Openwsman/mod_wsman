APXS = @APXS@
DESTDIR =
CFLAGS =   @OPENWSMAN_CFLAGS@
LDLIBS =   @OPENWSMAN_LIBS@
LIBEXECDIR = $(shell $(APXS) -q LIBEXECDIR)

EXTRA_DIST = bootstrap pkg/wsman.conf pkg/mod_wsman.spec
CLEANFILES = src/.libs

# override builtin clean-generic to to 'rm -rf'
clean-generic:
	-test -z "$(CLEANFILES)" || rm -rf $(CLEANFILES)

AUTOMAKE_OPTIONS = subdir-objects
include src/Makefile.am

# build rpm
TOPDIR := $(shell cd $(top_builddir);pwd)
BUILDDIR = $(TOPDIR)/_rpmbuild
RPMDIR = $(TOPDIR)
SOURCEDIR = $(TOPDIR)
SPECFILE= $(TOPDIR)/pkg/$(PACKAGE_NAME).spec
SPECDIR = $(TOPDIR)/pkg
SRCRPMDIR = $(TOPDIR)

RPM_DEFINES = --define "_topdir $(TOPDIR)" \
		--define "_builddir $(BUILDDIR)" \
		--define "_rpmdir $(RPMDIR)" \
		--define "_sourcedir $(SOURCEDIR)" \
		--define "_specdir $(SPECDIR)" \
		--define "_srcrpmdir $(SRCRPMDIR)"

.PHONY: rpm srpm
rpm: dist
	mkdir -p $(BUILDDIR)
	rpmbuild $(RPM_DEFINES) -ba --nodeps $(SPECFILE)
	rm -rf $(BUILDDIR)

srpm: dist
	mkdir -p $(BUILDDIR)
	rpmbuild $(RPM_DEFINES) -bs --nodeps $(SPECFILE)
	rm -rf $(BUILDDIR)



