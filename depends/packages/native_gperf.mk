package=native_gperf
$(package)_version=3.0.4
$(package)_download_path=http://ftp.gnu.org/pub/gnu/gperf/
$(package)_file_name=gperf-$($(package)_version).tar.gz
$(package)_sha256_hash=767112a204407e62dbc3106647cf839ed544f3cf5d0f0523aaa2508623aad63e

define $(package)_set_vars
  $(package)_config_opts=--disable-manpages
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
