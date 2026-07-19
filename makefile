CXX = g++
CFLAGS = -Wall -Wextra -pedantic -Werror -std=gnu++17 -I/ucrt64/include
LDFLAGS = -L/ucrt64/lib -lraylib -lopengl32 -lgdi32 -lwinmm
CXXFLAGS += -I.
CXXFLAGS += -Isrc

TARGET = Game.exe

OBJS = main.o player.o updater.o NPC.o rooms.o items.o world.o GameLoop.o graphics.o save.o

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

main.o: main.cpp
	$(CXX) -c main.cpp $(CFLAGS)
player.o: player.cpp player.h
	$(CXX) -c player.cpp $(CFLAGS)
NPC.o: NPC.cpp Entity.h player.h
	$(CXX) -c NPC.cpp $(CFLAGS)
rooms.o: rooms.cpp rooms.h items.h Entity.h
	$(CXX) -c rooms.cpp $(CFLAGS)
items.o: items.cpp
	$(CXX) -c items.cpp $(CFLAGS)
world.o: world.cpp world.h
	$(CXX) -c world.cpp $(CFLAGS)
updater.o: updater.cpp updater.h
	$(CXX) -c updater.cpp $(CFLAGS)
GameLoop.o: GameLoop.cpp GameLoop.h
	$(CXX) -c GameLoop.cpp $(CFLAGS)
graphics.o: graphics.cpp graphics.h
	$(CXX) -c graphics.cpp $(CFLAGS)
save.o: save.cpp save.h
	$(CXX) -c save.cpp $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJS)