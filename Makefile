CC 						?= gcc
AR 						?= ar
PYTHON 					?= python3
RM 						?= rm -f
MKDIR_P 				?= mkdir -p
INSTALL 				?= install

PREFIX 					?= /usr/local
DESTDIR 				?=
BINDIR 					?= $(PREFIX)/bin
LIBDIR 					?= $(PREFIX)/lib
DATADIR 				?= $(PREFIX)/share
CPLLIBDIR 				?= $(DATADIR)/cpl/include
CPLRUNTIMEDIR 			?= $(LIBDIR)/cpl
DOCDIR 					?= $(DATADIR)/doc/cpl
BASH_COMPLETION_DIR 	?= $(DATADIR)/bash-completion/completions
ZSH_COMPLETION_DIR 		?= $(DATADIR)/zsh/site-functions
FISH_COMPLETION_DIR 	?= $(DATADIR)/fish/vendor_completions.d
VERSION 				?= 3.7_X

BUILD 					?= debug
AVAILABLE_MEMORY 		?= 67108864
LOGS 					?=
PRINT_PARSE 			?= 1
ENABLE_Z3 				?= auto
INPUT 					?= examples/print.cpl
UTEST 					?= code_utesting/exec/raw
STD_UTEST 				?= std_utesting
CPLLIB_SRC_DIR 		    ?= cpllib
VSCODE_DIR 				?= vscode
VSCODE_DOCKER_IMAGE 	?= cpl-extension
VSCODE_ABS_DIR 		    := $(abspath $(VSCODE_DIR))
VSCODE_OUTPUT_DIR 		?= $(VSCODE_ABS_DIR)/output
DOCS_BACKEND_BUILD_DIR 	?= docs/back/.build
DOCS_BACKEND_PLATFORM 	?= ../$(DOCS_BACKEND_BUILD_DIR)
DOCS_BACKEND_COMPILER 	?= $(DOCS_BACKEND_BUILD_DIR)/cplc
DOCS_BACKEND_OUTPUT 	?= docs/back/cpl_docs_backend

UNAME_S ?= $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	RUN_ARGS ?= 				\
		--arch x86_64 			\
		--sys-type macho64 		\
		--asm-format macho64 	\
		--linker clang
else
	RUN_ARGS ?= 				\
		--arch x86_64 			\
		--sys-type linux64 		\
		--asm-format elf64 		\
		--linker gcc 			\
		--linker-no-pie 
endif

Z3_AVAILABLE := $(shell pkg-config --exists z3 2>/dev/null && echo 1 || echo 0)
ifeq ($(ENABLE_Z3),auto)
	Z3_ENABLED := $(Z3_AVAILABLE)
else
	Z3_ENABLED := $(ENABLE_Z3)
endif

ifeq ($(Z3_ENABLED),1)
	Z3_CFLAGS ?= $(shell pkg-config --cflags z3 2>/dev/null)
	Z3_LDLIBS ?= $(shell pkg-config --libs z3 2>/dev/null)
	ifeq ($(strip $(Z3_LDLIBS)),)
		Z3_LDLIBS = -lz3
	endif
else
	Z3_CFLAGS :=
	Z3_LDLIBS :=
endif

PLATFORM ?= $(shell uname -s | tr '[:upper:]' '[:lower:]')-$(shell uname -m | tr '[:upper:]' '[:lower:]')

SOURCES 		:= $(sort $(shell find src std -type f -name '*.c'))
OUTPUT 			= builds/$(PLATFORM)/cplc
OBJDIR 			= builds/$(PLATFORM)/obj
OBJECTS 		:= $(patsubst %.c,$(OBJDIR)/%.o,$(SOURCES))
DEPS 			:= $(OBJECTS:.o=.d)
CPLLIB_SOURCES  := $(sort $(shell if [ -d "$(CPLLIB_SRC_DIR)" ]; then find "$(CPLLIB_SRC_DIR)" -maxdepth 1 -type f -name '*.cpl'; fi))
CPLLIB_IMPLS 	:= $(sort $(shell if [ -d "$(CPLLIB_SRC_DIR)" ]; then find "$(CPLLIB_SRC_DIR)" -type f -name '*.cpl' ! -name '*_h.cpl'; fi))
CPLLIB_BUILDDIR := builds/$(PLATFORM)/cpllib
CPLLIB_OBJDIR   := $(CPLLIB_BUILDDIR)/obj
CPLLIB_OBJS     := $(patsubst $(CPLLIB_SRC_DIR)/%.cpl,$(CPLLIB_OBJDIR)/%.o,$(CPLLIB_IMPLS))
CPLLIB_ARCHIVE  := $(CPLLIB_BUILDDIR)/libcpl.a
COMPLETION_DIR  ?= completions

CPPFLAGS 		+= -Iinclude -DALLOC_BUFFER_SIZE=$(AVAILABLE_MEMORY) -DCPL_DEFAULT_INCLUDE_DIR=\"$(CPLLIBDIR)\" -DCPL_DEFAULT_RUNTIME_LIB=\"$(CPLRUNTIMEDIR)/libcpl.a\"
CFLAGS   		+= -Wall -Wno-int-conversion
LDFLAGS  		+=
LDLIBS   		+=

ifeq ($(BUILD),debug)
	CFLAGS += -g -O0
else ifeq ($(BUILD),release)
	CFLAGS += -O2
else
	$(error Unknown BUILD=$(BUILD), use debug or release)
endif

ifeq ($(PRINT_PARSE),1)
	CPPFLAGS += -DPRINT_PARSE
endif

ifeq ($(Z3_ENABLED),1)
	CPPFLAGS += -DCPL_ENABLE_Z3 $(Z3_CFLAGS)
	LDLIBS += $(Z3_LDLIBS)
endif

ifneq ($(filter error,$(LOGS)),)
	CPPFLAGS += -DERROR_LOGS
endif

ifneq ($(filter warn,$(LOGS)),)
	CPPFLAGS += -DWARNING_LOGS
endif

ifneq ($(filter info,$(LOGS)),)
	CPPFLAGS += -DINFO_LOGS
endif

ifneq ($(filter debug,$(LOGS)),)
	CPPFLAGS += -DDEBUG_LOGS
endif

ifneq ($(filter io,$(LOGS)),)
	CPPFLAGS += -DIO_OPERATION_LOGS
endif

ifneq ($(filter mem,$(LOGS)),)
	CPPFLAGS += -DMEM_OPERATION_LOGS
endif

ifneq ($(filter logging,$(LOGS)),)
	CPPFLAGS += -DLOGGING_LOGS
endif

ifneq ($(filter special,$(LOGS)),)
	CPPFLAGS += -DSPECIAL_LOGS
endif

all: $(OUTPUT) ## Build the compiler with the current configuration.

check-cpllib-src:
	@if [ ! -d "$(CPLLIB_SRC_DIR)" ] || [ -z "$$(find "$(CPLLIB_SRC_DIR)" -maxdepth 1 -type f -name '*.cpl' -print -quit 2>/dev/null)" ]; then \
		echo "error: CPL library source directory '$(CPLLIB_SRC_DIR)' is missing or empty."; \
		echo "       If it is a submodule, run: git submodule update --init --recursive $(CPLLIB_SRC_DIR)"; \
		exit 1; \
	fi

check-vscode-src:
	@if [ ! -f "$(VSCODE_DIR)/package.json" ] || [ ! -f "$(VSCODE_DIR)/Dockerfile" ]; then \
		echo "error: VS Code extension directory '$(VSCODE_DIR)' is missing or incomplete."; \
		echo "       If it is a submodule, run: git submodule update --init --recursive $(VSCODE_DIR)"; \
		exit 1; \
	fi

$(OUTPUT): $(OBJECTS)
	@$(MKDIR_P) $(dir $@)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LDLIBS)

$(OBJDIR)/%.o: %.c Makefile
	@$(MKDIR_P) $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

$(CPLLIB_OBJDIR)/%.o: $(CPLLIB_SRC_DIR)/%.cpl $(OUTPUT) | check-cpllib-src
	@$(MKDIR_P) $(dir $@)
	$(OUTPUT) $(RUN_ARGS) -c --output $@ $<

$(CPLLIB_ARCHIVE): $(CPLLIB_OBJS) | check-cpllib-src
	@$(MKDIR_P) $(dir $@)
	$(RM) $@
	$(AR) rcs $@ $^

cpllib: $(CPLLIB_ARCHIVE) ## Build the CPL runtime static library.

docs-backend: ## Build the CPL HTTP backend for the docs Playground.
	$(MAKE) PLATFORM=$(DOCS_BACKEND_PLATFORM) BUILD=$(BUILD) PRINT_PARSE=$(PRINT_PARSE) ENABLE_Z3=$(ENABLE_Z3) CPLLIB_SRC_DIR=$(CPLLIB_SRC_DIR) all cpllib
	$(DOCS_BACKEND_COMPILER) $(RUN_ARGS) docs/back/main.cpl --output $(DOCS_BACKEND_OUTPUT)

docs-backend-run: docs-backend ## Build and run the CPL docs backend on 127.0.0.1:8000.
	./$(DOCS_BACKEND_OUTPUT)

debug: ## Build a debug compiler.
	$(MAKE) BUILD=debug all

release: ## Build an optimized compiler.
	$(MAKE) BUILD=release PRINT_PARSE=0 all

install: $(OUTPUT) $(CPLLIB_ARCHIVE) | check-cpllib-src ## Install the compiler and CPL standard library under PREFIX.
	$(INSTALL) -d $(DESTDIR)$(BINDIR) $(DESTDIR)$(CPLLIBDIR) $(DESTDIR)$(CPLRUNTIMEDIR) $(DESTDIR)$(DOCDIR)
	$(INSTALL) -d $(DESTDIR)$(BASH_COMPLETION_DIR) $(DESTDIR)$(ZSH_COMPLETION_DIR) $(DESTDIR)$(FISH_COMPLETION_DIR)
	$(INSTALL) -m 0755 $(OUTPUT) $(DESTDIR)$(BINDIR)/cplc
	$(INSTALL) -m 0644 $(CPLLIB_SOURCES) $(DESTDIR)$(CPLLIBDIR)/
	$(INSTALL) -m 0644 $(CPLLIB_ARCHIVE) $(DESTDIR)$(CPLRUNTIMEDIR)/libcpl.a
	$(INSTALL) -m 0644 LICENSE $(DESTDIR)$(DOCDIR)/
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/cplc.bash $(DESTDIR)$(BASH_COMPLETION_DIR)/cplc
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/_cplc $(DESTDIR)$(ZSH_COMPLETION_DIR)/_cplc
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/cplc.fish $(DESTDIR)$(FISH_COMPLETION_DIR)/cplc.fish

package: | check-cpllib-src ## Build a relocatable binary tarball with the standard library.
	$(MAKE) BUILD=release PRINT_PARSE=0 CPLLIB_SRC_DIR=$(CPLLIB_SRC_DIR) -B all cpllib
	$(RM) -r builds/package/cpl-$(VERSION)
	$(INSTALL) -d builds/package/cpl-$(VERSION)/bin builds/package/cpl-$(VERSION)/lib/cpl builds/package/cpl-$(VERSION)/share/cpl/include builds/package/cpl-$(VERSION)/share/doc/cpl
	$(INSTALL) -d builds/package/cpl-$(VERSION)/share/bash-completion/completions builds/package/cpl-$(VERSION)/share/zsh/site-functions builds/package/cpl-$(VERSION)/share/fish/vendor_completions.d
	$(INSTALL) -m 0755 $(OUTPUT) builds/package/cpl-$(VERSION)/bin/cplc
	$(INSTALL) -m 0644 $(CPLLIB_SOURCES) builds/package/cpl-$(VERSION)/share/cpl/include/
	$(INSTALL) -m 0644 $(CPLLIB_ARCHIVE) builds/package/cpl-$(VERSION)/lib/cpl/libcpl.a
	$(INSTALL) -m 0644 LICENSE builds/package/cpl-$(VERSION)/share/doc/cpl/
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/cplc.bash builds/package/cpl-$(VERSION)/share/bash-completion/completions/cplc
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/_cplc builds/package/cpl-$(VERSION)/share/zsh/site-functions/_cplc
	$(INSTALL) -m 0644 $(COMPLETION_DIR)/cplc.fish builds/package/cpl-$(VERSION)/share/fish/vendor_completions.d/cplc.fish
	tar -C builds/package -czf builds/cpl-$(VERSION)-$(PLATFORM).tar.gz cpl-$(VERSION)

run: $(OUTPUT) ## Compile INPUT with the built compiler.
	$(OUTPUT) $(RUN_ARGS) $(INPUT)

test: unit-test std-test cli-test ## Run unit, standard library, and CLI tests.

unit-test: ## Run module tests, e.g. make unit-test UTEST=code_utesting/ast.
	cd tests && $(PYTHON) module_testing.py --path $(UTEST) --compiler $(CC) --output-dir bin

rewrite-test: ## Rewrite OUTPUT blocks for module tests.
	cd tests && $(PYTHON) module_testing.py --path $(UTEST) --compiler $(CC) --output-dir bin --base ../ --force-rewrite

std-test: ## Run std library tests, e.g. make std-test or make std-test STD_UTEST=std_utesting/list.
	@if [ "$(STD_UTEST)" = "std_utesting" ]; then \
		for dir in tests/std_utesting/*; do \
			if [ -d "$$dir" ]; then \
				name=$$(basename "$$dir"); \
				cd tests && $(PYTHON) std_testing.py --path "std_utesting/$$name" --compiler $(CC) --output-dir bin --base ../ || exit 1; \
				cd ..; \
			fi; \
		done; \
	else \
		cd tests && $(PYTHON) std_testing.py --path $(STD_UTEST) --compiler $(CC) --output-dir bin --base ../; \
	fi

cli-test: $(OUTPUT) ## Run builder CLI tests.
	cd tests && CPLC_BINARY=$(abspath $(OUTPUT)) $(PYTHON) cli_testing.py

vscode-docker-build: | check-vscode-src ## Build the VS Code extension Docker image.
	docker build -t $(VSCODE_DOCKER_IMAGE) $(VSCODE_ABS_DIR)

vscode-docker-package: vscode-docker-build | check-vscode-src ## Build and package the VS Code extension in Docker.
	docker run --rm -v $(VSCODE_ABS_DIR):/app -v $(VSCODE_OUTPUT_DIR):/output $(VSCODE_DOCKER_IMAGE)

submodules: ## Initialize repository submodules.
	git submodule update --init --recursive

clean: ## Remove compiler build outputs.
	$(RM) -r builds

clean-tests: ## Remove test binaries.
	$(RM) -r tests/bin

distclean: clean clean-tests ## Remove all generated build/test outputs.

print-sources:
	@printf "%s\n" $(SOURCES)

print-config:
	@echo "CC=$(CC)"
	@echo "BUILD=$(BUILD)"
	@echo "PLATFORM=$(PLATFORM)"
	@echo "OUTPUT=$(OUTPUT)"
	@echo "OBJDIR=$(OBJDIR)"
	@echo "PREFIX=$(PREFIX)"
	@echo "LIBDIR=$(LIBDIR)"
	@echo "CPLLIBDIR=$(CPLLIBDIR)"
	@echo "CPLLIB_SRC_DIR=$(CPLLIB_SRC_DIR)"
	@echo "CPLRUNTIMEDIR=$(CPLRUNTIMEDIR)"
	@echo "BASH_COMPLETION_DIR=$(BASH_COMPLETION_DIR)"
	@echo "ZSH_COMPLETION_DIR=$(ZSH_COMPLETION_DIR)"
	@echo "FISH_COMPLETION_DIR=$(FISH_COMPLETION_DIR)"
	@echo "CPLLIB_SOURCES=$(CPLLIB_SOURCES)"
	@echo "CPLLIB_IMPLS=$(CPLLIB_IMPLS)"
	@echo "CPLLIB_ARCHIVE=$(CPLLIB_ARCHIVE)"
	@echo "CPPFLAGS=$(CPPFLAGS)"
	@echo "CFLAGS=$(CFLAGS)"
	@echo "LDFLAGS=$(LDFLAGS)"
	@echo "LDLIBS=$(LDLIBS)"
	@echo "ENABLE_Z3=$(ENABLE_Z3)"
	@echo "Z3_AVAILABLE=$(Z3_AVAILABLE)"
	@echo "Z3_ENABLED=$(Z3_ENABLED)"
	@echo "Z3_CFLAGS=$(Z3_CFLAGS)"
	@echo "Z3_LDLIBS=$(Z3_LDLIBS)"
	@echo "LOGS=$(LOGS)"
	@echo "INPUT=$(INPUT)"
	@echo "RUN_ARGS=$(RUN_ARGS)"
	@echo "VSCODE_DIR=$(VSCODE_DIR)"
	@echo "VSCODE_DOCKER_IMAGE=$(VSCODE_DOCKER_IMAGE)"
	@echo "VSCODE_OUTPUT_DIR=$(VSCODE_OUTPUT_DIR)"

help:
	@awk 'BEGIN {FS = ":.*## "; printf "Usage: make <target> [VAR=value]\n\nTargets:\n"} /^[a-zA-Z0-9_.-]+:.*## / {printf "  %-14s %s\n", $$1, $$2}' $(MAKEFILE_LIST)

.DELETE_ON_ERROR:
.PHONY: all check-cpllib-src check-vscode-src cpllib debug release install package run test unit-test rewrite-test std-test cli-test vscode-docker-build vscode-docker-package submodules clean clean-tests distclean print-sources print-config help

-include $(DEPS)
