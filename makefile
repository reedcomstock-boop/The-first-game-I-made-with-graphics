CXX = g++
CFLAGS = -Wall -Wextra -pedantic -Werror -std=c++17 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lraylib

Game: main.o player.o updater.o NPC.o rooms.o items.o world.o GameLoop.o graphics.o sprite.o strip_animator.o roomscene.o tileset.o
	$(CXX) -o Game main.o player.o NPC.o rooms.o items.o world.o GameLoop.o updater.o graphics.o sprite.o strip_animator.o roomscene.o tileset.o $(LDFLAGS)

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
world.o: world.cpp
	$(CXX) -c world.cpp $(CFLAGS)
updater.o: updater.cpp
	$(CXX) -c updater.cpp $(CFLAGS)
GameLoop.o: GameLoop.cpp
	$(CXX) -c GameLoop.cpp $(CFLAGS)
graphics.o: graphics.cpp graphics.h
	$(CXX) -c graphics.cpp $(CFLAGS)
sprite.o: sprite.cpp
	$(CXX) -c sprite.cpp $(CFLAGS)
tileset.o: tileset.cpp tileset.h
	$(CXX) -c tileset.cpp $(CFLAGS)
strip_animator.o: strip_animator.cpp strip_animator.h
	$(CXX) -c strip_animator.cpp $(CFLAGS)
roomscene.o: roomscene.cpp roomscene.h
	$(CXX) -c roomscene.cpp $(CFLAGS)
clean:
	rm -f Game main.o player.o NPC.o rooms.o items.o world.o GameLoop.o updater.o graphics.o sprite.o strip_animator.o roomscene.o tileset.o 