#pragma once
#include <vector>
#include <string>
#include <memory>
using namespace std;
#pragma warning (disable: 4100)
#pragma warning (disable: 4389)
#pragma warning (disable: 4355)
#pragma warning (disable: 4996)

namespace BlmComm {

//把字符串按照分隔符切分
vector<string> splitString(const char* cont, char sep);
//按照ue中二进制的方式显示二进制数据的内容
string getHexDump(const void* data, int sz);
//生成随机内容
void randomBuf(void* data, int sz);
//返回格式化的字符串
string formatString(const char* fmt, ...);
//字符串的比较函数
int strcmpNoCase(const char* s1, const char* s2);
inline bool compareNoCase(const string& s1, const string& s2) { return strcmpNoCase(s1.c_str(), s2.c_str()) == 0; }
int strcmpCase(const char* s1, const char* s2);
inline bool compareCase(const string& s1, const string& s2) { return strcmpCase(s1.c_str(), s2.c_str()) == 0; }
string intToStr(int i);
string longToStr(long l);
string doubleToStr(double d);
int strToInt(const string& str);
long strToLong(const string& str);
double strToDouble(const string& str);
bool strToBool(const string& str);
string toLower(const string& str);
string toUpper(const string& str);
string& replaceAll(string& str, const string& oldVal, const string& newVal);
string intListToStr(vector<int> l, char delim);
string strListToStr(vector<string> l, char delim);
string strListToStr(vector<string> l, const string& delim);
vector<int> strToIntList(const string& str, char delim);
vector<long> strToLongList(const string& str, char delim);
vector<string> strToStrList(const string& str, char delim);
vector<string> strToStrList(const string& str, const string& delim);
string trimSpaces(const string& str) ;


//自动释放数组的内存
template<class C> class AutoArrayPtr {
	C* ptr;
public:
	AutoArrayPtr(C* p): ptr(p) {}
	~AutoArrayPtr() { delete [] ptr;}
};

//单例模式的模板类
template<class T> class Singleton {
public:
	//模板类不能放在cpp文件，因此放在这
	static T& instance(Singleton* ins=NULL) {
		static T* pins = (T*)ins;
		//如果未初始化，或者初始化了两次，则报错
		if (pins == NULL) {
			printf("singleton object was not created by user before used");
			throw 0;
		}else if(ins != NULL && ins != pins) {
			printf("singleton object is created twice");
			throw 0;
		}
		return *pins;
	}
	Singleton() { instance(this); }
};


}

