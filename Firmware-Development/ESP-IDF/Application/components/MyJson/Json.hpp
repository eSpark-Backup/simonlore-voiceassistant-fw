#include "cJSON.h"
#include "esp_log.h"

class Json
{
    public:
        char* createJson(const char* key, const char* value, ...);
};
