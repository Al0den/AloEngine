AlodEngine:
	gcc src/*.c -Ofast -o build/AloEngine.out 
run:
	gcc src/*.c -Ofast -o build/AloEngine.out && ./build/AloEngine.out
kill:
	killall -m AloEngine*
