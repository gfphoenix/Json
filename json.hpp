#ifndef _MYJSON_H
#define _MYJSON_H

#include <functional>
#include <string>
#include <vector>
#include <map>

namespace Json {
#define BUFSZ   4096
    class Value;
    class ObjectValue;
    class ArrayValue;
    class IntValue;
    class FloatValue;
    class NullValue;
    class NumberValue;
    class BoolValue;
//tokens
// { } [ ] , : "xxx"  true false null  123  12.3 unknown
// '{' '}' '[' ']' ',' ':' 's' 'b' 'b' 'n' 'i' 'd' '?'
// 分别对应以上各符号的返回值
// 需要保存的值：'i', 'd', 's'
    class Parser
    {
        private:
            typedef unsigned int size_t;
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
            Parser(const std::function<int(void *, unsigned)> &reader)
                : cap(0)
                  , pos(0)
                  , reader(reader){}
            ObjectValue *parse();
        protected:
            int getToken();
            int  eatSpace();
            void parseItem();
            Value* parseValue();
            ObjectValue *parseObject();
            ArrayValue *parseArray();
        private:
            bool literal(const char *s, int len);
            int parseTrue();
            int parseFalse();
            int parseNull();
            int parseString();
            int parseNumber();
            int parseInt();
            int parseFloat();
            bool parseExp(double &exp);
            Value *getValue(int token);
            int load();
            // >0 ok
            // =0 eof
            // <0 io error
            inline int checkAndLoad(){
                if(pos < cap-1 && cap>0)
                    return 1;
                return load();
            }
    };

    enum class Type {
        jType_UNKNOWN = 1,
        jType_OBJECT,
        jType_ARRAY,
        jType_BOOL,
        jType_INT,
        jType_FLOAT,
        jType_STRING,
        jType_NULL,
    };
    class Value {
        private:
            Type type;
        public:
            Value(Type t):type(t){}
            Type getType()const{return type;}
            virtual ~Value(){type = Type::jType_UNKNOWN;}
            virtual bool Encode(const std::function<int (const void *, unsigned)>& writer)const = 0;

            //check type
            bool isIntType()const {return type == Type::jType_INT;}
            bool isFloatType()const {return type == Type::jType_FLOAT;}
            bool isNumberType()const{return type == Type::jType_INT || type == Type::jType_FLOAT;}
            bool isBoolType()const {return type == Type::jType_BOOL;}
            bool isNullType()const {return type == Type::jType_NULL;}
            bool isStringType()const {return type == Type::jType_STRING;}
            bool isArrayType()const {return type == Type::jType_ARRAY;}
            bool isObjectType()const {return type == Type::jType_OBJECT;}
    };
    class NullValue : public Value {
        public:
            NullValue():Value(Type::jType_NULL){}
            bool Encode(const std::function<int(const void *, unsigned)> &writer)const{
                return writer("null", 4)==4;
            }
    };
    class BoolValue : public Value {
        private:
            bool _bool;
        public:
            BoolValue(bool b):Value(Type::jType_BOOL), _bool(b){}
            bool getValue()const{return _bool;}
            void setValue(bool b){_bool = b;}

        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class NumberValue : public Value {
        protected:
            NumberValue(Type t):Value(t){}
    };
    class IntValue : public NumberValue {
        private:
            long _v;
        public:
            IntValue(long v):NumberValue(Type::jType_INT), _v(v){}
            long getValue()const {return _v;}
            void setValue(long v){_v = v;}
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class FloatValue : public NumberValue {
        private:
            double _v;
        public:
            FloatValue(double v):NumberValue(Type::jType_FLOAT), _v(v){}
            double getValue()const {return _v;}
            void setValue(double v){_v = v;}
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class StringValue : public Value {
        private:
            std::string _v;
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
        public:
            StringValue(const char *s, size_t n):Value(Type::jType_STRING), _v(s,s+n){}
            StringValue(const std::string &v):Value(Type::jType_STRING), _v(v){}
            StringValue(std::string &&v):Value(Type::jType_STRING), _v(std::move(v)){}
            std::string getValue()const {return _v;}
            void setValue(const std::string &v){ _v = v;}
            void setValue(std::string && v){_v = std::move(v);}

    };
    //vector 操作的子集
    class ArrayValue : public Value {
        private:
            std::vector<Value*> _v;
        public:
            ~ArrayValue();
            ArrayValue(Value * v, ...);
            ArrayValue():Value(Type::jType_ARRAY){}
            ArrayValue(const std::vector<Value*> &v):Value(Type::jType_ARRAY),_v(v){}
            ArrayValue(std::vector<Value*> &&v)
                :Value(Type::jType_ARRAY), _v(std::move(v)){}
            Value *operator[](int idx) {
                if(idx<0 || idx>=(int)_v.size())
                    return nullptr;
                return _v[idx];
            }
            typedef std::vector<Value*>::iterator iterator;
            typedef std::vector<Value*>::const_iterator const_iterator;
            typedef std::vector<Value*>::reverse_iterator reverse_iterator;
            typedef std::vector<Value*>::const_reverse_iterator const_reverse_iterator;
            iterator begin(){return _v.begin();}
            iterator end(){return _v.end();}
            const_iterator begin()const {return _v.begin();}
            const_iterator end()const {return _v.end();}
            reverse_iterator rbegin(){return _v.rbegin();}
            reverse_iterator rend(){return _v.rend();}
            const_reverse_iterator rbegin()const{return _v.rbegin();}
            const_reverse_iterator rend()const {return _v.rend();}

            const_iterator cbegin()const{return _v.cbegin();}
            const_iterator cend()const{return _v.cend();}
            const_reverse_iterator crbegin()const{return _v.crbegin();}
            const_reverse_iterator crend()const{return _v.crend();}

            //end of iterator

            typedef std::vector<Value*>::size_type size_type;
            size_type size()const {return _v.size();}
            //add iterator
            void add(Value &);
            Value *del(size_t idx);
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class ObjectValue : public Value {
        private:
            std::map<std::string, Value*> _m;
        public:
            ~ObjectValue();
            ObjectValue():Value(Type::jType_OBJECT){}
            ObjectValue(const std::map<std::string, Value *> &map):Value(Type::jType_OBJECT),_m(map){}
            ObjectValue(std::map<std::string, Value *> &&map)
                :Value(Type::jType_OBJECT),_m(std::move(map)){}
            Value * operator[](const std::string &key){return _m[key];}

            typedef std::map<std::string, Value*>::iterator iterator;
            typedef std::map<std::string, Value*>::const_iterator const_iterator;
            typedef std::map<std::string, Value*>::reverse_iterator reverse_iterator;
            typedef std::map<std::string, Value*>::const_reverse_iterator const_reverse_iterator;
            iterator begin(){return _m.begin();}
            iterator end(){return _m.end();}
            const_iterator begin()const {return _m.begin();}
            const_iterator end()const {return _m.end();}
            reverse_iterator rbegin(){return _m.rbegin();}
            reverse_iterator rend(){return _m.rend();}
            const_reverse_iterator rbegin()const{return _m.rbegin();}
            const_reverse_iterator rend()const {return _m.rend();}

            const_iterator cbegin()const{return _m.cbegin();}
            const_iterator cend()const{return _m.cend();}
            const_reverse_iterator crbegin()const{return _m.crbegin();}
            const_reverse_iterator crend()const{return _m.crend();}

            typedef std::map<std::string, Value*>::size_type size_type;
            size_type size()const {return _m.size();}
            bool hasKey(const std::string &k){
                return _m[k] != nullptr;
            }
            void add(const std::string &key, Value &val);
            Value *del(const std::string &key);

            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };

    namespace {
        int defaultReader(void *buf, unsigned n)
        {
            return 0;
        }
        int defaultWriter(const void *buf, unsigned n)
        {
            printf("%s", buf);
            return n;
        }
    }

    class JsonDef {
        public:
            std::function<int (void *, unsigned)> R;
            std::function<int (const void *, unsigned)> W;
    };

    class Json {
        private:
            std::function<int (void *, unsigned)> reader;
            std::function<int (const void *, unsigned)> writer;
            ObjectValue *obj;
        public:
            Json(const JsonDef &def);
            ObjectValue *Decode()const;
            bool Encode()const{return obj->Encode(writer);}
    };
}
#endif
