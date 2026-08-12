# Orange Pi 5 Max: fdab0000.npu is on IRQ 27/28/29 (not 29/30/31 as on 5 Plus).
# Pin them to CPU 4/5/6 so the Secure World cores can take them.
echo 10 > /proc/irq/27/smp_affinity
echo 20 > /proc/irq/28/smp_affinity
echo 40 > /proc/irq/29/smp_affinity
