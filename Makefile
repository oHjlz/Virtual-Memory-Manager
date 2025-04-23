
CFLAGS = -Wall -Wextra -g

#target
vm: vm.c
	gcc $(CFLAGS) -o vm vm.c

clean:
	rm -f vm