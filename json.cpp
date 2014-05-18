#include "json.h"
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
    return n==_v.size()+2 && writer(buf, n) == n;
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
    int e;
    e = writer("{\n", 2);
    if(e != 2)
        return false;
    if(_m.size() == 0)
        goto out;

    for(auto x = _m.cbegin(); x != _m.cend(); x++){
        auto &k = x->first;
        auto &v = x->second;

        char buf[(unsigned)k.size() + 8];
        int n = snprintf(buf, (unsigned)k.size()+8, "\"%s\" : ", k.c_str());
        e = writer(buf, n);
        if(e!=n || e != (int)k.size()+5)
            return false;
        auto ok = v->Encode(writer);
        if(!ok)
            return false;
        e = writer(",\n", 2);
        if(e != 2)
            return false;
    }
out:
    e = writer("}", 1);
    if(e != 1)
        return false;
    return true;
}

// ==============================================================================================
// ############## end of encode

