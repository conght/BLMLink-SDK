#pragma once
#include <string>
#include <blm/util_fun.h>
using namespace std;
struct cJSON;

namespace BlmComm {
	struct Zlib {
		static string uncompress(const char* data, int len);
		static string compress(const char* data, int len);
	};

	struct Base64 {
		static string encode(const char* data, int len);
		static string decode(const char* data, int len);
	};

	struct Hash {
		static string md5(const char* data, int len);
		static string sha1(const char* data, int len);
	};

	//AES 128 cbc
	class AES128Cipher {
		char key[16];
		string doAES128(const char* in, int len, bool encrypt);
	public:
		void setKey(const char* key1) { memcpy(key, key1, sizeof(key)); }
		string getKey() { return string(key, key+sizeof(key)); }
		//生成随机key
		AES128Cipher() { randomBuf(key, sizeof(key)); }
		AES128Cipher(const char* key1) { setKey(key1); }
		string encrypt(const char* cont, int len) { return doAES128(cont, len, 1); }
		string decrypt(const char* cont, int len) { return doAES128(cont, len, 0); }
	};


	class RC4 {
		char key[32];
	public:
		//生成随机key
		RC4() { randomBuf(key, sizeof(key)); }
		RC4(const char* key1) { setKey(key1); }
		void setKey(const char* key1) { memcpy(key, key1, sizeof(key)); }
		string rc4(const char* data, int len);
		void rc4buf(void* data, int len);
	};

	class Json {
		bool isowned;
		cJSON* json;
		string err;
		Json(cJSON* j): json(j), isowned(false) {}
		Json& operator=(const Json&);
	public:
		Json(): json(NULL), isowned(false) {}
		~Json() ;
		//当前值获取
		const char* getName() ;
		int getInt() ;
		const char* getString();
		double getDouble() ;

		//当前值的判断
		bool isValid() { return json != NULL; }
		bool isFalse() ;
		bool isTrue() ;
		bool isNull() ;
		bool isNumber();
		bool isString();
		bool isArray();
		bool isObject() ;

		//成员的获取
		bool isMember(const char* name);
		Json get(const char* name);
		int getInt(const char* name) { return get(name).getInt(); }
		double getDouble(const char* name) { return get(name).getDouble(); }
		const char* getString(const char* name) { return get(name).getString(); }

		//数组操作
		int getSize() ;
		Json get(int index) ;
		int getInt(int i) { return get(i).getInt(); }
		double getDouble(int i) { return get(i).getDouble(); }
		const char* getString(int i) { return get(i).getString(); }

		const char* getErr() { return err.c_str(); }
		bool parse(const char* str) ;

	};
}

