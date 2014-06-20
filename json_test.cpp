#include "json.hpp"
#include <functional>
#include <unistd.h>
#include <string.h>

int myR(void *b, unsigned n)
{
    return read(0, b, 1);
}
int myW(const void *p, unsigned n)
{
    return write(1, p, n);
}
int myWriter(char *&out, const void *data, unsigned len)
{
    memcpy(out, data, len);
    out += len;
    return len;
}

char out[4096];
char *p=out;
int main()
{
    Json::ObjectValue *obj;
    Json::Parser parser(std::bind(myR, std::placeholders::_1, std::placeholders::_2));

    obj = parser.parse();
    if(obj != nullptr){
        //obj->Encode(std::bind(myW, std::placeholders::_1, std::placeholders::_2));
        obj->Encode(std::bind(myWriter, p, std::placeholders::_1, std::placeholders::_2));
        printf("%s\n", out);
    }
    return 0;
}
