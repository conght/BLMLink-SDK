LIB_DEP = third-party/pj-project 
LIB_DIRS =$(LIB_DEP) third-party/polarssl/library src example

ifdef MINSIZE
MAKE_FLAGS := MINSIZE=1
endif

all clean:
	for dir in $(LIB_DIRS); do \
		if $(MAKE) $(MAKE_FLAGS) -C $$dir $@; then \
		    true; \
		else \
		    exit 1; \
		fi; \
	done


dep depend distclean print realclean:
	for dir in $(LIB_DEP); do \
		if $(MAKE) $(MAKE_FLAGS) -C $$dir $@; then \
		    true; \
		else \
		    exit 1; \
		fi; \
	done

lib:
	for dir in $(LIB_DIRS); do \
		if $(MAKE) $(MAKE_FLAGS) -C $$dir all; then \
		    true; \
		else \
		    exit 1; \
		fi; \
	done; \
	