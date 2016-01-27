package=zlib
$(package)_version=1.2.8
$(package)_download_path=http://zlib.net/
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=36658cb768a54c1d4dec43c3116c27ed893e88b02ecfcb44f2166f9c0b7f2a0d

define $(package)_set_vars
  $(package)_config_env+=CHOST=$($($(package)_type)_host)
endef

define $(package)_config_cmds
	echo "--> $(MAKE)"
	./configure --prefix=$($($(package)_type)_prefix)
endef

define $(package)_build_cmd
  $(MAKE) -f win32/Makefile.gcc
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
