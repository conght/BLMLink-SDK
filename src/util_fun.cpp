#include <blm/util_fun.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
namespace BlmComm {

vector<string> splitString(const char* cont, char sep) {
	vector<string> res;
	string cur;
	while (*cont) {
		if (*cont == sep) {
			if (!cur.empty()) {
				res.push_back(cur);
				cur.clear();
			}
		} else {
			cur += *cont;
		}
		cont ++;
	}
	if (!cur.empty())
		res.push_back(cur);
	return res;
}

string getHexDump(const void* data, int sz) {
	const unsigned char* p = (const unsigned char*) data;
	string res;
	char buf[1024];
	sprintf(buf, "dumping bytes %02x\n", sz);
	res += buf;
	const unsigned char* pn = p;
	for (int i = 0; i < (sz+15) / 16; i ++) {
		const unsigned char* pline = pn + 16*i;
		int c = min(sz-16*i, 16);
		sprintf(buf, "%04x    ", 16*i);
		res += buf;
		for (int j = 0; j < 16; j ++) {
			if (j == 8)
				res += " ";
			if (j < c) {
				sprintf(buf, "%02x ", pline[j]);
				res += buf;
			}
			else
				res += "   ";
		}
		res += " ";
		for (int j = 0; j < 16; j ++) {
			if (j == 8)
				res += " ";
			if (j < c) {
				sprintf(buf, "%c", isprint(pline[j]) ? pline[j]: '.');
				res += buf;
			}
			else
				res += " ";
		}
		res += '\n';

	}
	return res;
}

void randomBuf(void* data, int sz) {
	char* d = (char*)data;
	srand( (unsigned)time( NULL ) ); 
	for (int i = 0; i < sizeof(d); i ++ ) { 
		d[i] = (char)(rand()%256); 
	}
}

string formatString(const char* fmt, ...)
{
	va_list va;
	va_start(va,fmt);
	char buf[20*1024] = {0};
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va); 
	return buf;
}

int strcmpNoCase(const char* s1, const char* s2) {
	while (*s1 && *s2 && tolower(*s1)-tolower(*s2) == 0) {
		++s1;
		++s2;
	}
	return tolower(*s1)-tolower(*s2);
}

int strcmpCase(const char* s1, const char* s2) {
	while (*s1 && *s2 && *s1-*s2 == 0) {
		++s1;
		++s2;
	}
	return *s1 - *s2;
}

string intToStr(int i) {
	return formatString("%d", i);
}

string longToStr(long l) {
	return formatString("%ld", l);
}

string doubleToStr(double d) {
	return formatString("%lf", d);
}

int strToInt(const string& str) {
	return atoi(str.c_str());
}

long strToLong(const string& str) {
	return atol(str.c_str());
}

double strToDouble(const string& str) {
	return atof(str.c_str());
}

bool strToBool(const string& str) {
	if (compareNoCase(str.c_str(), "true") == 0)
		return true;
	return false;
}

string toLower(const string& str) {
	string r;
	for(size_t i = 0; i<str.size(); i++) {
		if(str[i] >= 'A' && str[i] <= 'Z')
			r += str[i] | 32;
		else
			r += str[i];
	}
	return r;
}

string toUpper(const string& str) {
	string r;
	for(size_t i = 0; i<str.size(); i++) {
		if(str[i] >= 'a' && str[i] <= 'z')
			r += (char)(str[i] - 32);
		else
			r += str[i];
	}
	return r;
}

string& replaceAll(string& str, const string& oldVal, const string& newVal) {
	for(string::size_type pos(0); pos < str.length() && pos!=string::npos; pos+=newVal.length()) {
		if((pos=str.find(oldVal,pos)) != string::npos)
			str.replace(pos, oldVal.length(), newVal);
		else
			break;
	}
	return str;
}

string intListToStr(vector<int> l, char delim) {
	string str;
	vector<int>::iterator iter;
	for(iter = l.begin(); iter != l.end(); iter++)
		str += intToStr(*iter) + delim;
	str = str.substr(0, str.size()-1);
	return str;
}

string strListToStr(vector<string> l, char delim) {
	string str;
	vector<string>::iterator iter;
	for(iter = l.begin(); iter != l.end(); iter++)
		str += *iter + delim;
	str = str.substr(0, str.size()-1);
	return str;
}

string strListToStr(vector<string> l, const string& delim) {
	string str;
	vector<string>::iterator iter;
	for(iter = l.begin(); iter != l.end(); iter++)
		str += *iter + delim;
	str = str.substr(0, str.size()-delim.size());
	return str;
}

vector<int> strToIntList(const string& str, char delim) {
	string s = trimSpaces(str);
	vector<string> vs = splitString(s.c_str(), delim);
	vector<int> intList;
	for (size_t i = 0; i < vs.size(); i ++) {
		intList.push_back(strToInt(vs[i]));
	}
	return intList;
}

vector<long> strToLongList(const string& str, char delim) {
	string s = trimSpaces(str);
	vector<string> vs = splitString(s.c_str(), delim);
	vector<long> intList;
	for (size_t i = 0; i < vs.size(); i ++) {
		intList.push_back(strToLong(vs[i]));
	}
	return intList;
}

vector<string> strToStrList(const string& str, char delim) {
	return splitString(str.c_str(), delim);
}

vector<string> strToStrList(const string& src, const string& delimit)
{
	vector<string> strList;   
	if( src.empty() || delimit.empty() ) 
		return strList;

	string::size_type deli_len = delimit.size();   
	long index = string::npos, last_search_position = 0;   
	while( (index=src.find(delimit,last_search_position))!= string::npos )   
	{   
		strList.push_back( src.substr(last_search_position, index-last_search_position) );   
		last_search_position = index + deli_len;   
	}   
	string last_one = src.substr(last_search_position);  
	if(!last_one.empty())	    
		strList.push_back(last_one);   
	return strList;   
}



string trimSpaces(const string& value) {
	int be = 0;
	while (value[be] && isspace(value[be])) 
		be ++;
	int ed = value.length()-1;
	while (ed > 0 && isspace(value[ed]))
		ed --;
	if (ed >= be)
		return value.substr(be, ed-be + 1);
	return "";

}


}