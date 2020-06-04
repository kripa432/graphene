# Use one of the following commands to build the manifest for Python3:
#
# - make                Building for Linux
# - make DEBUG=1        Building for Linux (with Graphene debug output)
# - make SGX=1          Building for SGX
# - make SGX=1 DEBUG=1  Building for SGX (with Graphene debug output)
#
# Use `make clean` to remove Graphene-generated files.

# Constants


INSTALL_DIR ?= install
INSTALL_DIR_ABSPATH = $(shell pwd)/install

# By default, Graphene runs the system Python 3.5 executable.
PYTHONVERSION ?= python3.7
version=3.7.2
#INSTALL_DIR=$(shell pwd)/install
PYTHONPATH ?= $(INSTALL_DIR)

PYTHONHOME = $(PYTHONPATH)/lib/$(PYTHONVERSION)
PYTHONEXEC = $(PYTHONPATH)/bin/$(PYTHONVERSION)
PYTHONDISTHOME = $(PYTHONPATH)/lib/python3.7/site-packages
PYTHONSHORTVERSION = $(subst python,,$(subst .,,$(PYTHONVERSION)))

# Relative path to Graphene root
GRAPHENEDIR ?= ../../../../..

ifeq ($(DEBUG),1)
GRAPHENEDEBUG = inline
else
GRAPHENEDEBUG = none
endif

.PHONY: all
all: bzip2.manifest pal_loader
ifeq ($(SGX),1)
all: bzip2.manifest.sgx python.token bzip2.sig
endif

# Python dependencies (generate from ldd):
#
# For SGX, the manifest needs to list all the libraries loaded during the
# execution, so that the signer can include the file checksums.
#
# The dependencies are generated from the ldd results.

# We need to replace Glibc dependencies with Graphene-specific Glibc. The Glibc
# binaries are already listed in the manifest template, so we can skip them
# from the ldd results.
GLIBC_DEPS = linux-vdso /lib64/ld-linux-x86-64 libc libm librt libdl libutil libpthread

# Define the python libraries which are dynamically loaded.
PY_LIBS = $(PYTHONHOME)/lib-dynload/_hashlib.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so \
	  $(PYTHONHOME)/lib-dynload/_ctypes.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so \
	  $(PYTHONHOME)/lib-dynload/_ssl.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so \

PY_LIBS_TRUSTED_LIBS = "sgx.trusted_files.hashlib = file:$(PYTHONHOME)/lib-dynload/_hashlib.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so\\\\n" \
	  "sgx.trusted_files.ctypes = file:$(PYTHONHOME)/lib-dynload/_ctypes.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so\\\\n" \
	  "sgx.trusted_files.ssl = file:$(PYTHONHOME)/lib-dynload/_ssl.cpython-$(PYTHONSHORTVERSION)m-x86_64-linux-gnu.so\\\\n" \

# Listing all the Python dependencies, besides Glibc libraries
.INTERMEDIATE: python-ldd
python-ldd:

.INTERMEDIATE: python-deps
python-deps: python-ldd

# Generating manifest rules for Python dependencies
.INTERMEDIATE: python-trusted-libs
python-trusted-libs: python-deps

bzip2.manifest: bzip2.manifest.template python-trusted-libs install/bin/bzip2
	sed -e 's|$$(GRAPHENEDIR)|'"$(GRAPHENEDIR)"'|g' \
		-e 's|$$(GRAPHENEDEBUG)|'"$(GRAPHENEDEBUG)"'|g' \
		-e 's|$$(INSTALL_DIR)|'"$(INSTALL_DIR)"'|g' \
		-e 's|$$(INSTALL_DIR_ABSPATH)|'"$(abspath $(INSTALL_DIR))"'|g' \
		-e 's|$$(PYTHONDISTHOME)|'"$(PYTHONDISTHOME)"'|g' \
		-e 's|$$(PYTHONHOME)|'"$(PYTHONHOME)"'|g' \
		-e 's|$$(PYTHONEXEC)|'"$(PYTHONEXEC)"'|g' \
		-e 's|$$(PYTHON_TRUSTED_LIBS)|'"`cat python-trusted-libs`"'|g' \
		$< > $@
install/bin/bzip2:
	cd src && $(MAKE)
	cd src && $(MAKE) install PREFIX=$(PWD)/install
# Python manifests for SGX:
#   Generating the SGX-specific manifest (bzip2.manifest.sgx), the enclave signature,
#   and the token for enclave initialization.

bzip2.manifest.sgx: bzip2.manifest
	$(GRAPHENEDIR)/Pal/src/host/Linux-SGX/signer/pal-sgx-sign \
		-libpal $(GRAPHENEDIR)/Runtime/libpal-Linux-SGX.so \
		-key $(GRAPHENEDIR)/Pal/src/host/Linux-SGX/signer/enclave-key.pem \
		-manifest $< -output $@

bzip2.sig: bzip2.manifest.sgx

python.token: bzip2.sig
	$(GRAPHENEDIR)/Pal/src/host/Linux-SGX/signer/pal-sgx-get-token -output $@ -sig $<

# Extra executables
pal_loader:
	ln -s $(GRAPHENEDIR)/Runtime/pal_loader $@


install/bin/python3.7: Python-$(version)/configure
	cd Python-$(version); ./configure --prefix=$(INSTALL_DIR)
	cd Python-$(version); make
	cd Python-$(version); make install


Python-$(version)/configure:  Python-$(version).tar.xz
	tar -xvmf $?

Python-$(version).tar.xz:
	wget https://www.python.org/ftp/python/3.7.2/Python-$(version).tar.xz

.PHONY: check
check: all
	./pal_loader bzip2.manifest scripts/test-numpy.py > OUTPUT 2> /dev/null
	@sleep 1  # to make sure Bash child processes flush output under Graphene-SGX
	@grep -q "dot: " OUTPUT && echo "[ Success 1/2 ]"
	@rm OUTPUT

	./pal_loader bzip2.manifest scripts/test-scipy.py > OUTPUT 2> /dev/null
	@sleep 1
	@grep -q "cholesky: " OUTPUT && echo "[ Success 2/2 ]"
	@rm OUTPUT

.PHONY: clean
clean:
	$(RM) *.manifest *.manifest.sgx *.token *.sig pal_loader OUTPUT*
	$(RM) -r scripts/__pycache__

.PHONY: distclean
distclean: clean
