.PHONY:all
all:insert select

insert:insert.c
	gcc -o $@ $^ -I ./include -L ./lib -lmysqlclient

select:select.c
	gcc -o $@ $^ -I ./include -L ./lib -lmysqlclient

.PHONY:clean

clean:
	rm -f insert select
