#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "emu.h"
#include "log.h"
#include "emscripten_env.h"

const char *log_str[]={
	[LOG_SRC_UART]="uart",
	[LOG_SRC_CSR]="csr",
	[LOG_SRC_MBUS]="mbus",
	[LOG_SRC_MAPPER]="mapper",
	[LOG_SRC_SCSI]="scsi",
	[LOG_SRC_RAMROM]="ramrom",
	[LOG_SRC_RTC]="rtc",
	[LOG_SRC_EMU]="emu"
};

const char *level_str[]={
	"err", "warn", "notice", "info", "debug"
};

int  loglevel_for(const char *str) {
	for (int i=0; i<LOG_LVL_MAX; i++) {
		if (strcmp(str, level_str[i])==0) {
			return i;
		}
	}
	return -1;
}

//parses either e.g. 'notice' to set all srcs to that,
//or 'rtc=notice' to only set that source to the level.
//return 1 on error, 0 on ok
int parse_loglvl_str(char *str) {
	char *s=strchr(str, '=');
	if (!s) {
		int lvl=loglevel_for(str);
		if (lvl==-1) return 1;
		for (int i=0; i<LOG_SRC_MAX; i++) {
			log_set_level(i, lvl);
		}
		return 0;
	} else {
		int lvl=loglevel_for(s+1);
		if (lvl==-1) return 1;
		for (int i=0; i<LOG_SRC_MAX; i++) {
			if (strlen(log_str[i])==(s-str) && strncmp(str, log_str[i], s-str)==0) {
				log_set_level(i, lvl);
				return 0;
			}
		}
	}
	return 1;
}

int main(int argc, char **argv) {
	static_assert(sizeof(log_str)/sizeof(log_str[0])==LOG_SRC_MAX,
					"log_str array out of sync");
	static_assert(sizeof(level_str)/sizeof(level_str[0])==LOG_LVL_MAX,
					"level_str array out of sync");
	emu_cfg_t cfg={
#ifdef __EMSCRIPTEN__
		.u15_rom="U15-MERGED.BIN",
		.u17_rom="U17-MERGED.BIN",
		.cow_dir="persist/cow",
		.rtcram="persist/rtcram.bin",
		.realtime=1,
#else
		.u15_rom="../plexus-p20/ROMs/U15-MERGED.BIN",
		.u17_rom="../plexus-p20/ROMs/U17-MERGED.BIN",
		.rtcram="rtcram.bin",
#endif
		.hd0img="plexus-sanitized.img",
		.mem_size_bytes=2*1024*1024
	};
#ifdef __EMSCRIPTEN__
	emscripten_init();
#endif
	int error=0;
	for (int i=1; i<argc; i++) {
		if (strcmp(argv[i], "-u15")==0 && i+1<argc) {
			i++;
			cfg.u15_rom=argv[i];
		} else if (strcmp(argv[i], "-u17")==0 && i+1<argc) {
			i++;
			cfg.u15_rom=argv[i];
		} else if (strcmp(argv[i], "-r")==0) {
			cfg.realtime=1;
		} else if (strcmp(argv[i], "-l")==0 && i+1<argc) {
			i++;
			error|=parse_loglvl_str(argv[i]);
		} else if (strcmp(argv[i], "-c")==0 && i+1<argc) {
			i++;
			cfg.cow_dir=argv[i];
		} else if (strcmp(argv[i], "-m")==0 && i+1<argc) {
			i++;
			cfg.mem_size_bytes=atoi(argv[i])*1024*1024;
		} else {
			printf("Unknown argument %s\n", argv[i]);
			error=1;
			break;
		}
	}
	int m=cfg.mem_size_bytes/(1024*1024);
	if ((m & (m-1)) || m<1 || m>8) {
		printf("Memory needs to be 1, 2, 4 or 8MiB (%d given)\n", m);
		printf("Note 1 and 8 MB may not be supported by the OS\n");
		error=1;
	}
	if (error) {
		printf("Plexus-20 emulator\n");
		printf("Usage: %s [args]\n", argv[0]);
		printf(" -u15 Path to U15 rom file\n");
		printf(" -u17 Path to U17 rom file\n");
		printf(" -r Try to run at realtime speeds\n");
		printf(" -m n Set the amount of memory to n megabytes\n");
		printf(" -l module=level - set logging level of module to specified level\n");
		printf(" -l level - Set overal log level to specified level\n");
		printf("Modules: ");
		for (int i=0; i<LOG_SRC_MAX; i++) printf("%s ", log_str[i]);
		printf("\n");
		printf("Levels: ");
		for (int i=0; i<LOG_LVL_MAX; i++) printf("%s ", level_str[i]);
		printf("\n");
		exit(0);
	}
	emu_start(&cfg);
}
