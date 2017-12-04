# mruby is using Rake (http://rake.rubyforge.org) as a build tool.
# We provide a minimalistic version called minirake inside of our
# codebase.

RAKE = ruby ./minirake

all : settings
	$(RAKE)
.PHONY : all

test : all
	$(RAKE) test
.PHONY : test

settings:
	$(file > mruby_settings,MRUBY_LIBRARY_PATH = $(abspath ./build/host/lib) )

clean :
	$(RAKE) clean
	rm mruby_settings
.PHONY : clean
