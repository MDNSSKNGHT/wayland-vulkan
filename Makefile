CFLAGS				=	-g3 -Werror
SRC					=	main.c xdg-shell-protocol.c
LIBS				=	\
		$(shell pkg-config --cflags --libs wayland-client)

xdg-shell-protocol.c:
	wayland-scanner private-code \
		/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		xdg-shell-protocol.c

xdg-shell-client-protocol.h:
	wayland-scanner client-header \
		/usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
		xdg-shell-client-protocol.h

main: main.c xdg-shell-protocol.c xdg-shell-client-protocol.h
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
	rm -f main xdg-shell-protocol.c xdg-shell-client-protocol.h

.DEFAULT_GOAL=main
.PHONY: clean
