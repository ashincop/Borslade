#include <drivers/screen/fb.h>
#include <stdarg.h>
#include <utils/logging/log.h>

void log(const char *host, int type, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char *string = "";
	switch (type) {
	case O_OKAY:
		string = "%q OK %Q";
		break;
	case O_BUG:
		string = "%r EDGE CASE %Q";
		break;
	case O_ERROR:
		string = "%U NERR %Q";
		break;
	case O_FATAL:
		string = "%U FERR %Q";
		break;
	case O_INFO:
		string = " INFO ";
		break;
	case O_ALL:
		string = "%r ALL %Q";
		break;
	default:
		string = "%r UNDOC %Q";
		break;
	}
	kprintf("[%s] (", host);
	kprintf(string);
	kprintf("): ");
	vkprintf(fmt, args);

	va_end(args);
}