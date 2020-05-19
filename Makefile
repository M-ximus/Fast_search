CFLAGS = -MD
DEBUG_FLAGS = -ggdb

all: search.out

run_table_test: test.out
	./test.out

debug: debug.out

debug.out: Rabin.с Hash.с
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -o $@ $^

search.out: Rabin.o Hash.o
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

-include *.d

.PHONY: clean

clean:
	rm *.o *.out *.d
