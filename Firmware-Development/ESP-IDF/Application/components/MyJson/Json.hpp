#include "cJSON.h"
#include "esp_log.h"

class Json
{
public:
    template <typename T>
    char *createJson(const char *key, T value, ...)
    {
        cJSON *root = cJSON_CreateObject();
        if (is_same<T, const char *>::value)
        {
            cJSON_AddStringToObject(root, key, value);
        }
        else if (is_same<T, double>::value)
        {
            cJSON_AddNumberToObject(root, key, value);
        }

        va_list args;
        va_start(args, value);

        T nextKey = va_arg(args,T);
        while (nextKey != NULL)
        {

            T nextValue = va_arg(args, T);
            if (is_same<T, const char *>::value)
            {
                cJSON_AddStringToObject(root, nextKey, nextValue);
            }
            else if (is_same<T, double>::value)
            {
                cJSON_AddNumberToObject(root, nextKey, nextValue);
            }
            nextKey = va_arg(args, T);
        }

        va_end(args);

        char *jsonString = cJSON_PrintUnformatted(root);

        cJSON_Delete(root);

        return jsonString;
    }
};
