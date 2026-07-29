cmd_arch/arm64/kvm/hyp/nvhe/hyp-reloc.o := aarch64-linux-gnu-gcc -Wp,-MMD,arch/arm64/kvm/hyp/nvhe/.hyp-reloc.o.d -nostdinc -isystem /usr/lib/gcc-cross/aarch64-linux-gnu/13/include -I./arch/arm64/include -I./arch/arm64/include/generated  -I./include -I./arch/arm64/include/uapi -I./arch/arm64/include/generated/uapi -I./include/uapi -I./include/generated/uapi -include ./include/linux/kconfig.h -D__KERNEL__ -mlittle-endian -DCC_USING_PATCHABLE_FUNCTION_ENTRY -DKASAN_SHADOW_SCALE_SHIFT= -fmacro-prefix-map=./= -D__ASSEMBLY__ -fno-PIE -mabi=lp64 -fno-asynchronous-unwind-tables -fno-unwind-tables -DKASAN_SHADOW_SCALE_SHIFT= -I./arch/arm64/kvm/hyp/include -D__KVM_NVHE_HYPERVISOR__ -D__DISABLE_EXPORTS    -c -o arch/arm64/kvm/hyp/nvhe/hyp-reloc.o arch/arm64/kvm/hyp/nvhe/hyp-reloc.S

source_arch/arm64/kvm/hyp/nvhe/hyp-reloc.o := arch/arm64/kvm/hyp/nvhe/hyp-reloc.S

deps_arch/arm64/kvm/hyp/nvhe/hyp-reloc.o := \
  include/linux/kconfig.h \
    $(wildcard include/config/cc/version/text.h) \
    $(wildcard include/config/cpu/big/endian.h) \
    $(wildcard include/config/booger.h) \
    $(wildcard include/config/foo.h) \

arch/arm64/kvm/hyp/nvhe/hyp-reloc.o: $(deps_arch/arm64/kvm/hyp/nvhe/hyp-reloc.o)

$(deps_arch/arm64/kvm/hyp/nvhe/hyp-reloc.o):
