#ifndef _MYJSON_H
#define _MYJSON_H

#include <functional>
#include <string>
#include <vector>
#include <map>

namespace Json {
    enum class Type {
        jType_UNKNOWN = 1,
        jType_OBJECT,
        jType_ARRAY,
        jType_BOOL,
        jType_INT,
        jType_FLOAT,
        jType_STRING,
    };
    class Value {
        public:
            Type type;
            Value(Type t):type(t){}
            virtual ~Value(){type = Type::jType_UNKNOWN;}
            virtual bool Encode(const std::function<int (const void *, unsigned)>& writer)const = 0;

            //check type
            bool isIntType()const {return type == Type::jType_INT;}
            bool isFloatType()const {return type == Type::jType_FLOAT;}
            bool isBoolType()const {return type == Type::jType_BOOL;}
            bool isStringType()const {return type == Type::jType_STRING;}
            bool isArrayType()const {return type == Type::jType_ARRAY;}
            bool isObjectType()const {return type == Type::jType_OBJECT;}
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
    class IntValue : public Value {
        private:
            long _v;
        public:
            IntValue(long v):Value(Type::jType_INT), _v(v){}
            long getValue()const {return _v;}
            void setValue(long v){_v = v;}
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class FloatValue : public Value {
        private:
            double _v;
        public:
            FloatValue(double v):Value(Type::jType_FLOAT), _v(v){}
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
            StringValue(const std::string &v):Value(Type::jType_STRING), _v(v){}
            StringValue(std::string &&v):Value(Type::jType_STRING), _v(std::move(v)){}
            std::string getValue()const {return _v;}
            void setValue(const std::string &v){ _v = v;}
            void setValue(std::string && v){_v = std::move(v);}

    };
    class ArrayValue : public Value {
        private:
            std::vector<Value*> _v;
        public:
            //          ArrayValue(Value * v, ...);
            ArrayValue(const std::vector<Value*> &v):Value(Type::jType_ARRAY),_v(v){}
            ArrayValue(std::vector<Value*> &&v)
                :Value(Type::jType_ARRAY), _v(std::move(v)){}
            Value *operator[](int idx) {
                if(idx<0 || idx>=_v.size())
                    return nullptr;
                return _v[idx];
            }
            //add iterator
        protected:
            bool Encode(const std::function<int (const void *, unsigned)>& writer)const ;
    };
    class ObjectValue : public Value {
        private:
            std::map<std::string, Value*> m;
        public:
            ObjectValue():Value(Type::jType_OBJECT){}
            ObjectValue(const std::map<std::string, Value *> &map):Value(Type::jType_OBJECT),m(map){}
            ObjectValue(std::map<std::string, Value *> &&map)
                :Value(Type::jType_OBJECT),m(std::move(map)){}
            Value * operator[](const std::string &key){return m[key];}

            bool hasKey(const std::string &k){
                return m[k] != nullptr;
            }

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
            //Json(const std::function<int(void *, unsigned)> &r = defaultReader,
            //        const std::function<int(const void *, unsigned)> &w=defaultWriter);
            ObjectValue *Decode()const;
            bool Encode()const{return obj->Encode(writer);}
    };
}
#endif
