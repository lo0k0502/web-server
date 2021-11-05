FLAGS = -I include
OBJS = \
		src/main.o \
		src/utility.o \

EXEC = webserver

src/%.o: src/%.c
	gcc $(FLAGS) -c $< -o $@

$(EXEC): $(OBJS)
	gcc $(FLAGS) $(OBJS) -o $@

all: $(EXEC)

run: $(EXEC)
	./$(EXEC)

clean: rm $(EXEC) $(OBJS)