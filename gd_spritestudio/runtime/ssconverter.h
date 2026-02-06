#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

enum class CConverterError {
  Success = 0,
  ErrorIo = -1,
  ErrorSspj = -2,
  ErrorConversion = -3,
  ErrorNotYetStarted = -4,
  ErrorUnknown = -99,
};

struct Context;

using LogCallback = void(*)(const char*);

extern "C" {

const char *ss_converter_version();

void ss_converter_version_free(char *s);

Context *ss_converter_create();

void ss_converter_destroy(Context *context);

void ss_converter_convert(Context *ctx,
                          const char *sspj,
                          const char *output_dir,
                          LogCallback callback);

bool ss_converter_is_finished(Context *context);

CConverterError ss_converter_get_result(Context *context);

void ss_converter_get_error(Context *context, char **out_data, uintptr_t *out_len);

}  // extern "C"
