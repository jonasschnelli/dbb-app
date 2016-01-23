package=eudev
$(package)_version=v3.1.5
$(package)_download_path=https://github.com/gentoo/eudev/archive/
$(package)_file_name=$($(package)_version).tar.gz
$(package)_sha256_hash=ce9d5fa91e3a42c7eb95512ca0fa2a631e89833053066bb6cdf42046b2a88553
$(package)_dependencies=native_gperf

define $(package)_set_vars
  $(package)_config_opts=--disable-gudev --disable-introspection --disable-hwdb --disable-manpages
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmd
  $(MAKE)
endef

define $(package)_preprocess_cmds
  cd $($(package)_build_subdir); autoreconf -f -i
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
