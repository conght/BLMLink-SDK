include ../../../../build.mak
CFLAGS	+= -I../include
LIBDIR = ../../lib/
OBJDIR = obj-$(CONFTAG)
LIBFILE = $(LIBDIR)/libpolarssl-$(CONFTAG).a
OBJS=	aes.o		arc4.o		asn1parse.o		\
		asn1write.o base64.o	bignum.o		\
		blowfish.o	camellia.o					\
		certs.o		cipher.o	cipher_wrap.o	\
		ctr_drbg.o	debug.o		des.o			\
		dhm.o		entropy.o	entropy_poll.o	\
		error.o		gcm.o		havege.o		\
		md.o		md_wrap.o	md2.o			\
		md4.o		md5.o		net.o			\
		padlock.o	pbkdf2.o	pem.o			\
		pkcs11.o								\
		rsa.o		sha1.o		sha2.o			\
		sha4.o		ssl_cache.o	ssl_cli.o		\
		ssl_srv.o								\
		ssl_tls.o	timing.o	version.o		\
		x509parse.o	x509write.o	xtea.o
OBJS2 = $(foreach file, $(OBJS), $(OBJDIR)/$(file))
all: mkdir1 $(LIBFILE) 

mkdir1:
	[ ! -d $(OBJDIR) ] && mkdir $(OBJDIR) || echo

dep:
	echo "empty dep"

$(LIBFILE): $(OBJS2)
	echo "  AR    $@"
	$(AR) r $@ $(OBJS2)
	echo "  RL    $@"
	$(AR) s $@


#.c.o:
$(OBJS2):$(OBJDIR)/%.o:%.c
	echo "  CC    $<"
	$(CC) $(CFLAGS) $(OFLAGS) -c $< -o $@

clean:
ifndef WINDOWS
	rm -rf $(OBJDIR) *.o ${LIBFILE}
endif
