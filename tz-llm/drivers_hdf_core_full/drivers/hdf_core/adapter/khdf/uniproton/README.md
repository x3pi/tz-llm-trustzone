# khdf\_uniproton

-   [Introduction](#section11660541593)
-   [Directory Structure](#section161941989596)
-   [Repositories Involved](#section1371113476307)

## Introduction

This repository stores the code and compilation scripts for the OpenHarmony driver subsystem to adapt to the uniproton kernel and to deploy the hardware driver foundation \(HDF\).

## Directory Structure

```
/drivers/hdf_core/adapter/khdf/uniproton
├── core                 # Driver code for adapting to the LiteOS Cortex-M kernel
├── osal                 # System APIs for adapting to the LiteOS Cortex-M kernel
├── platform             # Platform driver interfaces and adaptation code build
└── test                 # Test code for the kernel driver framework
```

## Repositories Involved

[Driver subsystem](https://gitee.com/openharmony/docs/blob/master/en/readme/driver.md)

[drivers\_framework](https://gitee.com/openharmony/drivers_framework/blob/master/README.md)

