#pragma  once
#include <blm/imp/blmbase_imp.h>
using namespace std;
struct pj_caching_pool;
struct pj_pool_t;
struct pj_atomic_t;
struct pj_mutex_t;
struct pj_sem_t;
struct pj_thread_t;
namespace BlmComm {
	extern pj_caching_pool& pj_cp;
	//初始化基本库
	void blminit();
	void blmshutdown();
	//设置日志，可以多次设置，
	//对于detail_level，0为最低级别，错误、警告以0级别输出，3为普通信息输出，5为最详细的输出
	//modules指定输出那些模块的信息，例如：""全部输出， "blmcomm main"输出blmcomm与main模块的日志
	//如果不调用blmSetLog，则默认输出级别为3，输出所有模块日志，输出到标准输出
	void blmSetLog(int detail_level=3, const char* modules="", const char* logfile="");
	//设置日志输出函数，例如android平台可以调用此函数把日志输出到logcat
	void blmSetLogOutput(void (*logfun)(int, const char*, int));
	//打印日志
	void blmlog(int detail_level, const char* module, const char* fmt, ...);
	//打印二进制内容
	void blmlogDump(int detaillevel, const char* module, const char* tag, const void* data, int len);
	//使用方式为：checkpj=pj_fun,用于检查返回值，适合pj函数,主要在namespace BlmComm的实现中使用
#define checkpj CheckerPjStatus(__FILE__, __LINE__)

	//使用方式为：checktrue=fun, 用于检查返回值，适合成功则返回true的函数或表达式
#define checktrue CheckerTrue(__FILE__, __LINE__)

	//内存池，pjlib的内存使用方式是：1.分配pool 2.从pool分配内存 3.释放pool（所有从pool分配的内存随之释放)
	class MemPool {
	protected:
		pj_pool_t* pool;
		bool owned;
		//参数p为NULL，则自行分配一个pool，如果p!=NULL，使用p
		MemPool(MemPool* p=NULL);
		~MemPool();
	};

	//后续各个类的构造函数中，有个MemPool参数，如果该参数为0，则创建pool，从pool分配内存；如果不为0，则从传入的pool分配内存
	//如果对象的创建销毁非常频繁的话，最好使用非NULL的MemPool参数

	//原子加减操作
	class Atomic: protected MemPool {
		pj_atomic_t* ato;
	public:
		Atomic(int initial = 0, MemPool* pool1=NULL);
		~Atomic();
		void set(int val);
		int get() ;
		int add(int val = 0);
	};

	//互斥对象
	class Mutex: protected MemPool {
		pj_mutex_t* mutex;
	public:
		Mutex(MemPool* p = NULL);
		~Mutex();
		void lock();
		void unlock();
		bool trylock();
	};
	
	//自动锁
	class Autolock {
		Mutex& mutex;
	public:
		Autolock(Mutex& mutex1): mutex(mutex1) { mutex.lock(); }
		~Autolock() { mutex.unlock(); }
	};

	//信号量
	class Semaphore: protected MemPool {
		pj_sem_t* sem;
	public:
		Semaphore(int initial, int max = 1024*1024, MemPool* pool1=NULL);
		~Semaphore() ;
		void wait();
		bool trywait();
		void post();
	};

	//生产者消费者缓冲区，若每次添加/删除一个对象的开销太大，则可以使用addAll/removeAll
	template<class T> class ProConPool: public Mutex {
		int sz; //由于gcc的list实现中，当容器中的元素很多时，size调用会很慢，所以单独维护一个size字段
		std::list<T> data;
		Semaphore available; //如果缓冲区中有对象，则available为true
	public:
		ProConPool():sz(0), available(0) {}
		void add(T product) { Autolock al(*this); if (data.empty()) available.post(); data.push_back(product); sz++; }
		//如果缓冲区中没有消息，此函数会阻塞等待消息，不等待则应当调用tryRemove
		T remove() { available.wait(); Autolock al(*this); T ret = data.front(); data.pop_front(); sz--; if (!data.empty()) available.post(); return ret; }
		bool tryRemove(T& val) { Autolock al(*this); bool avail = available.trywait(); if (!avail) return false; val = data.front(); data.pop_front(); sz--; if (!data.empty()) available.post(); return true; }
		//每个add，remove都要进行锁定，因此为了效率考虑，可以一次把整个list添加，可以把整个缓冲区中的元素保存到list
		void addAll(std::list<T>& initems) { Autolock al(*this); if (data.empty() && !initems.empty()) available.post(); sz += initems.size(); data.splice(data.end(), initems); }
		//如果缓冲区中没有消息，此函数会阻塞等待消息，不等待则应当调用tryRemoveAll
		void removeAll(std::list<T>& resitems){ available.wait(); Autolock al(*this); resitems.splice(resitems.end(), data); sz=0; }
		bool tryRemoveAll(std::list<T>& resitems) { Autolock al(*this); bool avail = available.trywait(); if (!avail) return false; resitems.splice(resitems.end(), data); sz=0; return true; }
		int size() { Autolock al(*this); return sz; }
	};

	struct ParsedTime {
		int wday, day, mon, year, sec, min, hour, msec;
	};
	//时间处理
	class Time {
	public:
		int sec, msec;
		Time(int sec1, int msec1): sec(sec1), msec(msec1) {}
		Time(): sec(0), msec(0) {}
		//pj_time_val只有两个成员：{ int sec, msec; }参见pjlib中types.h
		static Time getTimeofday();
		//pj_parsed_time有多个成员用于表示年月日时分秒，参见pjlib的types.h
		static ParsedTime value2Parsed(Time* tv);
		static Time parsed2Value(ParsedTime* pt);
		static Time localToGmt(Time* tv);
		static Time gmtToLocal(Time* tv) ;

		static Time getTickcount() ;
		static int getTickDiff(Time* tv1, Time* tv2) ;
		static void addTick(Time* tv, int millisec) ;

	};

	//线程类
	class Thread: public MemPool {
		SimpleCallback* sc;
		pj_thread_t* pthread;
		static int threadproc(void* p);
		Thread(SimpleCallback* callback, const char* name1) ;
	public:
		~Thread() ;
		//等待线程结束
		void join() ;
		static void sleep(unsigned msec) ;
		//启动一个线程，构造函数为私有，只能通过下面函数启动线程
		template<class C> static Thread* startThread(const char* name, C* obj, int (C::*mfn)(void*), void* arg){
			SimpleCallback* sc = makeSimpleCallback(obj, mfn, arg);
			return new Thread(sc, name);
		}

	};
}
