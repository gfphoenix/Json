#include <functional>
#include "json.hpp"
#include <string.h>
#include <stdlib.h>
#include <string>
#include <cmath>

using namespace Json;

// >0 读取到有效数据
// =0 文件已经读取完毕，没有可用数据
// <0 IO错误
int  Parser::load()
{
    int n;
    int left = BUFSZ;
    char *p = buf;
    pos = cap = 0;
    while(left >0 && ((n = reader(p, left))>0)){
        cap += n;
        p  += n;
        left -= n;
    }
    if(cap > 0)
        return cap;
    return n;
}
inline bool isBlackSpace(char c)
{
    return c==' ' || c=='\n' || c=='\t' || c=='\r' || c=='\v';
}
// >0 ok, left non-blank char
// =0 eof
// <0 io error
int  Parser::eatSpace()
{
again:
    int e = checkAndLoad();
    if(e<=0)
        return e;
    while(pos<cap){
        if(!isBlackSpace(buf[pos]))
            break;
        pos++;
    }
    if(pos == cap)
        goto again;
    return 1;
}

// 根据token值，获取Value
Value *Parser::getValue(int token)
{
    switch(token){
        case 'i':
            return new IntValue(data.i);
        case 'f':
            return new FloatValue(data.d);
        case 'b':
            return new BoolValue(data.b);
        case 'n':
            return new NullValue();
        case 's':
            return new StringValue(data.s, data.len);
        case '{':
            return parseObject();
        case '[':
            return parseArray();
        default:
            return nullptr;
    }
}

// token '{' is already eaten
ObjectValue *Parser::parseObject()
{
    ObjectValue *obj = new ObjectValue;
    int token;
    while((token=getToken())!='}'){
        //expect key : value
        if(token != 's')
            goto err_out;
        if(getToken()!=':')
            goto err_out;
        // how to make a key is better ?
        std::string str(this->data.s, this->data.len);
        token = getToken();
        auto v = getValue(token);
        if(v == nullptr)
            goto err_out;

        obj->add(str, *v);
        //test if the next token is ','
        token = getToken();
        if(token != ',')
            break;
    }

    if(token == '}')
        return obj;
err_out:
    //clean data;
    //  clean object's key and value
    delete obj;
    return nullptr;
}

bool Parser::literal(const char *s, int len)
{
    int left = cap - pos;
    int r;
    if(left>=len){
        r = memcmp(s, &buf[pos], len);
        if(r)
            return false;
        pos += len;
        return checkAndLoad()>0 ? true : false;
    }
    r = memcmp(s, &buf[pos], left);
    if(r)
        return false;
    if(load()<=0)
        return false;
    if(cap - pos < len-left)
        return false;
    r = memcmp(s+left, &buf[pos], len-left);
    if(r)
        return false;
    pos += len-left;
    return true;
}

int Parser::parseTrue()
{
    if(!literal("true", 4))
        return -1;
    data.b = true;
    return 'b';
}
int Parser::parseFalse()
{
    if(!literal("false", 5))
        return -1;
    data.b = false;
    return 'b';
}
int Parser::parseNull()
{
    if(!literal("null", 4))
        return -1;
    return 'n';
}

//buf[pos] = '"'
int Parser::parseString()
{
    pos++;
    if(checkAndLoad()<=0)
        return -1;
    char *p = data.s;
    data.len = 0;
    while(buf[pos] != '"'){
        int left = cap - pos;
        int i;
        for(i=0; i<left; i++){
            if(buf[pos+i] == '"'){
                memcpy(p, &buf[pos], i);
                p += i;
                data.len += i;
                pos += i+1;

                return checkAndLoad() > 0 ? 's' : -1;
            }
        }
        memcpy(p, &buf[pos], left);
        p += left;
        data.len += left;
        if(load()<=0)
            return -1;
    }
    pos++;
    return checkAndLoad() > 0 ? 's' : -1;
}

bool Parser::parseExp(double &ret)
{
    bool neg = false;
    int e;
    pos++;
    e = checkAndLoad();
    if(e<=0)
        return e;
    if(buf[pos]=='-'){
        neg = true;
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
    }
    long v=0;
    if(buf[pos]<'0' || buf[pos]>'9')
        return false;
    while(buf[pos]>='0' && buf[pos]<='9'){
        v = v*10 + buf[pos]-'0';
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
    }
    if(neg)
        v = -v;
    ret = pow(10, (double)v);
    return true;
}

int  Parser::parseInt()
{
    bool neg = false;
    int e;
    if(buf[pos]=='-'){
        neg = true;
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
    }
    data.i = 0;
    if(buf[pos]<'0' || buf[pos]>'9')
        return -1;
    if(buf[pos]=='0'){
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
        return 'i';
    }
    do{
        data.i = data.i * 10 + buf[pos]-'0';
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
    }while(buf[pos]>='0' && buf[pos]<='9');

    if(neg)
        data.i = -data.i;
    return 'i';
}

//整数部分在data.i
//当前符号是'.'
int  Parser::parseFloat()
{
    double v=0.0;
    double x=0.1;
    int e;
    pos++;
    e = checkAndLoad();
    if(e<=0)
        return e;
    if(buf[pos]<'0' || buf[pos]>'9')
        return -1;
    while(buf[pos]>='0' && buf[pos]<='9'){
        v = v + x*(buf[pos]-'0');
        x /= 10;
        pos++;
        e = checkAndLoad();
        if(e<=0)
            return e;
    }
    if(data.i<0){
        data.d = -(v-data.i);
    }else{
        data.d = data.i + v;
    }
    if(buf[pos]!='e' || buf[pos]!='E')
        return 'f';
    double tmp;
    auto b = parseExp(tmp);
    if(!b)
        return -1;
    data.d = data.d * tmp;
    return 'f';
}

int Parser::parseNumber()
{
    int e ;
    e = parseInt();
    if(e<=0)
        return e;
    e = checkAndLoad();
    if(e<=0)
        return e;
    switch(buf[pos]){
        case '.':
            e = parseFloat();
            break;
        case 'e':
        case 'E':
            {
                double tmp;
                bool b = parseExp(tmp);
                if(b){
                    data.d = data.i * tmp;
                    e = 'f';
                }else{
                    e = -1;
                }
                break;
            }
        default:
            return 'i';
    }
    return e;
}

int Parser::getToken()
{
again:
    int e = checkAndLoad();
    if(e<=0)
        return e;
    char c = buf[pos];
    switch(buf[pos]){
        //blank space
        case ' ':
        case '\t':
        case '\n':
        case '\r':
        case '\v':
            pos++;
            goto again;
        case '{':
        case '}':
        case '[':
        case ']':
        case ',':
        case ':':
            pos++;
            if(checkAndLoad()<0)
                return -1;
            return c;
        case 't': return parseTrue();
        case 'f': return parseFalse();
        case 'n': return parseNull();
        case '"': return parseString();
        default:
                  return parseNumber();
    }
}
ObjectValue *Parser::parse()
{
    int e = getToken();
    if(e<=0)
        return nullptr;
    if(e == '{'){
        return parseObject();
    }
    return nullptr;
}

// '[' has parsed already
ArrayValue *Parser::parseArray()
{
    ArrayValue *arr = new ArrayValue;
    int token;
    while((token=getToken())!=']'){
        Value *v;
        v = getValue(token);
        if(v == nullptr)
            goto err_out;
        arr->add(*v);

        //test if the next token is ','
        token = getToken();
        if(token != ',')
            break;
    }
    if(token == ']')
        return arr;
err_out:
    //clean data;
    //  clean array's data and itself
    delete arr;
    return nullptr;
}
