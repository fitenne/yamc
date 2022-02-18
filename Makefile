
yamc:
	g++ src/**.cpp -o build/yamc -g -std=c++17 -lstdc++fs -lglog -DDEBUG -Wall -Wextra #-fsanitize=address,undefined
