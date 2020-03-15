
CC = gcc
SRC = procsim.c procsim_driver.c
INCLUDE = procsim.h
CFLAGS := -g -Wall -Wextra -std=c99

build:
	$(CC) $(CFLAGS) $(SRC) $(INCLUDE) -o procsim

clean:
	rm -f procsim *.o

# This looks for a report pdf in the top level directory of the project
.PHONY: submit

submit:
	tar -cvzf ../../project2-submit.tar.gz procsim.c procsim_driver.c procsim.h \
				Makefile $(PWD)/../../report.pdf
