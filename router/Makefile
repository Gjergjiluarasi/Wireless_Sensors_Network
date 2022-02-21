CONTIKI_PROJECT = router
all: $(CONTIKI_PROJECT)
#UIP_CONF_IPV6=1

PROJECT_SOURCEFILES += buffer.c
PROJECT_SOURCEFILES += helpers.c

CONTIKI_WITH_RIME = 1
CONTIKI = $(HOME)/contiki

CFLAGS += -DPROJECT_CONF_H=\"project-conf.h\"

include $(CONTIKI)/Makefile.include
	
