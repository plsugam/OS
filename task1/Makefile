all: frontend backend

frontend: frontend.c
	gcc -Wall -o frontend frontend.c

backend: backend.c
	gcc -Wall -o backend backend.c

clean:
	rm -f frontend backend /tmp/auth.sock
