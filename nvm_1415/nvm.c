#define RELEASE_WEBOS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <dlfcn.h>
//#include "release/release_misc.h"
//#include "release/release_types.h"
//#include "release/nvm.h"
#include "helpers/lh_base.h"
#include "lh_module.h"

#include <sys/stat.h>

typedef int DTV_STATUS_T;
typedef uint32_t __UINT32;
typedef uint8_t __UINT8;

void hook_exit(lh_r_process_t * proc) {
	printf("hook_main finished\n");
}

static DTV_STATUS_T (*API_NVM_Read)(__UINT32 offset, __UINT32 nData, __UINT8 *pRxBuf) = NULL;
static DTV_STATUS_T (*API_NVM_Write)(__UINT32 offset, __UINT32 nData, __UINT8 *pTxBuf) = NULL;

void restoreNvmBackup(const char *nvmPath){
	int fd = open(nvmPath, O_RDONLY);
	if(fd <= 0){
		lh_printf("Failed to open %s for reading\n", nvmPath);
		return;
	}

	struct stat statBuf;
	if(fstat(fd, &statBuf) < 0){
		lh_printf("Failed to stat file\n");
		close(fd);
		return;
	}

	size_t nvmSize = statBuf.st_size;

	uint8_t *nvmBuf = calloc(1, nvmSize);
	ssize_t numRead = read(fd, nvmBuf, nvmSize);
	if(numRead != nvmSize){
		lh_printf("read failed (read %zu, expected %zu)\n", numRead, nvmSize);
		return;
	}

	lh_printf("Writing nvm (%zu bytes)\n", nvmSize);
	for(uint32_t i=0; i<(uint32_t)nvmSize; i++){
		API_NVM_Write(i, 1, &nvmBuf[i]);
	}

	lh_printf("DONE!\n");

	free(nvmBuf);
	close(fd);
}

#define TOOL_OPTS_REGION 0x4E4
void restoreToolOptions(const char *nvmPath){
	FILE *nvm = fopen(nvmPath, "rb");
	if(!nvm){
		lh_printf("Error: cannot open %s for reading\n", nvmPath);
		return;
	}

	uint8_t buf[0x4A];
	memset(buf, 0x0, sizeof(buf));

	fseek(nvm, TOOL_OPTS_REGION, SEEK_SET);
	fread(buf, 1, sizeof(buf), nvm);
	fclose(nvm);

	lh_printf("Writing %d bytes...\n", sizeof(buf));
	API_NVM_Write(TOOL_OPTS_REGION, sizeof(buf), buf);
	lh_printf("DONE\n");
}

void dumpNvram(const char *destPath){
	FILE *nvram = fopen(destPath, "wb");
	if(!nvram){
		lh_printf("Error! cannot open/create /tmp/nvram.bin for writing\n");
		return;
	}

	uint8_t byte;
	uint32_t offset = 0;
	int result;
	while(true){
		result = API_NVM_Read(offset++, 1, &byte);
		if(result != 0) break;
		fputc(byte, nvram);
	}
	lh_printf("Dump finished! dumped %d (0x%x) bytes\n", offset, offset);

	fflush(nvram);
	fclose(nvram);
}

int hook_main(int argc, char **argv) {
	lh_get_stdout(argv[1]); //redirect stdio and stderr

	lh_printf("HELLO WORLD!\n");


	API_NVM_Read = dlsym(RTLD_DEFAULT, "DIL_NVM_Read");
	API_NVM_Write = dlsym(RTLD_DEFAULT, "DIL_NVM_Write");

	if(API_NVM_Read == 0 || API_NVM_Write == 0){
		lh_printf("ERROR: Cannot locate NVM function!\n");
		return LH_SUCCESS;
	}
	lh_printf("NVM_Read: 0x%x\n", API_NVM_Read);
	lh_printf("NVM_Write: 0x%x\n", API_NVM_Write);

	if(argc > 3){
		char *nvmPath;
		switch(argv[2][0]){
			case 'b': //backup
				lh_printf("Dumping nvram to /tmp/nvram.bin\n");
				dumpNvram("/tmp/nvram.bin");
				break;
			case 'r':; //restore
				nvmPath = argv[3];
				restoreNvmBackup(argv[3]);
				break;
			case 't':; //tool-opts
				nvmPath = argv[3];
				lh_printf("NVM file path: %s\n", nvmPath);
				restoreToolOptions(nvmPath);
				break;
			default:
				lh_printf("Unrecognized command %c\n", argv[2][0]);
				return LH_SUCCESS;
		}
	}

	lh_printf("All done! have a good day\n");
	return LH_SUCCESS;
}

lh_hook_t hook_settings = {
	.version = 1,
	.autoinit_pre = hook_main,
	.autoinit_post = hook_exit,
	.fn_hooks =
	{
		{
			.hook_kind = LHM_FN_HOOK_TRAILING
		}
	}
};
