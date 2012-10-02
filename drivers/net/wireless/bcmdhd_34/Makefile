# Prime
#    -DDHD_GPL -DDHD_SCHED -DCUSTOMER_HW2 -DNEW_COMPAT_WIRELESS -DWIFI_ACT_FRAME         \
#    -DGET_CUSTOM_MAC_ENABLE -DPKT_FILTER_SUPPORT -DENABLE_INSMOD_NO_FW_LOAD             \
#    -DPNO_SUPPORT -DWLBTAMP -DOOB_INTR_ONLY -DMMC_SDIO_ABORT -DHW_OOB -DWIFI_ACT_FRAME  \
#    -DSDTEST -DDHD_USE_EARLYSUSPEND -DCUSTOM_OOB_GPIO_NUM=2 -DWL_ENABLE_P2P_IF          \
#    -DWL_CFG80211_STA_EVENT


DHDCFLAGS = -fshort-wchar -Wall -Wstrict-prototypes -Werror -Dlinux     \
	-DBCMDRIVER -DBCMDONGLEHOST -DUNRELEASEDCHIP -DBCMDMA32 	\
        -DBCMFILEIMAGE -DDHDTHREAD -DDHD_DEBUG -DSDTEST -DBDC -DTOE     \
        -DDHD_BCMEVENTS -DSHOW_EVENTS -DPROP_TXSTATUS                   \
        -DBCMSDIO -DBCMLXSDMMC -DBCMPLATFORM_BUS -DARP_OFFLOAD_SUPPORT  \
        -DBCMDBG -DCUSTOMER_HW2 -DEMBEDDED_PLATFORM -DKEEP_ALIVE        \
        -DWLP2P -DMMC_SDIO_ABORT -DWIFI_ACT_FRAME			\
        -DGET_CUSTOM_MAC_ENABLE -DPKT_FILTER_SUPPORT                    \
        -DENABLE_INSMOD_NO_FW_LOAD -DPNO_SUPPORT -DWLBTAMP              \
        -DSET_RANDOM_MAC_SOFTAP                                         \
        -Idrivers/net/wireless/bcmdhd_34 -Idrivers/net/wireless/bcmdhd_34/include
        
ifeq ($(CONFIG_BCMDHD_WIFI_CONTROL_FUNC),y)
DHDCFLAGS += -DCONFIG_WIFI_CONTROL_FUNC
else
DHDCFLAGS += -DCUSTOM_OOB_GPIO_NUM=2
endif

# Use HW_OOB as default. HW_OOB is level-trigger
ifeq ($(CONFIG_BCMDHD_HW_OOB),y)
DHDCFLAGS += -DHW_OOB -DOOB_INTR_ONLY
else
DHDCFLAGS += -DSDIO_ISR_THREAD
endif

DHDOFILES = siutils.o sbutils.o aiutils.o hndpmu.o dhd_linux.o		\
        dhd_linux_sched.o linux_osl.o bcmutils.o bcmevent.o		\
        dhd_common.o dhd_custom_gpio.o dhd_cdc.o dhd_sdio.o		\
        bcmsdh_sdmmc.o bcmsdh.o bcmsdh_linux.o bcmsdh_sdmmc_linux.o     \
	wl_android.o wldev_common.o bcmwifi_channels.o dhd_bta.o

obj-$(CONFIG_BCM4334) += bcmdhd_34.o
bcmdhd_34-objs += $(DHDOFILES)
ifneq ($(CONFIG_BCMDHD_WEXT),)
bcmdhd_34-objs += wl_iw.o
DHDCFLAGS += -DSOFTAP -DUSE_IW
endif
ifneq ($(CONFIG_CFG80211),)
bcmdhd_34-objs += wl_cfg80211.o wl_cfgp2p.o wl_linux_mon.o dhd_cfg80211.o
DHDCFLAGS += -DWL_CFG80211 -DWL_CFG80211_STA_EVENT -DWL_ENABLE_P2P_IF

# WAPI support
#DHDCFLAGS += -DBCMWAPI_WPI -DBCMWAPI_WAI
endif
EXTRA_CFLAGS = $(DHDCFLAGS)
ifeq ($(CONFIG_BCM4334),m)
EXTRA_LDFLAGS += --strip-debug
endif
