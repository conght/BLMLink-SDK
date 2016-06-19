#pragma once
#include <blm/blmbase.h>
struct pj_timer_heap_t;
struct pj_ioqueue_t;
namespace BlmComm {
	class BlmIoqueue;
	class ClientTcp;
	class PjTcp;
	//用户可以使用这个类与服务器通信，发送请求，处理数据
	class BlmClient {
	private:
		friend class ClientTcp;
		ClientTcp* clientTcp;
		vector<ClientTcp*> tcpCandidates;
		BlmIoqueue* poller;
		list<SimpleCallback*> connCallbacks, closeCallbacks;
		typedef map<int, list<MsgCallback*> > MsgMap;
		MsgMap msgMap;
	
		//以下函数供clientTcp调用
		int processMsg(int mt, int st, const string& data, int msgid);
		int onConnect(PjTcp* tcp);
		int onClose(PjTcp* tcp);
	public:
		BlmClient(BlmIoqueue* poller1);
		~BlmClient();
		//选择最快的服务器连接，ips参数为 "ip1 ip2 ip3"
		void connectFastest(const char* ips, short port);
		//与服务器建立连接
		void connect(const char* ip, short port) { connectFastest(ip, port); }
		//断开连接
		void disconnect();
		//发送请求返回一个消息id，在msgcallback中，会传入同一个msgid
		int sendMsg(int mt, int st, const char* data, int sz);

		//以下与回调相关，只支持成员函数的回调（这样可以保持接口的简洁性，又能满足绝大部分的需求）
		//注册回调时，请注意下面的函数是以模板的方式给出，请确保成员函数与add*Callback的声明相匹配，否则编译错误
		//成员函数的返回值为int，返回0表示成功，返回其他表示出错，此时连接会被关闭
		//成员函数的最后一个参数为void*，进行回调时，会把注册回调时传递的arg参数传递给成员函数，与启动线程时传递的参数方式相同
		//返回值类型为void*，指向内部使用的回调对象，详见removeCallback

		//注册回调，建立连接后，进行回调
		template<class C> void* addConnectCallback(C* obj, int (C::*memFn)(void*), void* arg=0);
		//注册回调，断开连接时，进行回调（如果连接未能成功建立，此函数不会被调用）
		template<class C> void* addCloseCallback(C* obj, int (C::*memFn)(void*), void* arg=0);
		//注册回调，收到指定的mt消息包时，进行回调
		//memFn的参数依次为：int mt, int st, const string& msg, int msgid, void* arg 其中msgid为sendMsg返回的消息id
		template<class C> void* addMsgCallback(int mt, C* obj, int (C::*memFn)(int, int, const string&, int, void*), void* arg=0);

		//上面注册的所有回调都可用下面的函数注销
		//注销回调，有两种方式，通常在对象的析构中调用removeAllCallback，更细粒度的控制则调用removeCallback

		//注销回调，传入的参数为注册回调时返回的指针
		//成功返回0，失败返回-1
		int removeCallback(void* cb);
		//注销与此对象相关的所有回调，传入的参数为注册回调时所使用的obj参数
		//返回移除的callback个数
		int removeObjCallbacks(void* obj);

	};


	//IO队列，用户可以在此注册定时器函数
	class BlmIoqueue: private MemPool {
		friend class PjTcp;
		friend class PjHttp;
		pj_timer_heap_t *timer_heap;
		pj_ioqueue_t *ioque;
		vector<TimerObj*> timerCallbacks;
		void* addTimerCallback_imp(int interval, SimpleCallback* scb);
		void wget_imp(const char* url, DataCallback* dcb, int millsec);
	public:
		BlmIoqueue();
		~BlmIoqueue();
		//轮询网络数据，调用数据处理的回调，轮询定时器，调用超时的定时器，如果没有数据，则最多等待milliseconds毫秒
		void poll(int milliseconds=0);
		//注册定时器的回调，参数详见BlmClient中，注册注销函数的说明
		//使用堆对超时的定时器进行遍历，算法复杂度为O(lg(n))
		template<class T> void* addTimerCallback(int interval, T* obj, int (T::*mfn)(void*), void* arg=0) ;
		//注销回调
		//成功返回0，失败返回-1
		int removeTimer(void* timer);
		//注销与此对象相关的所有回调，传入的参数为注册回调时所使用的obj参数
		//返回移除的timer个数
		int removeObjTimers(void* obj);
		template<class T> void wget(const char* url, T* obj, int(T::*mfn)(const string&, void*), int millisec_timeout=5000, void* arg=0);
	};

	struct SingleComm: public BlmIoqueue, public BlmClient, public Singleton<SingleComm> { 
		SingleComm(): BlmClient((BlmIoqueue*)this) {}
		int removeObj(void* obj) { int r = removeObjTimers(obj); r += removeObjCallbacks(obj); checktrue = r > 0; return r; }
	};


	
	//以下是模板实现，必须放在头文件中，而内容与与接口无关
	template<class T> void* BlmIoqueue::addTimerCallback(int interval, T* obj, int (T::*mfn)(void*), void* arg) {
		return addTimerCallback_imp(interval, makeSimpleCallback(obj, mfn, arg));
	}

	template<class T> void BlmIoqueue::wget(const char* url, T* obj, int(T::*mfn)(const string&, void*), int millisec, void* arg) {
		DataCallback* cb = makeDataCallback(obj, mfn, arg);
		wget_imp(url, cb, millisec);
	}


	template<class C> void* BlmClient::addConnectCallback(C* obj, int (C::*memFn)(void*), void* arg) {
		SimpleCallback* cb = makeSimpleCallback(obj, memFn, arg);
		connCallbacks.push_front(cb);
		return cb;
	}
	template<class C> void* BlmClient::addCloseCallback(C* obj, int (C::*memFn)(void*), void* arg) {
		SimpleCallback* cb = makeSimpleCallback(obj, memFn, arg);
		closeCallbacks.push_front(cb);
		return cb;
	}
	template<class C> void* BlmClient::addMsgCallback(int mt, C* obj, int (C::*memFn)(int, int, const string&, int, void*), void* arg) {
		MsgCallback* cb = makeMsgCallback(obj, memFn, arg);
		msgMap[mt].push_front(cb);
		return cb;
	}

}


