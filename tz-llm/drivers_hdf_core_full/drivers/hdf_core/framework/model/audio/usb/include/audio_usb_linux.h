/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 *
 * HDF is dual licensed: you can use it either under the terms of
 * the GPL, or the BSD license, at your option.
 * See the LICENSE file in the root of this repository for complete details.
 */

#ifndef AUDIO_USB_LINUX_H
#define AUDIO_USB_LINUX_H

#include <linux/usb.h>

#include "audio_platform_if.h"
#include "hdf_dlist.h"

struct AudioUsbEndpoint;

#define MAX_PACKS    6               /* per URB */
#define MAX_PACKS_HS (MAX_PACKS * 8) /* in high speed mode, 8 is max packet length */
#define MAX_URBS     12

#define AUDIO_USB_ENDPOINT_TYPE_DATA 0
#define AUDIO_USB_ENDPOINT_TYPE_SYNC 1

struct AudioUsbUrbCtx {
    struct urb *urb;
    uint32_t bufferSize; /* size of data buffer, if data URB */
    struct AudioUsbEndpoint *endpoint;
    int32_t index;                    /* index for urb array */
    int32_t packets;                  /* number of packets per urb */
    int32_t packetSize[MAX_PACKS_HS]; /* size of packets for next submission */
    struct list_head readyList;
};

struct AudioUsbEndpoint {
    struct AudioUsbDriver *audioUsbDriver;

    int32_t useCount;
    int32_t endpointNum; /* the referenced endpoint number */
    int32_t type;        /* endpoint type */
    unsigned long flags;

    void (*AudioPrepareDataUrb)(struct AudioUsbDriver *audioUsbDriver, struct urb *urb);
    void (*AudioRetireDataUrb)(struct AudioUsbDriver *audioUsbDriver, struct urb *urb);

    struct AudioUsbEndpoint *syncMasterEndpoint;
    struct AudioUsbEndpoint *syncSlaveEndpoint;

    struct AudioUsbUrbCtx urbContext[MAX_URBS];

    struct AudioUsbPacketInfo {
        uint32_t packetSize[MAX_PACKS_HS];
        int32_t packets;
    } nextPacket[MAX_URBS];
    int32_t nextPacketReadPos;
    int32_t nextPacketWritePos;
    struct list_head readyPlaybackUrbs;

    uint32_t nurbs;           /* # urbs */
    unsigned long activeMask; /* bitmask of active urbs */
    unsigned long unlinkMask; /* bitmask of unlinked urbs */
    char *syncbuf;            /* sync buffer for all sync URBs */
    dma_addr_t syncDma;       /* DMA address of syncbuf */

    uint32_t pipe;             /* the data i/o pipe */
    uint32_t packsize[2];      /* small/large packet sizes in samples */
    uint32_t sampleRem;        /* remainder from division fs/pps */
    uint32_t sampleAccum;      /* sample accumulator */
    uint32_t pps;              /* packets per second */
    uint32_t freqn;            /* nominal sampling rate in fs/fps in Q16.16 format */
    uint32_t freqm;            /* momentary sampling rate in fs/fps in Q16.16 format */
    int32_t freqShift;         /* how much to shift the feedback value to get Q16.16 */
    uint32_t freqMax;          /* maximum sampling rate, used for buffer management */
    uint32_t phase;            /* phase accumulator */
    uint32_t maxPackSize;      /* max packet size in bytes */
    uint32_t maxFrameSize;     /* max packet size in frames */
    uint32_t maxUrbFrames;     /* max URB size in frames */
    uint32_t curPackSize;      /* current packet size in bytes (for capture) */
    uint32_t curframesize;     /* current packet size in frames (for capture) */
    uint32_t syncMaxSize;      /* sync endpoint packet size */
    uint32_t fillMax : 1;      /* fill max packet size always */
    uint32_t tenorFbQuirk : 1; /* corrupted feedback data */
    uint32_t dataInterval;     /* log_2 of data packet interval */
    uint32_t syncInterval;     /* P for adaptive mode, 0 otherwise */
    uint8_t silenceValue;
    uint32_t stride;
    int32_t iface, altsetting;
    int32_t skipPackets;     /* quirks for devices to ignore the first n packets in a stream */
    bool isImplicitFeedback; /* This endpoint is used as implicit feedback */

    spinlock_t lock;
    struct DListHead list; /* list of endpoint */
};

struct AudioUsbFormat {
    struct DListHead list;
    uint64_t formats;          /* format bits */
    uint32_t channels;         /* # channels */
    uint32_t fmtType;          /* USB audio format type (1-3) */
    uint32_t fmtBits;          /* number of significant bits */
    uint32_t frameSize;        /* samples per frame for non-audio */
    int32_t iface;             /* interface number */
    uint8_t altsetting;        /* corresponding alternate setting */
    uint8_t altsetIdx;         /* array index of altenate setting */
    int32_t attributes;        /* corresponding attributes of cs endpoint */
    uint8_t endpoint;          /* endpoint */
    uint8_t epAttr;            /* endpoint attributes */
    uint8_t dataInterval;      /* log_2 of data packet interval */
    uint8_t protocol;          /* UAC_VERSION_1/2/3 */
    uint32_t maxPackSize;      /* max. packet size */
    uint32_t rates;            /* rate bitmasks */
    uint32_t rateMin, rateMax; /* min/max rates */
    uint32_t nrRates;          /* number of rate table entries */
    uint32_t *rateTable;       /* rate table */
    uint8_t clock;             /* associated clock */
    bool dsdDop;               /* add DOP headers in case of DSD samples */
    bool dsdBitRev;            /* reverse the bits of each DSD sample */
    bool dsdRaw;               /* altsetting is raw DSD */
};

struct AudioUsbDriver {
    struct usb_device *dev;
    struct usb_interface *usbIf;
    const struct usb_device_id *usbDevId;
    struct AudioCard *audioCard;
    struct AudioUsbFormat *renderUsbFormat;
    struct AudioUsbFormat *captureUsbFormat;

    struct usb_host_interface *ctrlIntf;
    struct CircleBufInfo renderBufInfo;  /**< Render pcm stream transfer */
    struct CircleBufInfo captureBufInfo; /**< Capture pcm stream transfer */
    struct PcmInfo renderPcmInfo;        /**< Render pcm stream info */
    struct PcmInfo capturePcmInfo;       /**< Capture pcm stream info */
    int32_t ifNum;
    uint32_t usbId;
    void *priv;
    int8_t pnpFlag;
    atomic_t active;
    atomic_t shutdown;
    atomic_t usageCount;
    wait_queue_head_t shutdownWait;
    int32_t sampleRateReadError;
    bool needSetupEp;
    int32_t setup;

    unsigned long renderFlags;
    unsigned long captureFlags;

    uint32_t fmtType;

    uint32_t renderTransferDone;
    uint32_t captureTransferDone;
    uint32_t renderHwptr;
    uint32_t captureHwptr;

    uint32_t frameLimit;
    bool triggerTstampPendingUpdate;
    uint32_t running;

    uint32_t epNum; /* the endpoint number */

    struct AudioUsbEndpoint *renderDataEndpoint;
    struct AudioUsbEndpoint *renderSyncEndpoint;
    struct AudioUsbEndpoint *captureDataEndpoint;
    struct AudioUsbEndpoint *captureSyncEndpoint;

    spinlock_t lock;

    struct DListHead endpointList; /* list of endpoint */

    struct DListHead renderUsbFormatList;  /* list of audioUsbFormat */
    struct DListHead captureUsbFormatList; /* list of audioUsbFormat */

    struct mutex mutex;
};

uint32_t AudioUsbGetUsbId(uint32_t vendor, uint32_t product);
struct AudioUsbDriver *GetLinuxAudioUsb(void);

#endif