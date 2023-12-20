#include "cJSON.h"
#include "esp_log.h"
#include <type_traits>
#include <cstddef>
#include <type_traits>
#include <cstring>
#include <utility>
#include "cJSON.h"
#include <typeinfo>
#include <iostream>

class Json
{
public:
    template <typename T, typename... Args>
    void createJsonHelper(cJSON *root, T orderedPair, Args... args)
    {
        const char *key = (orderedPair.first);
        auto value = (orderedPair.second);
        // if (std::is_same<value, const char *>::value)
        // {
        cJSON_AddStringToObject(root, key, value);
        // }
        // else if (std::is_same<value, double>::value)
        // {
        //     cJSON_AddNumberToObject(root, key, arg);
        // }
        // cJSON_AddStringToObject(root, key, value);
        if constexpr (sizeof...(Args) > 0)
        {
            createJsonHelper(root, args...);
        }
    }

    template <typename... Args>
    char *createJson(Args... args)
    {
        cJSON *root = cJSON_CreateObject();
        createJsonHelper(root, args...);

        char *jsonString = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        return jsonString;
    }
};