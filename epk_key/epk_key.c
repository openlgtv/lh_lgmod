#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libgen.h>
#include <fcntl.h>
#include <dlfcn.h>
#include "helpers/lh_base.h"
#include "lh_module.h"

void hook_exit(lh_r_process_t * proc) {
	printf("hook_main finished\n");
}

int hook_main(int argc, char **argv) {
	lh_r_process_t *rproc = lh_get_procinfo(argc, argv);
	lh_get_stdout(rproc->ttyName);

	lh_printf("argc: %d\n", argc);

	FILE *(*DILE_CRYPTO_SFU_Initialize)(const char *) = dlsym(RTLD_DEFAULT, "DILE_CRYPTO_SFU_Initialize");
	int (*DILE_CRYPTO_SFU_GetAESKey)(uint8_t *) = dlsym(RTLD_DEFAULT, "DILE_CRYPTO_SFU_GetAESKey");

	FILE *hPub = DILE_CRYPTO_SFU_Initialize("/mnt/lg/lgapp/netflix_pub.pem");
	puts("survived init");

	uint8_t buf[17];
	memset(buf, 0x00, sizeof(buf));
	DILE_CRYPTO_SFU_GetAESKey((uint8_t *)&buf);

	lh_printf("survived get\n");
	for(int i=0; i<16; i++){
		lh_printf("%02X,", buf[i]);
	}
	lh_printf("\n");


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
