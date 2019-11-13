.PHONY: all
all: lab2server lab2client

lab2server: lab2server.C
	gcc -pthread -o lab2server lab2server.C

lab2client: lab2client.C
	gcc -o lab2client lab2client.C