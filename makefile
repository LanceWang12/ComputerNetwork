src = client.cpp server.cpp
objs = client.o server.o
targ = server client

all : $(targ)
server : server.o 
	g++ -pthread -o server server.o 
client : client.o 
	g++ -pthread -o client client.o 
	
%.o : %.cpp %.h
	g++ -c $<

clean : 
	rm -f *.o $(targ) out* ?.mp4
