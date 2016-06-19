#include <blm/blmcomm.h>
#include <pjlib.h>
#include <pjlib-util/http_client.h>
namespace BlmComm {

//此函数用于二进制数据到int之间的转换
static int toLittleEndian(int v) {
	if (1 == pj_htonl(1)) { //如果是BigEndian，则需要转换字节序
		char* p = (char*)v;
		reverse(p, p + 4);
	}
	return v;
}

//如果obj参数为NULL，则直接全部删除
template<class C> static int removeAllCallbackFromList(C& cbs, void* obj) {
	typedef typename C::iterator Iter;
	Iter result = cbs.begin();
	int removed = 0;
	for (Iter first = cbs.begin(); first!=cbs.end(); ++first) {
		if (obj && (*first)->obj() != obj) {
			*result = *first;
			++result;
		} else {
			delete *first;
			removed ++;
		}
	}
	cbs.erase(result, cbs.end());
	return removed;
}

class TimerObj{
public:
	long milliInter;
	Time timeout;
	SimpleCallback* callback;
	void* obj() { return callback->obj(); }
	void resetTimer() { timeout = Time::getTickcount(); Time::addTick(&timeout, milliInter); }
	void handleTimeout() { callback->call(); resetTimer(); }
	TimerObj(int interval, SimpleCallback* cb) : milliInter(interval), callback(cb) { resetTimer(); }
	~TimerObj() { delete callback; }
	//以下函数用于比较大小，被stl中的堆函数使用
	static bool pointerGreater(TimerObj* to1, TimerObj* to2) { return *to2 < *to1; }
	bool operator < (const TimerObj& to2) { return operator<(to2.timeout); }
	bool operator < (const Time& tv) { return (timeout.sec - tv.sec)*1000 + timeout.msec - tv.msec < 0; }
};


class BlmIoqueue;
class PjHttp: private MemPool {
	static pj_http_req_callback httpcb;
	string url, result;
	DataCallback* onComplete;
	pj_http_req *http_req;
	pj_http_req_param req_param;
	int millisec;
	static void on_response(pj_http_req *http_req, const pj_http_resp *resp);
	static void on_send_data(pj_http_req *http_req, void **data, pj_size_t *size);
	static void on_data_read(pj_http_req *hreq, void *data, pj_size_t size);
	static void on_complete(pj_http_req *hreq, pj_status_t status, const pj_http_resp *resp);
public:
	PjHttp(const char* url, DataCallback* cb, BlmIoqueue* poller, int millisec1);
	~PjHttp();
};


pj_http_req_callback PjHttp::httpcb = {
	&PjHttp::on_response,
	&PjHttp::on_send_data,
	&PjHttp::on_data_read,
	&PjHttp::on_complete
};

PjHttp::PjHttp(const char* url, DataCallback* cb, BlmIoqueue* poller, int millisec1) {
	blmlog(3, "blmcomm", "getting url: %s", url);
	this->url = url;
	millisec = millisec1;
	onComplete = cb;
	pj_str_t pjurl;
	pj_strdup2(pool, &pjurl, url);
	pj_http_req_param_default(&req_param);
	req_param.user_data = this;
	checkpj = pj_http_req_create(pool, &pjurl, poller->timer_heap, poller->ioque, &req_param, &httpcb, &http_req);
	pj_time_val timout = { millisec / 1000, millisec % 1000 };
	pj_http_req_set_timeout(http_req, &timout);
	checkpj = pj_http_req_start(http_req);
}

PjHttp::~PjHttp() {
	blmlog(3, "blmcomm", "destroying pjhttp: %s", url.c_str());
	delete onComplete;
	pj_http_req_destroy(http_req);
}

void PjHttp::on_response(pj_http_req *http_req, const pj_http_resp *resp) {
	PJ_UNUSED_ARG(http_req);
	PJ_UNUSED_ARG(resp);
}

void PjHttp::on_send_data(pj_http_req *http_req, void **data, pj_size_t *size) {
	PJ_UNUSED_ARG(http_req);
	PJ_UNUSED_ARG(size);
	PJ_UNUSED_ARG(data);
}

void PjHttp::on_data_read(pj_http_req *hreq, void *data, pj_size_t size) {
	PjHttp* http = (PjHttp*)pj_http_req_get_user_data(hreq);
	char* cdata = (char*)data;
	http->result.append(cdata, cdata+size);
}
void PjHttp::on_complete(pj_http_req *hreq, pj_status_t status, const pj_http_resp *resp) {
	checkpj = status;
	PjHttp* http = (PjHttp*)pj_http_req_get_user_data(hreq);
	http->onComplete->call(http->result);
	if (status == PJ_SUCCESS) {
		checktrue = resp->size == http->result.size();
	}
	blmlog(3, "blmcomm", "oncomplete: size: %d, ", resp?resp->size : 0);
	delete http;
}


class PjTcp: private MemPool {
protected:
	static pj_ioqueue_callback callback;
	pj_sockaddr_in addr;
	pj_sock_t sock;
	string ip;
	int port;
	pj_ioqueue_key_t* key;
	pj_ioqueue_op_key_t read_op, write_op;
	char read_buf[1025];
	list<string> write_bufs;

	static void on_ioqueue_read(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key,	pj_ssize_t bytes_read) ;
	static void on_ioqueue_write(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key,pj_ssize_t bytes_written) ;
	static void on_ioqueue_accept(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, pj_sock_t sock, int status);
	static void on_ioqueue_connect(pj_ioqueue_key_t *key, int status);
	void startRead() ;
public:
	PjTcp(const char* ip, short port, BlmIoqueue* poller);
	virtual ~PjTcp() ;
	void sendData(const char* data, int size);
	virtual int onConnect(PjTcp* tcp) { return 0; }
	virtual int onError() { return 0; }
	virtual int onDataRead(char* data, int size) { return 0; }
	virtual int onDataWritten(const char* data, int size) { return 0; }
	virtual void onClose(PjTcp* tcp) {  }
};

pj_ioqueue_callback PjTcp::callback = 
{
	&PjTcp::on_ioqueue_read,
	&PjTcp::on_ioqueue_write,
	&PjTcp::on_ioqueue_accept,
	&PjTcp::on_ioqueue_connect
};

PjTcp::PjTcp(const char* ip, short port, BlmIoqueue* poller){
	blmlog(3, "blmcomm", "constructing tcp: %s: %d", ip, port);
	this->ip = ip;
	this->port = port;
	checkpj = pj_sockaddr_in_init(&addr, 0, 0);
	pj_str_t s;
	addr.sin_addr = pj_inet_addr(pj_cstr(&s, ip));
	addr.sin_port = pj_htons(port);
	checkpj = pj_sock_socket(pj_AF_INET(), pj_SOCK_STREAM(), 0, &sock);
	checkpj = pj_ioqueue_register_sock(pool, poller->ioque, sock, NULL, &PjTcp::callback, 
		&key);
	pj_ioqueue_set_user_data(key, this, NULL);
	checkpj = pj_ioqueue_connect(key, &addr, sizeof(addr));
}

void PjTcp::on_ioqueue_read(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key,	pj_ssize_t bytes_read) {
	PjTcp* p = (PjTcp*)pj_ioqueue_get_user_data(key); 
	p->read_buf[bytes_read] = '\0';
	if (bytes_read == 0 || p->onDataRead((char*)p->read_buf, bytes_read)) {
		p->onClose(p);
	} else {
		p->startRead();
	}
}

void PjTcp::on_ioqueue_write(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key,pj_ssize_t bytes_written) {
	PjTcp* p = (PjTcp*)pj_ioqueue_get_user_data(key);
	string& first_data = p->write_bufs.front();
	if (bytes_written != first_data.size()) {
		blmlog(0, "blm_comm", "in tcp written, should have written %d only written: %d", first_data.size(), bytes_written);
	}
	int eno = p->onDataWritten(first_data.c_str(), first_data.size());
	p->write_bufs.pop_front();
	if (eno != 0) {
		p->onClose(p);
	}
}

void PjTcp::on_ioqueue_accept(pj_ioqueue_key_t *key, pj_ioqueue_op_key_t *op_key, pj_sock_t sock, int status) {
	blmlog(0, "blm_comm", "accept finished");
}

void PjTcp::on_ioqueue_connect(pj_ioqueue_key_t *key, int status) {
	PjTcp* p = (PjTcp*)pj_ioqueue_get_user_data(key); 
	if (status != PJ_SUCCESS || p->onConnect(p)) {
		p->onClose(p);
	} else 
		p->startRead();
}

void PjTcp::startRead() {
	pj_ssize_t bytes = 1024;
	checkpj = pj_ioqueue_recv(key, &read_op, read_buf, &bytes, PJ_IOQUEUE_ALWAYS_ASYNC);
}

PjTcp::~PjTcp() { 
	checkpj = pj_ioqueue_unregister(key);
	pj_sock_close(sock);
	blmlog(3, "blmcomm", "connection of %s:%d closed", ip.c_str(), port);
}

void PjTcp::sendData(const char* data, int size) {
	write_bufs.push_back(string(data, size));
	pj_ssize_t bytes = size;
	checkpj = pj_ioqueue_send(key, &write_op, write_bufs.back().c_str(), &bytes, PJ_IOQUEUE_ALWAYS_ASYNC);
}


struct ClientMsg {
	char st;
	char mt;
	char compress;
	char encrypt;
	int id;
	int len;
	static int msgid;
	bool isValid() const { return (compress == 0 || compress == 1) && (encrypt == 0 || encrypt==1); }
	ClientMsg() { st = mt = compress = encrypt = 0; len = 0; msgid = 0; }
	ClientMsg(int mt1, int st1, int compress1, int encrypt1, int length1) :st(st1), mt(mt1), compress(compress1), encrypt(encrypt1), len(length1), id(++msgid) {}
};

int ClientMsg::msgid = 0;

class BlmClient;

class ClientTcp: public PjTcp {
	RC4 rc4;
	AES128Cipher aes;
	string recved;
	BlmClient* blmclient;
	string decodeMsg(ClientMsg& msg, const char* data) ;
public:
	ClientTcp(const char* ip, short port, BlmClient* client);
	virtual int onConnect(PjTcp* tcp);
	virtual int onDataRead(char* data, int size) ;
	int sendMsg(int mt, int st, int compressType, int encryptType, const char* data, int sz);
	virtual void onClose(PjTcp* tcp) { blmclient->onClose(tcp); }
};


BlmIoqueue::BlmIoqueue() {
	checkpj = pj_timer_heap_create(pool, 32, &timer_heap);
	checkpj = pj_ioqueue_create(pool, 64, &ioque);
}

BlmIoqueue::~BlmIoqueue() {
	if (!timerCallbacks.empty()) {
		blmlog(0, "blmcomm", "timers are not unregistered. total: %d", timerCallbacks.size());
		removeAllCallbackFromList(timerCallbacks, NULL);
	}
	pj_timer_heap_destroy(timer_heap);
	pj_ioqueue_destroy(ioque);
}

void BlmIoqueue::poll(int milliseconds) {
	Time tv = Time::getTickcount();
	while (!timerCallbacks.empty() && *timerCallbacks.front() < tv) {
		TimerObj* to = timerCallbacks.front();
		pop_heap(timerCallbacks.begin(), timerCallbacks.end(), TimerObj::pointerGreater);
		to->handleTimeout();
		timerCallbacks.back() = to;
		push_heap(timerCallbacks.begin(), timerCallbacks.end(), TimerObj::pointerGreater);
	}
	pj_time_val timeout = {milliseconds / 1000, milliseconds % 1000 };
	int pret = pj_ioqueue_poll(ioque, &timeout);
	checktrue = (pret != -1 || pret >= 0);
	pj_timer_heap_poll(timer_heap, NULL);

}

int BlmIoqueue::removeTimer(void* timer) {
	vector<TimerObj*>::iterator p = find(timerCallbacks.begin(), timerCallbacks.end(), timer);
	if (p == timerCallbacks.end()) {
		blmlog(0, "blmcomm", "removing a nonexists timer. total size of timers is: %d", timerCallbacks.size());
		return -1;
	}
	delete *p;
	swap(*p, *(timerCallbacks.end() - 1));
	timerCallbacks.pop_back();
	make_heap(timerCallbacks.begin(), timerCallbacks.end(), TimerObj::pointerGreater);
	return 0;
}

int BlmIoqueue::removeObjTimers(void* obj) {
	return removeAllCallbackFromList(timerCallbacks, obj);
}

void* BlmIoqueue::addTimerCallback_imp(int interval, SimpleCallback* scb) {
	TimerObj* to =new TimerObj(interval, scb);
	timerCallbacks.push_back(to);
	push_heap(timerCallbacks.begin(), timerCallbacks.end(), TimerObj::pointerGreater);
	return to;
}

void BlmIoqueue::wget_imp(const char* url, DataCallback* dcb, int millisec) {
	new PjHttp(url, dcb, this, millisec);
}
unsigned char rc4_key[32] = {0x22, 0xBE, 0xE5, 0xCF, 0xBB, 0x07, 0x64, 0xD9, 0x00, 0x45, 0x1B, 0xD0, 0x24, 0xB8, 0xD5, 0x45};

ClientTcp::ClientTcp(const char* ip, short port, BlmClient* client): PjTcp(ip, port, client->poller), rc4((char*)rc4_key) {
	blmclient = client;
}

int ClientTcp::sendMsg(int mt, int st, int compressType, int encryptType, const char* data, int sz) {
	if (compressType != 0 && compressType != 1 || encryptType != 0 && encryptType != 1) {
		blmlog(0, "blmcomm", "compress type or encrypt type not supported: compress: %d, encrypt: %d", compressType, encryptType);
		return 0;
	}
	string comd, encd;
	if (compressType == 0) {
		comd.assign(data, sz);
	} else {
		comd = Zlib::compress(data, sz);
	}
	if (encryptType == 0 || mt == 255 && st == 255) {
		encd.assign(comd.c_str(), comd.size());
	} else {
		encd = aes.encrypt(comd.c_str(), comd.size());
	}
	ClientMsg msg(mt, st, compressType, encryptType, encd.size());
	int id = msg.id;
	msg.len = toLittleEndian(msg.len);
	rc4.rc4buf(&msg, sizeof(msg));
	sendData((char*)&msg, sizeof(msg));
	sendData(encd.c_str(), encd.size());
	blmlogDump(5, "blmcomm", "head",&msg,  sizeof(msg));
	blmlogDump(5, "blmcomm", "cont", encd.c_str(), encd.length());
	return id;
}

int ClientTcp::onConnect(PjTcp* tcp) {
	blmlog(3, "blmcomm", "connected: %s: %d", ip.c_str(), port);
	char buf[16];
	memcpy(buf, aes.getKey().c_str(), sizeof(buf));
	rc4.rc4buf(buf, sizeof(buf));
	//发送密钥
	sendMsg(0xff, 0xff, 0, 1, buf, sizeof(buf));
	return blmclient->onConnect(tcp);
}

int  ClientTcp::onDataRead(char* data, int size) {
	recved.append(data, data+size);
	for (;;) {
		if (recved.size() < sizeof(ClientMsg)) {
			break;
		}
		ClientMsg msg = *(const ClientMsg*)recved.c_str();
		rc4.rc4buf(&msg, sizeof(ClientMsg));
		msg.len = toLittleEndian(msg.len);
		if (msg.len < 0 || msg.len > 16*1024*1024) {
			blmlog(0, "blmcomm", "bad msg format recved");
			return -1;
		}
		if (recved.size() < sizeof(ClientMsg) + msg.len) {
			break;
		}
		string cont = decodeMsg(msg, recved.c_str() + sizeof(ClientMsg));
		recved = recved.substr(sizeof(ClientMsg) + msg.len);
		blmlog(3, "blmcomm", "msg recved: mt: %d st: %d cont: %s", msg.mt, msg.st, cont.c_str());
		int eno = blmclient->processMsg(msg.mt, msg.st, cont, msg.id);
		if (eno != 0)
			return eno;
	}
	return 0;
}
string  ClientTcp::decodeMsg(ClientMsg& msg, const char* data) {
	string comd, encd;
	if (msg.compress == 0) {
		comd.assign(data, msg.len);
	} else {
		comd = Zlib::uncompress(data, msg.len);
	}
	if (msg.encrypt == 0) {
		encd.assign(comd.c_str(), comd.size());
	} else {
		encd = aes.decrypt(comd.c_str(), comd.size());
	}
	return encd;
}

BlmClient::BlmClient(BlmIoqueue* poller1) {
	clientTcp = NULL;
	poller = poller1;
}

BlmClient::~BlmClient() {
	if (!connCallbacks.empty() || !closeCallbacks.empty() || !msgMap.empty()) {
		blmlog(0, "blmcomm", "callbacks are not removed properly size of connect: %d close: %d msg: %d", connCallbacks.size(), closeCallbacks.size(), msgMap.size());
		removeAllCallbackFromList(connCallbacks, NULL);
		removeAllCallbackFromList(closeCallbacks, NULL);
		for (MsgMap::iterator it = msgMap.begin(); it != msgMap.end(); ++it) {
			removeAllCallbackFromList(it->second, NULL);
		}
	}
	disconnect();
}

template<class C> static int callList(list<C*>& ls) {
	typename list<C*>::iterator it = ls.begin();
	for (; it != ls.end(); ++it) {
		C* p = *it;
		int eno = p->call();
		if (eno != 0) 
			return eno;
	}
	return 0;
}

int BlmClient::onConnect(PjTcp* tcp) {
	checktrue = tcpCandidates.size() && clientTcp == NULL;
	for (unsigned int i = 0; i < tcpCandidates.size(); i ++) {
		if (tcpCandidates[i] != tcp)
			delete tcpCandidates[i];
	}
	tcpCandidates.clear();
	clientTcp = (ClientTcp*)tcp;
	return callList(connCallbacks);
}

int BlmClient::onClose(PjTcp* tcp) {
	if (tcpCandidates.size() > 0) {
		for (size_t i = 0; i < tcpCandidates.size(); i ++) {
			if (tcpCandidates[i] == tcp) {
				delete tcpCandidates[i];
				swap(tcpCandidates[i], tcpCandidates.back());
				tcpCandidates.pop_back();
				break;
			}
		}
	}
	if (!tcpCandidates.empty())
		return 0;
	disconnect();
	return callList(closeCallbacks);
}

void BlmClient::connectFastest(const char* ips, short port) {
	if (clientTcp != NULL || tcpCandidates.size() > 0) {
		blmlog(0, "blmcomm", "connect called when already connected");
		disconnect();
	}
	vector<string> sips = splitString(ips, ' ');
	for (size_t i = 0; i < sips.size(); i ++) {
		if (!sips.empty())
			tcpCandidates.push_back(new ClientTcp(sips[i].c_str(), port, this));
	}
}

void BlmClient::disconnect() {
	if (clientTcp) {
		delete clientTcp;
		clientTcp = NULL;
	}
	for (size_t i = 0; i < tcpCandidates.size(); i ++) {
		delete tcpCandidates[i];
	}
	tcpCandidates.clear();
}

int BlmClient::sendMsg(int mt, int st, const char* data, int sz) {
	return clientTcp ? clientTcp->sendMsg(mt, st, sz > 1024 ? 1 : 0, 1, data, sz) : -1;
}

int BlmClient::processMsg(int mt, int st, const string& data, int msgid) {
	MsgMap::iterator it = msgMap.find(mt);
	if (it == msgMap.end()) {
		blmlog(0, "blmcomm", "mt: %d's callback not found", mt);
		return 0;
	}
	list<MsgCallback*>& cbs = it->second;
	for (list<MsgCallback*>::iterator p = cbs.begin(); p != cbs.end(); ++p) {
		MsgCallback* cb = *p;
		int eno = cb->call(mt, st, data, msgid);
		if (eno != 0)
			return eno;
	}
	return 0;
}

int BlmClient::removeObjCallbacks(void* obj) {
	int removedCn = 0;
	removedCn += removeAllCallbackFromList(connCallbacks, obj);
	removedCn += removeAllCallbackFromList(closeCallbacks, obj);
	vector<int> emptyMsg;
	for (MsgMap::iterator it = msgMap.begin(); it != msgMap.end(); ++it) {
		list<MsgCallback*>& mcbs = it->second;
		removedCn += removeAllCallbackFromList(mcbs, obj);
		if (mcbs.empty()) {
			emptyMsg.push_back(it->first);
		}
	}
	for (size_t i = 0; i < emptyMsg.size(); i ++) {
		msgMap.erase(emptyMsg[i]);
	}
	return removedCn;
}
}
