#include <blm/blmutil.h>
#include <memory>
#include <zlib/zlib.h>
#include <blm/util_fun.h>
#include <stdlib.h>
#include <polarssl/arc4.h>
#include <polarssl/aes.h>
#include <polarssl/md5.h>
#include <polarssl/sha1.h>
#include <polarssl/base64.h>
#include <blm/blmbase.h>
#include "imp/cJSON.h"


namespace BlmComm {

string Zlib::uncompress(const char* data, int len) {
	long reslen = len * 3;
	char* pres = new char[reslen];
	int ret = ::uncompress((Bytef*)pres, (uLongf*)&reslen, (Bytef*)data, len);
	//如果内存不够，则分配更大的内存进行解压
	while (ret == Z_BUF_ERROR) {
		delete[] pres;
		reslen *= 2;
		pres = new char[reslen];
		ret = ::uncompress((Bytef*)pres, (uLongf*)&reslen, (Bytef*)data, len);
	}
	if (ret != Z_OK) {
		blmlog(0, "blmutil", "Zlib::uncompress failed: %d %s",ret, ret==Z_DATA_ERROR ? "bad data format, can't be uncompressed" : "other error");
	}
	AutoArrayPtr<char> release(pres);
	return string(pres, reslen);
}

string Zlib::compress(const char* data, int len) {
	long reslen = len;
	char* pres = new char[reslen];
	int ret = ::compress((Bytef*)pres, (uLongf*)&reslen, (Bytef*)data, len);
	//如果内存不够，则分配更大的内存进行压缩
	while (ret == Z_BUF_ERROR) {
		delete[] pres;
		reslen *= 2;
		pres = new char[reslen];
		ret = ::compress((Bytef*)pres, (uLongf*)&reslen, (Bytef*)data, len);
	}
	if (ret != Z_OK) {
		blmlog(0, "blmutil", "Zlib::uncompress failed: %d %s",ret, ret==Z_DATA_ERROR ? "bad data format, can't be uncompressed" : "other error");
	}
	AutoArrayPtr<char> release(pres);
	return string(pres, reslen);

}

string Base64::encode(const char* data, int len) {
	size_t olen = len * 2 + 2;
	char* o = new char[olen];
	auto_ptr<char> release(o);
	checktrue = 0 == base64_encode((unsigned char*)o, &olen, (const unsigned char*)data, len);
	return string(o, o + olen);
}

string Base64::decode(const char* data, int len) {
	size_t olen = len + 2;
	char* o = new char[olen];
	auto_ptr<char> release(o);
	checktrue = 0 == base64_decode((unsigned char*)o, &olen, (const unsigned char*)data, len);
	return string(o, o + olen);
}

string Hash::md5(const char* data, int len) {
	char buf[16];
	::md5((const unsigned char*) data, len, (unsigned char*)buf);
	return string(buf, buf+16);
}

string Hash::sha1(const char* data, int len) {
	char buf[20];
	::sha1((const unsigned char*)data, len, (unsigned char*)buf);
	return string(buf, buf+20);
}

string RC4::rc4(const char* data, int len) {
	char* p = new char[len];
	AutoArrayPtr<char> release1(p);
	arc4_context rc4ctx;
	arc4_setup(&rc4ctx, (unsigned char*)key, sizeof(key));
	arc4_crypt(&rc4ctx, len, (unsigned char*)data, (unsigned char*)p);
	return string(p, len);
}

void RC4::rc4buf(void* data, int len) {
	string obuf = rc4((char*)data, len);
	memcpy(data, obuf.c_str(), len);
}

string AES128Cipher::doAES128(const char* in, int inlen, bool encrypt) {
	if ( inlen <= 0 || in == NULL ) {  
		return "";  
	}

	unsigned char iv1[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	aes_context aesctx;
	if (encrypt) {
		int paded = 16 - (inlen % 16);
		int blen = inlen + paded;
		char padv = char(paded);
		unsigned char* pin = new unsigned char[blen];
		unsigned char* po = new unsigned char[blen];
		AutoArrayPtr<unsigned char> release1(pin), release2(po);
		memcpy(pin, in, inlen);
		memset(pin + inlen, padv, paded);
		aes_setkey_enc(&aesctx, (unsigned char*)key, 128);
		aes_crypt_cbc(&aesctx, 1, blen, iv1, pin, po);
		return string(po, po + blen);
	}
	aes_setkey_dec(&aesctx, (unsigned char*)key, 128);
	unsigned char* po = new unsigned char[inlen];
	AutoArrayPtr<unsigned char> release1(po);
	aes_crypt_cbc(&aesctx, 0, inlen, iv1, (const unsigned char*)in, po);
	int paded = po[inlen -1];
	return string(po, po + inlen - paded);  
}

Json::~Json() { if (json && isowned) cJSON_Delete(json); }
//当前值获取
const char* Json::getName() { return json->string; }
int Json::getInt() { return json->valueint; }
const char* Json::getString() { return json->valuestring; }
double Json::getDouble() { return json->valuedouble; }

//当前值的判断
bool Json::isFalse() { return json->type == cJSON_False; }
bool Json::isTrue() { return json->type ==cJSON_True; }
bool Json::isNull() { return json->type ==cJSON_NULL; }
bool Json::isNumber() { return json->type == cJSON_Number; }
bool Json::isString() { return json->type == cJSON_String; }
bool Json::isArray() { return json->type == cJSON_Array; }
bool Json::isObject() { return json->type ==cJSON_Object; }

//成员的获取
bool Json::isMember(const char* name) { return cJSON_GetObjectItem(json, name) != NULL; }
Json Json::get(const char* name) { return cJSON_GetObjectItem(json, name); }

//数组操作
int Json::getSize() { return cJSON_GetArraySize(json); }
Json Json::get(int index) { return cJSON_GetArrayItem(json, index); }

bool Json::parse(const char* str) {
	if (isowned && json) {
		cJSON_Delete(json);
	}
	json = cJSON_Parse(str);
	isowned = true;
	if (!json) {
		err = cJSON_GetErrorPtr();
		blmlog(0, "blmcomm", "json parsed failed: string: %s msg: %s", str, err.c_str());
	}
	return json != NULL;
}

}
