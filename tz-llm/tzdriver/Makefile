ifeq ($(CONFIG_TZDRIVER),y)
KERNEL_DIR := $(srctree)

EXTRA_CFLAGS += -I$(KERNEL_DIR)/../../../../third_party/bounds_checking_function/include/
SEC_LIB_SOURCES = memcpy_s.c memmove_s.c memset_s.c securecutil.c secureinput_a.c secureprintoutput_a.c snprintf_s.c \
                  sprintf_s.c strcat_s.c strcpy_s.c strncat_s.c strncpy_s.c strtok_s.c  vsnprintf_s.c vsprintf_s.c
SEC_FUNCTION_OBJECTS := $(patsubst %.c,%.o,$(SEC_LIB_SOURCES))
SEC_FUNCTION_OBJECTS := $(addprefix $(KERNEL_DIR)/../../../../third_party/bounds_checking_function/src/,${SEC_FUNCTION_OBJECTS})


obj-$(CONFIG_TZDRIVER) += agent_rpmb/
obj-$(CONFIG_TZDRIVER) += auth/
obj-$(CONFIG_TZDRIVER) += core/
obj-$(CONFIG_TZDRIVER) += tlogger/
obj-$(CONFIG_TZDRIVER) += ion/
obj-$(CONFIG_TZDRIVER) += tui/
obj-$(CONFIG_TZDRIVER) += whitelist/
#obj-$(CONFIG_TZDRIVER) += $(SEC_FUNCTION_OBJECTS)

endif
