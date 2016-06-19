#include <blm/blmutil.h>
#include <blm/blmcomm.h>
#include <CppSQLite3.h>
using namespace BlmComm;

void testCppsqlite() {
	const char* gszFile = "test.db";
	try {
		CppSQLite3DB db;
		db.open(gszFile);
		db.execDML("create table emp(empno int, empname char(20));");
		int nRows = db.execDML("insert into emp values (7, 'David Beckham');");
		nRows = db.execDML("update emp set empname = 'Christiano Ronaldo' where empno = 7;");
		blmlog(3, "main", "sqlite insert and update ok and count in emp is: %d", db.execScalar("select count(*) from emp;"));
	} catch (CppSQLite3Exception& e) {
		blmlog(0, "main", "sqlite exception: %d %s", e.errorCode(), e.errorMessage());
	}
}
void testJson() {
	const char* s2 = "[1, 2,ab]";
	const char* str = "{a:\"value\",\"b\":5}";
	Json j;
	checktrue = j.parse(str);
	blmlog(3, "blmcomm", "json[name] is: %s", j.get("a").getName());
	checktrue = j.parse(s2);
	blmlog(3, "blmcomm", "array size: %d first obj: %d second: %d", j.getSize(), j.getInt(int(0)), j.getInt(1));
}

////testClient1//////////////
class ClientTester {
	BlmClient* pcli;
public:
	bool timerfired;
	ClientTester(BlmClient* cli) { pcli = cli; timerfired = false; }
	int onConnect(void*) { 
		//发送登录消息
		string cont = formatString("{uinput:\"%s\",pwd:\"%s\",ver:\"%s\",mac:\"\",tp:\"%s\"}", "dongfuye", "yedongfu", "2", "1");
		pcli->sendMsg(0x00, 0x03, cont.c_str(), cont.size());
		return 0;
	}
	int handleLogin(int mt, int st, const string& data, int msgid, void*) {
		blmlog(3, "main", "login result: %s", data.c_str());
		return 0;
	}
	int onTimer(void* p) {
		long id = (long)p;
		timerfired = true;
		blmlog(3, "main", "timer: %ld is fired", id);
		return 0;
	}
	int onTimerlog(void*) {
		blmlog(3, "main", "this is a log");
		return 0;
	}
	int onUrlDownloaded(const string& data, void* url) {
		blmlog(3, "blmcomm", "result size: %d  url: %s", data.size(), url);
		return 0;
	}

};
void testClient1() {
	//BlmIoqueue ioq;
	////BlmClient代表一个与服务器的连接
	//BlmClient blmclient(&ioq);

	BlmIoqueue& ioq = SingleComm::instance();
	BlmClient& blmclient = SingleComm::instance();
	ClientTester mtest(&blmclient);
	//http使用
	//ioq.wget("http://192.168.129.1", &mtest, &ClientTester::onUrlDownloaded, 5000, (void*)"http://192.168.129.1");
	//定时器使用
	//ioq.addTimerCallback(100, &mtest, &ClientTester::onTimerlog);
	//ioq.addTimerCallback(3000, &mtest, &ClientTester::onTimer, (void*)3);
	//客户端连接
	blmclient.addConnectCallback(&mtest, &ClientTester::onConnect);
	blmclient.addMsgCallback(0x00, &mtest, &ClientTester::handleLogin);
	blmclient.connectFastest("127.0.0.1 58.214.1.186", 5221);
	while(!mtest.timerfired) {
		ioq.poll(50);
	}
	//下面是释放资源，如果没有remove，BlmClient也会释放资源，产生一个警告
	blmclient.removeObjCallbacks(&mtest);
	ioq.removeObjTimers(&mtest);
}
////////////testClient finished

///////////testClient2//////////////////////
////testClient1//////////////
class JavaMsgs: public ProConPool<int>, public Singleton<JavaMsgs> {};

class ClientTester2 {
	BlmClient* pcli;
public:
	bool timerfired;
	ClientTester2(BlmClient* cli) { pcli = cli; timerfired = false; }
	int onConnect(void*) { 
		//发送登录消息
		string cont = formatString("{uinput:\"%s\",pwd:\"%s\",ver:\"%s\",mac:\"\",tp:\"%s\"}", "dongfuye", "yedongfu", "2", "1");
		pcli->sendMsg(0x00, 0x03, cont.c_str(), cont.size());
		return 0;
	}
	int handleLogin(int mt, int st, const string& data, int msgid, void*) {
		blmlog(3, "main", "login result: %s", data.c_str());
		JavaMsgs::instance().add(mt*256+st);
		timerfired = true;
		return 0;
	}
	int onClose(void*) {
		JavaMsgs::instance().add(255);
		timerfired = true;
		return 0;
	}
};

void testClient2() {
	JavaMsgs javaMsgs;
	//BlmIoqueue ioq;
	////BlmClient代表一个与服务器的连接
	//BlmClient blmclient(&ioq);

	BlmIoqueue& ioq = SingleComm::instance();
	BlmClient& blmclient = SingleComm::instance();
	ClientTester2 mtest(&blmclient);
	//客户端连接
	blmclient.addConnectCallback(&mtest, &ClientTester2::onConnect);
	blmclient.addCloseCallback(&mtest, &ClientTester2::onClose);
	blmclient.addMsgCallback(0x00, &mtest, &ClientTester2::handleLogin);
	blmclient.connect("58.214.1.186", 5221);
	while(!mtest.timerfired) {
		ioq.poll(50);
	}
	//下面是释放资源，如果没有remove，BlmClient也会释放资源，产生一个警告
	SingleComm::instance().removeObj(&mtest);
	blmlog(3, "main", "java msgs size is: %d", javaMsgs.size());
	while (javaMsgs.size() > 0) {
		int msg = javaMsgs.remove();
		blmlog(3, "main", "java msgs item is: %d", msg);
	}
}
/////////////testClient2 finished/////////////////////////
class TestThread {
public: 
	int threadfun(void*) {
		blmlog(3, "blmcomm", "my thread function called");
		return 0;
	}
};

void testthread() {
	TestThread tt;
	Thread* p = Thread::startThread("testthread", &tt, &TestThread::threadfun, 0);
	//析构时，会等待线程退出
	auto_ptr<Thread> release(p);
}

void teststrNoCase(const char* s1, const char* s2) {
	blmlog(3, "main", "strcmpNocase result of %s %s is: %d", s1, s2, strcmpNoCase(s1, s2));
}
void teststr() {
	teststrNoCase("a", "");
	teststrNoCase("ab", "ab");
	teststrNoCase("Ab", "aB");
}
int main() {
	blminit();
	blmSetLog(3);
	blmlog(3, "main", "program begin 2");
	{
		testCppsqlite();
		string s = Zlib::compress("abc", 3);
		blmlog(3, "main", "the size of compressed: %d", s.size());
		testJson();
		teststr();
		testthread();
		SingleComm comm;
		//testClient1();
		testClient2();
	}
	blmlog(3, "main2", "program exit");
	blmshutdown();
	return 0;
}

