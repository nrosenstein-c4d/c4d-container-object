VERSION=$(shell git describe --tags)

ifeq ($(OS),Windows_NT)
	PLATFORM=win
else
	PLATFORM=mac
endif

ifeq ($(RELEASE),)
  $(error RELEASE is not defined)
endif

.PHONY: dist
dist:
	mkdir -p dist
	tar -zcvf dist/c4d-container-object-$(VERSION)-r$(RELEASE)-$(PLATFORM).tar.gz \
		--exclude=*.lib --exclude=*.exp --exclude=*.ilk --exclude=*.pdb \
		--exclude=build --exclude=*.pyc \
		res CHANGELOG.md LICENSE.txt README.md \
		$(shell ls c4d-container-object.xdl64 c4d-container-object.xlib)
