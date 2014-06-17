#include <functional>
#include "json.h"
#include <string.h>
#include <string>

using namespace Json;

#define BUFSZ   4096
class Reader {
    public:
        bool isEnd()const;
};
//tokens
// { } [ ] , : "xxx"  true false null  123  12.3 unknown
// '{' '}' '[' ']' ',' ':' 's' 'b' 'b' 'n' 'i' 'd' '?'
// 分别对应以上各符号的返回值
// 需要保存的值：'i', 'd', 's'
enum class TokenType {
    OL = 1,
    OR,
    AL,
    AR,
    COMMA,
    COLON,
    s,
    b,
    n,
    i,
    f,
    UNKNOWN,
    eof,
};

class Parser
{
    private:
        typedef unsigned int size_t;
        const char *err;
        char buf[BUFSZ];
        int cap; // real active in [0, cap)
        int pos;
        std::function<int (void *, unsigned)> reader;
        union {
            long long   i;
            double      d;
            bool        b;
            struct {
                //最长4KB的字符串
                char s[4096];
                size_t len;
            };
        }data;
    public:
        Parser(const std::function<int(void *, unsigned)> &r)
            : pos(0)
              , cap(0)
              , err(nullptr)  
              , reader(r){}
        ObjectValue *parseObject();
        ArrayValue *parseArray();
        ObjectValue *parseObjectInternal();
    protected:
        int getToken();
        int  eatSpace();
        void parseItem();
        Value* parseValue();
    private:
        bool literal(const char *s, int len);
        int parseTrue();
        int parseFalse();
        int parseNull();
        int parseString();
        int parseNumber();
        int parseInt();
        int parseFloat();
        Value *getValue(int token);
        int load();
        int checkAndLoad();
};

// >0 读取到有效数据
// =0 文件已经读取完毕，没有可用数据
// <0 IO错误
int  Parser::load()
{
    int n;
    int left = BUFSZ;
    char *p = buf;
    pos = cap = 0;
    while(left >0 && (n = reader(p, left)>0)){
        cap += n;
        p  += n;
        left -= n;
    }
    if(cap > 0)
        return cap;
    return n;
}
// >0 ok
// =0 eof
// <0 io error
int Parser::checkAndLoad(){
    if(pos < cap-1)
        return 1;
    return load();
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
        //        token = getToken();
        if(token != 's')
            goto err_out;
        if(getToken()!=':')
            goto err_out;
        // how to make a key is better ?
        std::string str(this->data.s, this->data.len);
        Value *v;
        token = getToken();
        v = getValue(token);
        if(v == nullptr)
            goto err_out;

        obj->add(str, v);
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

int Parser::parseInt()
{
    if(cap - pos>=22){
        data.i = strtoll(&buf[pos], nullptr, 10);
        return 'i';
    }
    char tmp[22];
    memcpy(tmp, &buf[pos], cap - pos);
    return -1;
}

int Parser::parseFloat()
{
    return -1;
}

int Parser::parseNumber()
{
    return -1;
}


int Parser::getToken()
{
    int e = checkAndLoad();
    if(e<=0)
        return e;
    char c = buf[pos];
    switch(buf[pos]){
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
        case 't':
            return parseTrue();
        case 'f':
            return parseFalse();
        case 'n':
            return parseNull();
        case '"':
            return parseString();
        default:
            return parseNumber();
    }
}
ObjectValue *Parser::parseObjectInternal()
{
    int e = getToken();
    if(e<=0)
        return nullptr;
    if(e != '{'){
        return nullptr;
    }
    return parseObject();
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
        arr->add(v);

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

//
////make sure buf[pos] is '"'
//Value Parser::parseString()
//{
//
//    int cur = pos+1;
//    int start = pos + 1;
//    StringVal val;
//    bool b;
//    pos++;
//    int e = checkAndLoad();
//    if(e<0)
//        goto err_out;
//
//again:
//    while(cur<cap && buf[cur]!='"'){
//        cur++;
//    }
//    //
//    b = val.add(buf+start, buf+cur-1);
//    if(!b) // StringVal 分配内存失败，添加失败，可能是字符串太长
//        goto err_out;
//    if(cur<cap){
//        //碰到'"', 字符传解析完成
//        pos = cur+1;
//        e = checkAndLoad();
//        if(e<0)
//            goto err_out;
//        return val;
//    }
//    if(load()<=0)//IO err or end of file that need to be parsed as a string
//        goto err_out;
//    //accumulate string
//    //reset start count offset
//    start = cur = 0;
//    goto again;
//
//err_out:
//    return Value();
//}
//
//Value *Parser::parseString()
//{
//    const char *s = &buf[++pos];
//    while(buf[pos] != '"'){
//        if(buf[pos] == '\\'){
//            //do unscape
//        }
//        pos++;
//    }
//    return new StringVal(s, (size_t)(&buf[pos]-s));
//}
//
////buf[pos] = '['
//Value *Parser::parseArray()
//{
//    ArrayVal *val=new ArrayVal;
//    Value *v = nullptr;
//    bool isFirst = true;
//    pos++;
//    eatSpace();
//
//    while(1){
//        if(!isFirst && buf[pos]!=','){
//            //expect ','
//            err = "expected ',';
//            goto destroy;
//        }
//        isFirst = false;
//        eatSpace();
//        switch(buf[pos]){
//            case ']': //end of an array
//                pos++;
//                return val;
//            default:
//                v = parseValue();
//                break;
//        }
//        if(!v){
//            err = "bad array val";
//            goto destroy;
//        }
//        val.add(v);
//        eatSpace();
//        if(buf[pos] == ']'){
//            pos++;
//            return val;
//        }
//    }
//    // shouldn't go to here
//    err = "BUG : shouldn't go to here";
//
//destroy:
//    //for i in Array :
//    //  delete i
//    //
//    return null;
//}
//
//Value* Parser::parseValue()
//{
//    char c = buf[pos];
//    switch(c){
//        case '"' :
//            return parseString();// beginning with '"'
//        case '[' :
//            return parseArray(); // beginning with '['
//        case '{' :
//            return parseObject();
//        case 't' :
//            return parseTrue();
//        case 'f' :
//            return parseFalse();
//        case 'n' :
//            return parseNull();
//        default:
//            return parseNumber();
//    }
//}
//Value *Parser::parseTrue()
//{
//    if(parseLiteral("true"))
//        return new BoolVal(true);
//    err = "bad true val ?";
//    return nullptr;
//}
//Value *Parser::parseFalse()
//{
//    if(parseLiteral("false"))
//        return new BoolVal(false);
//    err = "bad false val ?";
//    return nullptr;
//}
//Value *Parser::parseNull()
//{
//    if(parseLiteral("null"))
//        return new Nullval;
//    err = "bad null val ?";
//    return  nullptr;
//}
//bool isDelimit(char x)
//{
//    bool b = false;
//    switch(x){
//        case ',':
//        case '"':
//        case ':':
//        case '[':
//        case ']':
//        case '{':
//        case '}':
//            b = true;
//        default:
//            b = isBlackSpace(x);
//    }
//    return b;
//}
//
//bool Parse::parseLiteral(const char *s)
//{
//    if(!strcmp(buf[pos], s))
//        return false;
//    pos += strlen(s);
//    return true;
//}
//Value *Parse::ParseString()
//{
//    if(buf[pos]!='"'){
//        err = "expected start string with `\"'";
//        return nullptr;
//    }
//    return parseString();
//}
//// buf[pos] = '{' now
//Value *Parser::parseObject()
//{
//    ObjectVal *obj = new ObjectVal;
//    StringVal *key;
//    Value *val;
//    pos++;
//    while(1){
//        eatSpace();
//        key = ParseString();
//        if(!key)
//            goto may_end;
//        eatSpace();
//        if(buf[pos] != ':')
//            goto err_out;
//        pos++;
//        eatSpace();
//        val = parseValue();
//        if(!val)
//            goto err_out;
//        obj->add(key, val);
//        eatSpace();
//        if(buf[pos] == '}'){
//            pos++;
//            return obj;
//        }
//        if(buf[pos]!=',')
//            goto err_out;
//        pos++;
//    }
//may_end:
//    if(buf[pos]=='}')
//        return obj;
//err_out:
//    err = "bad key or val in parsing obj";
//
//    // for i in obj :
//    //  delete key and val
//    //
//    return nullptr;
//}
//
//struct mystring {
//    char *str;
//    size_t len;
//};
//namespace json {
//    typedef std::string string;
//    typedef std::
