CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -pthread -Iinclude
LDFLAGS = -lrt -lpthread

SRCS    = process_manager.c thread_manager.c ipc_module.c main_sim.c
TARGET  = eduos

.PHONY: all clean race fixed memcheck

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile WITHOUT mutex to demonstrate race condition
race:
	$(CC) $(CFLAGS) -DRACE_MODE -o eduos_race $(SRCS) $(LDFLAGS)
	@echo "=== Running race demo 5 times ==="
	@for i in 1 2 3 4 5; do ./eduos_race 2>/dev/null | grep "Expected\|Got"; done

# Compile WITH mutex (default)
fixed:
	$(CC) $(CFLAGS) -o eduos_fixed $(SRCS) $(LDFLAGS)
	@echo "=== Running fixed demo ==="
	@./eduos_fixed 2>/dev/null | grep "Expected\|Got"

memcheck: $(TARGET)
	valgrind --leak-check=full --error-exitcode=1 ./$(TARGET)

clean:
	rm -f $(TARGET) eduos_race eduos_fixed pcb_snapshot.json *.o
