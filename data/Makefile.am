systemdsysdir = $(libdir)/systemd/system
systemdsys_DATA = meshnet.service

configdir = $(sysconfdir)/meshnet
config_DATA = default.mesh.sample

SERVICE_SUBS = \
	s,[@]sbindir[@],$(sbindir),g

meshnet.service: meshnet.service.in
	sed -e '$(SERVICE_SUBS)' < $< > $@

