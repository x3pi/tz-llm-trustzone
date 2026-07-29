if [ "$setup" = "base" ]; then
    echo 10 > /proc/irq/29/smp_affinity
    echo 20 > /proc/irq/30/smp_affinity
    echo 40 > /proc/irq/31/smp_affinity
else
    echo 10 > /proc/irq/29/smp_affinity
    echo 20 > /proc/irq/30/smp_affinity
    echo 40 > /proc/irq/31/smp_affinity
fi
