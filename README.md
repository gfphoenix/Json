Json
====

a simple json decoder and encoder using flex++bison, 
This implementation is restricted by:
1) simple integer pattern [0-9]+;
2) simple float pattern [0-9]*\.[0-9]*;
3) simple string pattern ;
4) no error handler

Integer is store as long in C, float as double,
and string as C++'s std::string

With the powerful pattern matching, you can easily change the pattern for your
particular need.
