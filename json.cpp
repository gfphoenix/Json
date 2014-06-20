#include "json.hpp"
#include <cstdarg>
#include <stdio.h>
#include <string.h>

using namespace Json;

bool BoolValue::Encode(const std::function<int (const void *, unsigned)>& writer)const
{
#define     _J_TRUE    "true"
#define     _J_FALSE    "false"
    int ret;
    if(_bool){
        ret = writer(_J_TRUE , sizeof(_J_TRUE)-1);
        return ret == sizeof(_J_TRUE)-1;
    }
    ret = writer(_J_FALSE , sizeof(_J_FALSE)-1);
    return ret == sizeof(_J_FALSE)-1;
#undef      _J_TRUE
#undef      _J_FALSE
}
bool IntValue::Encode(const std::function<int (const void *, unsigned)>& writer)const
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%ld", _v);
    return writer(buf, strlen(buf)) == n;
}
bool FloatValue::Encode(const std::function<int (const void *, unsigned)>& writer)const
{
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "%lf", _v);
    return writer(buf, strlen(buf)) == n;
}
bool StringValue::Encode(const std::function<int (const void *, unsigned)>& writer)const
{
    char buf[(unsigned)_v.size()+4];
    int n = sprintf(buf, "\"%s\"", _v.c_str());
    return n==(int)_v.size()+2 && writer(buf, n) == n;
}
bool ArrayValue::Encode(const std::function<int (const void *, unsigned)>& writer)const 
{
    auto sz = _v.size();
    int e;
    e = writer("[", 1);
    if(e != 1)
        return false;
    if(sz == 0)
        goto out;

    if(! _v[0]->Encode(writer))
        return false;
    for(auto x = _v.cbegin()+1; x != _v.cend(); x++){
        if(writer(",", 1) != 1)
            return false;
        if(!(*x)->Encode(writer))
            return false;
    }
out:
    e = writer("]", 1);
    if(e != 1)
        return false;
    return true;
}

bool ObjectValue::Encode(const std::function<int (const void *, unsigned)>& writer)const
{
    bool first = true;
    int e;
    e = writer("{\n", 2);
    if(e != 2)
        return false;
    if(_m.size() == 0)
        goto out;

    for(auto x = _m.cbegin(); x != _m.cend(); x++){
        auto &k = x->first;
        auto &v = x->second;
        if(!first){
            e = writer(",\n", 2);
            if(e != 2)
                return false;
        }
        first = false;
        char buf[(unsigned)k.size() + 8];
        int n = snprintf(buf, (unsigned)k.size()+8, "\"%s\" : ", k.c_str());
        e = writer(buf, n);
        if(e!=n || e != (int)k.size()+5)
            return false;
        auto ok = v->Encode(writer);
        if(!ok)
            return false;
    }
out:
    e = writer("\n}", 2);
    if(e != 2)
        return false;
    return true;
}

    ArrayValue::ArrayValue(Value *first, ...)
:Value(Type::jType_ARRAY)
{
    Value *x;
    va_list ap;
    for(x=first, va_start(ap, first); x!=nullptr; x = va_arg(ap, Value*)){
        _v.push_back(x);
    }
    va_end(ap);
}
ArrayValue::~ArrayValue()
{
    for(auto x : _v)
        delete x;
}
ObjectValue::~ObjectValue()
{
    for(auto x : _m)
        delete x.second;
}
// ==============================================================================================
// add, del
void ArrayValue::add(Value &v)
{
    _v.push_back(&v);
}
Value *ArrayValue::del(size_t idx)
{
    if(idx>= _v.size())
        return nullptr;
    auto tmp = _v.begin()+idx;
    _v.erase(tmp);
    return *tmp;
}

void ObjectValue::add(const std::string &key, Value &val)
{
    auto v = _m[key];
    _m[key] = &val;
    delete v;
}
Value *ObjectValue::del(const std::string &key)
{
    auto u = _m.find(key);
    if(u!=_m.end()){
        _m.erase(u);
    }
    return u->second;
}

