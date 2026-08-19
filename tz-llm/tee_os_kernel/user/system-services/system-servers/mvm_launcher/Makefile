# mvm_launcher: metanode's own, fully independent /mvm_ta launcher.
# Deliberately NOT part of chanmgr -- see main.c's own top-of-file comment.

CFLAGS := $(DEFAULT_USER_CFLAGS)
LDFLAGS := $(DEFAULT_DYNAMIC_USER_LDFLAGS) -pie
SRCS := $(wildcard *.c)
OBJS := $(SRCS:%=%.o)
DEPS := $(OBJS:.o=.d)

mvm_launcher.srv: $(OBJS)
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

clean:
	$(Q)rm -f $(OBJS) $(DEPS) mvm_launcher.srv

-include $(DEPS)

$(OBJS): %.o: %
	$(Q)$(CC) $(CFLAGS) -c $< -o $@ -MMD
