phpdbg: $(SAPI_PHPDBG_PATH)

phpdbg-shared: $(SAPI_PHPDBG_SHARED_PATH)

$(SAPI_PHPDBG_SHARED_PATH): $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_PHPDBG_OBJS)
	$(BUILD_PHPDBG_SHARED)

$(SAPI_PHPDBG_PATH): $(PHP_GLOBAL_OBJS) $(PHP_BINARY_OBJS) $(PHP_PHPDBG_OBJS)
	$(BUILD_PHPDBG)

%.c: %.y
%.c: %.l

$(builddir)/phpdbg_lexer.lo: $(srcdir)/phpdbg_parser.h

$(srcdir)/phpdbg_lexer.c: $(srcdir)/phpdbg_lexer.l
	@(cd $(top_srcdir); $(RE2C) $(RE2C_FLAGS) -cbdFo sapi/phpdbg/phpdbg_lexer.c sapi/phpdbg/phpdbg_lexer.l)

$(srcdir)/phpdbg_parser.h: $(srcdir)/phpdbg_parser.c
$(srcdir)/phpdbg_parser.c: $(srcdir)/phpdbg_parser.y
	@$(YACC) $(YFLAGS) -v -d $(srcdir)/phpdbg_parser.y -o $@

install-phpdbg: $(SAPI_PHPDBG_PATH)
	@echo "Installing phpdbg binary:         $(INSTALL_ROOT)$(bindir)/"
	@$(mkinstalldirs) $(INSTALL_ROOT)$(bindir)
	@$(mkinstalldirs) $(INSTALL_ROOT)$(localstatedir)/log
	@$(mkinstalldirs) $(INSTALL_ROOT)$(localstatedir)/run
	@$(INSTALL) -m 0755 $(SAPI_PHPDBG_PATH) $(INSTALL_ROOT)$(bindir)/$(program_prefix)phpdbg$(program_suffix)$(EXEEXT)
	@echo "Installing phpdbg man page:       $(INSTALL_ROOT)$(mandir)/man1/"
	@$(mkinstalldirs) $(INSTALL_ROOT)$(mandir)/man1
	@$(INSTALL_DATA) sapi/phpdbg/phpdbg.1 $(INSTALL_ROOT)$(mandir)/man1/$(program_prefix)phpdbg$(program_suffix).1
