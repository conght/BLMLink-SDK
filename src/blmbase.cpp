#include <blm/blmbase.h>
#include <pjlib.h>
#include <pjlib-util.h>
namespace BlmComm {
	static pj_caching_pool g_pj_cp;
	pj_caching_pool& pj_cp = g_pj_cp;
	void blminit() {
		pj_init();
		pjlib_util_init();
		pj_caching_pool_init(&pj_cp, NULL, 0);
	}
	void blmshutdown() {
		pj_caching_pool_destroy(&pj_cp);
	}

	void CheckerPjStatus::operator = (int status){
		if (status != PJ_SUCCESS && status != PJ_EPENDING) {
			char errbuf[1024];
			pj_strerror(status, errbuf, 1024);
			blmlog(0, "checkpj", "file: %s line: %d pjret: %d pjmsg: %s\nerrno: %d errmsg: %s", fileName.c_str(), lineNo, status, errbuf, errno, strerror(errno));
		}
	}

	void CheckerTrue::operator = (bool status) {
		if (status != true) {
			blmlog(0, "checktrue", "file: %s line: %d errno: %d errormsg: %s", fileName.c_str(), lineNo, errno, strerror(errno));
		}
	}

	void CheckerTrue::operator = (void* p) {
		if (!p) {
			blmlog(0, "checktrue", "file: %s line: %d errno: %d errormsg: %s", fileName.c_str(), lineNo, errno, strerror(errno));
		}
	}

	static int blmlog_level = 3;
	static set<string> blmlog_modules;

	void blmlog(int detaillevel, const char* module, const char* fmt, ...) {
		//如果modules为空，则打印所有模块的日志
		if (detaillevel > blmlog_level || !blmlog_modules.empty() && blmlog_modules.find(module) == blmlog_modules.end())
			return;
		va_list va;	
		va_start(va,fmt); 
		pj_log(module, detaillevel, fmt, va);
		va_end(va);
	}

	void blmlogDump(int detaillevel, const char* module, const char* tag, const void* data, int len) {
		//如果modules为空，则打印所有模块的日志
		if (detaillevel > blmlog_level || !blmlog_modules.empty() && blmlog_modules.find(module) == blmlog_modules.end())
			return;
		blmlog(detaillevel, module, "dumping %s %x bytes:", tag, len);
		blmlog(detaillevel, module, "\n%s", getHexDump(data, len).c_str());
	}

	void blmSetLog(int detail_level, const char* modules, const char* logfile) {
		blmlog_level = detail_level;
		vector<string> ms = splitString(modules, ' ');
		for (size_t i = 0; i < ms.size(); i ++) {
			blmlog_modules.insert(ms[i]);
		}
	}


	void blmSetLogOutput(void (*logfun)(int, const char*, int)) {
		pj_log_set_log_func(logfun);
	}

	MemPool::MemPool(MemPool* p) { pool = p ? p->pool : pj_pool_create(&pj_cp.factory, NULL, 256, 8192, NULL); owned = p ==NULL; checktrue = pool; }
	MemPool::~MemPool() { if (owned) pj_pool_release(pool); }

	Atomic::Atomic(int initial, MemPool* pool1):MemPool(pool1) { checkpj = pj_atomic_create(pool, initial, &ato); checktrue = ato; }
	Atomic::~Atomic() { checkpj = pj_atomic_destroy(ato); }
	void Atomic::set(int val) { pj_atomic_set(ato, val); }
	int Atomic::get() { return pj_atomic_get(ato); }
	int Atomic::add(int val) { return pj_atomic_add_and_get(ato, val); }

	Mutex::Mutex(MemPool* p ): MemPool(p) { checkpj = pj_mutex_create_recursive(pool, "", &mutex); checktrue = mutex; }
	Mutex::~Mutex() { checkpj = pj_mutex_destroy(mutex); }
	void Mutex::lock() { checkpj = pj_mutex_lock(mutex); }
	void Mutex::unlock() { checkpj = pj_mutex_unlock(mutex); }
	bool Mutex::trylock() { return PJ_SUCCESS == pj_mutex_trylock(mutex); }

	Semaphore::Semaphore(int initial, int max, MemPool* pool1): MemPool(pool1) { pj_sem_create(pool, "", (unsigned)initial, (unsigned)max, &sem); checktrue = sem;}
	Semaphore::~Semaphore() { checkpj = pj_sem_destroy(sem); }
	void Semaphore::wait() { checkpj = pj_sem_wait(sem); }
	bool Semaphore::trywait() { return PJ_SUCCESS == pj_sem_trywait(sem); }
	void Semaphore::post() { checkpj = pj_sem_post(sem); }

	Time Time::getTimeofday() { pj_time_val tv; checkpj = pj_gettimeofday(&tv); return Time(tv.sec, tv.msec); }
	//pj_parsed_time有多个成员用于表示年月日时分秒，参见pjlib的types.h
	ParsedTime Time::value2Parsed(Time* tv) { 
		pj_time_val tv1 = {tv->sec, tv->msec};
		ParsedTime pt;
		checkpj = pj_time_decode(&tv1, (pj_parsed_time*) &pt); 
		return pt; 
	}
	Time Time::parsed2Value(ParsedTime* pt) { pj_time_val tv; checkpj = pj_time_encode((pj_parsed_time*)pt, &tv); return Time(tv.sec, tv.msec);}
	//Time Time::localToGmt(Time* tv) { pj_time_val tv1 = {tv->sec, tv->msec}; checkpj = pj_time_local_to_gmt(&tv1); return Time(tv1.sec, tv1.msec); }
	//Time Time::gmtToLocal(Time* tv) { pj_time_val tv1 = {tv->sec, tv->msec}; checkpj = pj_time_gmt_to_local(&tv1); return Time(tv1.sec, tv1.msec); }

	Time Time::getTickcount() { pj_time_val tv; checkpj = pj_gettickcount(&tv); return Time(tv.sec, tv.msec); }
	int Time::getTickDiff(Time* tv1, Time* tv2) { return (tv1->sec - tv2->sec) * 1000 + tv1->msec - tv2->msec; }
	void Time::addTick(Time* tv, int millisec) { tv->sec += (millisec + tv->msec) / 1000; tv->msec = (tv->msec + millisec) % 1000; }


	int Thread::threadproc(void* p) { Thread* pt = (Thread*)p;	return pt->sc->call();	}
	Thread::Thread(SimpleCallback* callback, const char* name1): sc(callback) {
		checkpj = pj_thread_create(pool, name1, Thread::threadproc, this, 64*1024, 0, &pthread);
	}
	Thread::~Thread() { join(); delete sc; checkpj = pj_thread_destroy(pthread); }
	void Thread::join() { checkpj = pj_thread_join(pthread); }
	void Thread::sleep(unsigned msec) { checkpj = pj_thread_sleep(msec); }

}
