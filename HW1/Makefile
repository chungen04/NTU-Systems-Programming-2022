all:
	gcc server.c -D WRITE_SERVER -o write_server
	gcc server.c -D READ_SERVER -o read_server
write:
	gcc server.c -D WRITE_SERVER -o write_server
read:
	gcc server.c -D READ_SERVER -o read_server
clean:
	rm write_server
	rm read_server