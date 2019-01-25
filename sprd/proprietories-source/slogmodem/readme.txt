Configuration options for slogmodem:

1. Make file variables.

The following make file variables can be defined in project make files.

CP_LOG_DIR_IN_AP
    Specifies the directory under which to save CP log. Supported locations
    are:
        slog
            the modem_log directory will locate under the slog directory.
        ylog
            the modem_log directory will locate under the ylog directory.
        (Not defined)
            the modem_log directory will locate under the root directory
            of the /data partition or the SD card.

SPRD_CP_LOG_WCN
    The WCN type. Supported types:
        NONE
            No WCN.
        TSHARK
            WCN integrated in TShark.
        MARLIN
            MARLIN
        MARLIN2
            MARLIN 2

2. Macros

The following macros can be defined in Android.mk.

EXT_STORAGE_PATH
    The external storage path. If this macro is not defined, slogmodem will
    get the external storage path from /proc/mounts.

3. Command line arguments

slogmodem command line syntax:
    slogmodem [–-test-ic <initial_conf>] [--test-conf <working_conf>]

    –-test-ic <initial_conf>
        Specify test mode initial configuration file path.
        /system/etc/test_mode.conf is used in test mode (calibration ors
        autotest)
        /system/etc/slog_mode.conf is used by default.

    --test-conf <working_conf>
        Specify working configuration file in test mode (calibration mode or
        autotest mode). <working conf> shall only include the file name.
