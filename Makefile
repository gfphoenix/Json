SRCS	:= json.tab.cxx lex.yy.cc json.cpp
OBJS	:= json.tab.o  lex.yy.o json.o
FLAGS 	+= -c -std=c++11
CC := g++

all: $(OBJS)
	$(CC) $^

OBJS: $(SRCS)

json.o	:	json.cpp
	$(CC) $(FLAGS) $<
json.tab.o: json.tab.cxx
	$(CC) $(FLAGS) $<
lex.yy.o:	lex.yy.cc
	$(CC) $(FLAGS) $<

$(SRCS):	bison flex

bison: json.yxx
	bison -d $<
flex:	json.l bison
	flex++ $<

clean:
	$(RM)	$(OBJS)  *.tab.* lex.yy.* a.out $(PROG)
