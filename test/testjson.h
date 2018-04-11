#include "../src/data/json/rapidjson/document.h"
#include "../src/data/json/rapidjson/writer.h"
#include "../src/data/json/rapidjson/stringbuffer.h"
#include <iostream>
using namespace rapidjson;



void __testJson()
{
    // 1. 
    const char* json = "{\"project\":\"rapidjson\",\"stars\":10}";
    Document d;
    d.Parse(json);
    // 2.
    Value& s = d["stars"];
    s.SetInt(s.GetInt() + 1);
    // 3.
    StringBuffer buffer;
    Writer<StringBuffer> writer(buffer);
    d.Accept(writer);
    // Output {"project":"rapidjson","stars":11}
    std::cout << buffer.GetString() << std::endl;
}
