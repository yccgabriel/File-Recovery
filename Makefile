all: clean recover

recover:
	@ g++ -std=c++0x *.cpp -o recover
	@ echo "Make completed, run \"./recover\"\n"
#	 ./recover -d /dev/ram1 -R asdfhwjeidhjabjifdsahgivwdhabivjwdfiskhgvjdwiasfkdsahjvdkwanocskdgnv.txt -o test.txt

clean:
	@ rm -f recover