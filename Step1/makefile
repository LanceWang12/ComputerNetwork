src = client.cpp server.cpp main.cpp TCP.cpp
objs = client.o server.o main.o TCP.o
targ = server client

all : $(targ)
	clear
server : server.o TCP.o
	g++ -pthread -o server server.o TCP.o
client : client.o TCP.o
	g++ -pthread -o client client.o TCP.o
	
%.o : %.cpp %.h
	g++ -c $<

clean : 
	rm -f *.o $(targ) out*
