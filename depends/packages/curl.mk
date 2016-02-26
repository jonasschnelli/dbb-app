package=curl
$(package)_version=7.46.0
$(package)_download_path=http://curl.haxx.se/download
$(package)_file_name=$(package)-$($(package)_version).tar.gz
$(package)_sha256_hash=df9e7d4883abdd2703ee758fe0e3ae74cec759b26ec2b70e5d1c40239eea06ec
$(package)_patches=darwinssl.patch
$(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -DCURL_STATICLIB"
linux_$(package)_dependencies=openssl
define $(package)_preprocess_cmds
  patch -p1 < $($(package)_patch_dir)/darwinssl.patch && autoreconf -i
endef

define $(package)_set_vars
  $(package)_build_env+=CFLAGS="$($(package)_cflags) $($(package)_cppflags) -DCURL_STATICLIB"
  $(package)_config_opts=-disable-shared --enable-static --disable-ftp --disable-ldap --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-file --without-libssh2 --without-libidn --without-librtmp --without-libpsl --without-ldap --without-libmetalink --without-zlib --with-ssl=$(host_prefix)
  $(package)_config_opts_linux=--with-pic
  $(package)_config_opts_darwin=--with-darwinssl
	$(package)_config_opts_mingw32=--without-ssl --with-winssl
endef

define $(package)_config_cmds
  $($(package)_autoconf)
endef

define $(package)_build_cmd
  $(MAKE) $($(package)_build_opts)
endef

define $(package)_stage_cmds
  $(MAKE) DESTDIR=$($(package)_staging_dir) install
endef
