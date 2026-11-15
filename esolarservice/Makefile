CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
LDLIBS  = -lm -lpthread

esolard: esolard.c WebPages.h icon.h
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

# program + service; the config file only the first time (never overwrites your settings)
install: esolard
	install -m 755 esolard /usr/local/bin/esolard
	install -m 644 esolard.service /etc/systemd/system/esolard.service
	test -e /etc/esolard.conf || install -m 600 esolard.conf /etc/esolard.conf
	systemctl daemon-reload

clean:
	rm -f esolard
