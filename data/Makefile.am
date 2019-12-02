systemdsysdir = $(libdir)/systemd/system
systemdsys_DATA = meshnet.service

configdir = $(sysconfdir)/meshnet
config_DATA = default.mesh.sample ifup.sample ifdown.sample

SERVICE_SUBS = \
	s,[@]sbindir[@],$(sbindir),g; \
	s,[@]configdir[@],$(configdir),g

meshnet.service: meshnet.service.in
	sed -e '$(SERVICE_SUBS)' < $< > $@

default.mesh.sample: default.mesh.sample.in
	sed -e '$(SERVICE_SUBS)' < $< > $@

