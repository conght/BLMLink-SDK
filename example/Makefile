include ../../build.mak

BINPATH=../bin/blmexample-$(CONFTAG)
CPPFLAGS += -I../include $(CF_STL_ANDROID)

LIBFILE = libblmlib-$(CONFTAG).a

LDFLAGS += -L../lib -lz -lblmlib-$(CONFTAG)  $(LD_STL_ANDROID) $(LIBS)

OBJDIR = obj-$(CONFTAG)

OBJS=blmcomm_testmain.o
OBJS2 = $(foreach file, $(OBJS), $(OBJDIR)/$(file))

all:mkdir1 $(BINPATH)
mkdir1:
		[ ! -d $(OBJDIR) ] && mkdir $(OBJDIR) || echo


$(BINPATH): $(OBJS2)
	$(CXX) -o$(BINPATH) $(CPPFLAGS) $(OBJS2) $(LDFLAGS)


$(OBJS2): $(OBJDIR)/%.o:%.cpp
	$(CXX) $(CPPFLAGS) $(OFLAGS) -c $< -o $@

#.cpp.o:
#	$(CXX) $(CPPFLAGS) $(OFLAGS) -c $<

clean:
ifndef WINDOWS
	rm -rf $(OBJDIR) $(BINPATH)
endif

