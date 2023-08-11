all: app
app: app.o
	gcc -o app app.o
app.o: app.c
	gcc -c app.c -std=gnu99 
clean:
	rm app.o app