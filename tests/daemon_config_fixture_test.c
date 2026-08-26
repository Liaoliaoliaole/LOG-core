/* Shared Web/Core daemon-config validation corpus. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cjson/cJSON.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "../src/Supplementary/Morfeas_XML.h"

static int g_checks = 0;
static int g_failures = 0;

#define CHECK(cond, fmt, ...) \
    do { \
        g_checks++; \
        if (cond) printf("PASS: " fmt "\n", ##__VA_ARGS__); \
        else { g_failures++; fprintf(stderr, "FAIL: " fmt "\n", ##__VA_ARGS__); } \
    } while (0)

static char *read_file(const char *path)
{
    FILE *fp = fopen(path, "rb");
    long length;
    char *contents;
    if (!fp) return NULL;
    if (fseek(fp, 0, SEEK_END) != 0 || (length = ftell(fp)) < 0 || fseek(fp, 0, SEEK_SET) != 0) {
        fclose(fp);
        return NULL;
    }
    contents = calloc((size_t)length + 1, 1);
    if (!contents || fread(contents, 1, (size_t)length, fp) != (size_t)length) {
        free(contents);
        fclose(fp);
        return NULL;
    }
    fclose(fp);
    return contents;
}

int main(int argc, char **argv)
{
    const char *fixture_path = argc == 2 ? argv[1] : "tests/fixtures/daemon_config_validation_cases.json";
    char *raw = read_file(fixture_path);
    cJSON *cases = raw ? cJSON_Parse(raw) : NULL;
    int index;

    CHECK(cases != NULL && cJSON_IsArray(cases), "shared daemon-config fixture corpus parses");
    if (!cases || !cJSON_IsArray(cases)) {
        cJSON_Delete(cases);
        free(raw);
        return EXIT_FAILURE;
    }
    CHECK(cJSON_GetArraySize(cases) > 0, "shared daemon-config fixture corpus is non-empty");

    for (index = 0; index < cJSON_GetArraySize(cases); index++) {
        cJSON *entry = cJSON_GetArrayItem(cases, index);
        cJSON *name = cJSON_GetObjectItemCaseSensitive(entry, "name");
        cJSON *xml = cJSON_GetObjectItemCaseSensitive(entry, "xml");
        cJSON *accepted = cJSON_GetObjectItemCaseSensitive(entry, "accepted");
        const char *label = cJSON_IsString(name) ? name->valuestring : "unnamed";
        xmlDocPtr doc = cJSON_IsString(xml)
            ? xmlReadMemory(xml->valuestring, (int)strlen(xml->valuestring), label, NULL, XML_PARSE_NOBLANKS)
            : NULL;
        int actual = doc && Morfeas_daemon_config_valid(xmlDocGetRootElement(doc)) == EXIT_SUCCESS;
        int expected = cJSON_IsTrue(accepted);
        CHECK(doc != NULL, "%s fixture parses as XML", label);
        CHECK(actual == expected, "%s: Core validator result is %s", label, expected ? "accepted" : "rejected");
        if (doc) xmlFreeDoc(doc);
    }

    cJSON_Delete(cases);
    free(raw);
    printf("\n%d checks, %d passed, %d failed\n", g_checks, g_checks - g_failures, g_failures);
    return g_failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
