include ../../build.mak

LDFLAGS +=-L../../../pj-project/pjlib/lib -L../../../pj-project/pjlib-util/lib -L../../../lib
LDFLAGS += $(LIBS) -lpolarssl-$(CONFTAG) -lz
CPPFLAGS += -I../third-party/pj-project/pjlib/include -I../third-party/pj-project/pjlib-util/include -I../include -I../third-party/polarssl/include
CPPFLAGS += $(PJFLAGS) $(CF_STL_ANDROID)
LIBDIR=../lib

OBJS_CUR = blmbase.o blmcomm.o blmutil.o util_fun.o CppSQLite3.o
OBJS_IMP = cJSON.o sqlite3.o

OBJS_CUR2 = $(foreach file, $(OBJS_CUR), $(OBJDIR)/$(file))
OBJS_IMP2 = $(foreach file, $(OBJS_IMP), $(OBJDIR)/$(file))

OBJDIR = obj-$(CONFTAG)

OBJS2=$(OBJS_CUR2) $(OBJS_IMP2)

LIBFILE = $(LIBDIR)/libblmlib-$(CONFTAG).a


all: mkdir1 $(LIBFILE)

mkdir1:
	[ ! -d $(OBJDIR) ] && mkdir $(OBJDIR) || echo


LIBPOLAR=../third-party/lib/libpolarssl-$(CONFTAG).a
LIBPJ=../third-party/pj-project/pjlib/lib/libpj-$(CONFTAG).a
LIBPJUTIL=../third-party/pj-project/pjlib-util/lib/libpjlib-util-$(CONFTAG).a

$(LIBFILE): $(OBJS2) $(LIBPOLAR) $(LIBPJ) $(LIBPJUTIL) 
	cd $(OBJDIR); ar x ../$(LIBPOLAR); mv base64.o base64_polar.o; mv sha1.o sha1_polar.o; mv md5.o md5_polar.o
	cd $(OBJDIR); ar x ../$(LIBPJUTIL); mv errno.o errno_pjlib.o; mv string.o string_polar.o
	cd $(OBJDIR); ar x ../$(LIBPJ)
	ar cr $@ $(OBJDIR)/*.o
	
$(OBJS_CUR2): $(OBJDIR)/%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(OFLAGS) -c $< -o $@
	
$(OBJS_IMP2):$(OBJDIR)/%.o: imp/%.c
	$(CC) $(CPPFLAGS) $(OFLAGS) -c $< -o $@

#.cpp.o:
#	$(CXX) $(CPPFLAGS) $(OFLAGS) -c $<

clean:
ifndef WINDOWS
	rm -rf $(OBJDIR) $(LIBFILE)
endif

