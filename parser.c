
#include "conf.h"

int main(int argc, char **argv) {
	conf_filename="stdin";
	yyparse();
	printf("driver: %d\n", conf_driver_type);
	printf("device: %s\n", conf_device_name ? conf_device_name : "");
}
