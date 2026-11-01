#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    EXIT_CODE_SUCCESS = 0,
    EXIT_CODE_INVALID_ARGS = 1,
    EXIT_CODE_SOURCE_OPEN_FAIL = 2,
    EXIT_CODE_OUTPUT_OPEN_FAIL = 3,
    EXIT_CODE_SOURCE_READ_FAIL = 4,
    EXIT_CODE_SOURCE_EMPTY = 5
} exit_codes_t;

int is_whitespace(char c) {
    if (c == ' ') return 1;
    if (c == '\t') return 1;
    return 0;
}

int starts_with(const char* text, const char* prefix) {
    size_t i = 0;
    while (1) {
        if (prefix[i] == 0) return 1;
        if (text[i] != prefix[i]) return 0;
        ++i;
    }
}

int main(int argc, char** argv) {
    int append = 0;

    if (argc == 3) {
        printf("usage: flandocs <file> <output> [--append]\n");
        append = 0;
    }
    else if (argc == 4) {
        if (!starts_with(argv[3], "--append")) {
            printf("usage: flandocs <file> <output> [--append]\n");
            return EXIT_CODE_INVALID_ARGS;
        }
        append = 1;
    }
    else {
        printf("usage: flandocs <file> <output> [--append]\n");
    }

    FILE* source = fopen(argv[1], "rb");
    FILE* output = NULL;

    if (!source) {
        printf("Failed to open file \"%s\"\n", argv[1]);
        return EXIT_CODE_SOURCE_OPEN_FAIL;
    }\

    // fetch file size
    fseek(source, 0, SEEK_END);
    int end = ftell(source);
    fseek(source, 0, SEEK_SET);
    int start = ftell(source);
    int size = end - start;

    if (size == 0) {
        printf("Source file was empty: \"%s\"\n", argv[1]);
        return EXIT_CODE_SOURCE_EMPTY;
    }

    // read file
    char* data = malloc(size);
    fread(data, size, 1, source);

    char* data_curr = data;

    char func[8192] = {0};
    size_t func_i = 0;

    char desc[8192] = {0};
    size_t desc_i = 0;

    char param_name[32][256] = {0};
    char param_type[32][256] = {0};
    char param_desc[32][512] = {0};
    size_t param_i = 0;

    int prev_was_doc_line = 0;
    int printed_source_name = 0;

    while (1) {
        int is_doc_line = 0;

        // we are at the start of a new line, which may contain whitespace - filter that out
        while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
            ++data_curr;
        }

        // triple slash
        if (*data_curr != '/') goto next_line;
        int consecutive_slashes = 0;
        while(*data_curr == '/') {
            ++consecutive_slashes;
            ++data_curr;
        }
        if (consecutive_slashes != 3) goto next_line;
        while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
            ++data_curr;
        }

        // "doc: "
        if (!starts_with(data_curr, "doc:")) goto next_line;
        while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
            ++data_curr;
        }
        data_curr += sizeof("doc:");

        is_doc_line = 1;

        // detect line type
        if (starts_with(data_curr, "desc:")) {
            if (desc_i != 0) {
                desc[desc_i++] = '\n';
                desc[desc_i++] = '\n';
            }
            data_curr += sizeof("desc:") - 1; // exclude null
            while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
                ++data_curr;
            }
            while (*data_curr != '\n') {
                desc[desc_i++] = *data_curr++;
            }
        }
        else if (starts_with(data_curr, "param(")) {
            const size_t curr_param_i = param_i++;
            size_t param_name_cursor = 0;
            size_t param_type_cursor = 0;
            size_t param_desc_cursor = 0;

            // param type
            data_curr += sizeof("param(") - 1; // exclude null
            while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
                ++data_curr;
            }
            while (!is_whitespace(*data_curr)) {
                if (*data_curr == ')') goto next_line;
                param_type[curr_param_i][param_type_cursor++] = *data_curr++;
            }
            while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
                ++data_curr;
            }
            while (*data_curr != ')') {
                param_name[curr_param_i][param_name_cursor++] = *data_curr++;
            }

            // param desc
            data_curr += 2;
            while (is_whitespace(*data_curr) && ((data_curr - data) < size)) {
                ++data_curr;
            }
            while (*data_curr != '\n') {
                param_desc[curr_param_i][param_desc_cursor++] = *data_curr++;
            }
        }

        next_line:

        if (!is_doc_line && prev_was_doc_line) {
            while (*data_curr != '{' && *data_curr != '\n') {
                func[func_i++] = *data_curr++;
            }

            // markdown yippeeee
            if (param_i || desc_i) {
                if (!output) {
                    output = fopen(argv[2], append ? "ab" : "wb");

                    if (!output) {
                        printf("Failed to open file \"%s\"\n", argv[2]);
                        return EXIT_CODE_OUTPUT_OPEN_FAIL;
                    }
                }
                if (!printed_source_name) {
                    fprintf(output, "# %s\n\n", argv[1]);
                    fprintf(output, "---\n");
                    printed_source_name = 1;
                }
                fprintf(output, "```c\n%s\n```\n\n", func);
                fprintf(output, "%s\n\n", desc);
                if (param_i) {
                    fprintf(output, "|Parameter|Type|Description|\n|-|-|-|\n");
                    for (size_t i = 0; i < param_i; ++i) {
                        fprintf(output, "|%s|%s|%s|\n", param_name[i], param_type[i], param_desc[i]);
                    }
                }
                fprintf(output, "---\n");
            }

            // clear all the buffers
            memset(param_name, 0, sizeof(param_name));
            memset(param_type, 0, sizeof(param_type));
            memset(param_desc, 0, sizeof(param_desc));
            param_i = 0;

            memset(func, 0, sizeof(func));
            func_i = 0;

            memset(desc, 0, sizeof(desc));
            desc_i = 0;
        }

        while (*data_curr++ != '\n') {
            if ((data_curr - data) >= size) goto end;
        }
        prev_was_doc_line = is_doc_line;
    }

    end:

    if (output) printf("Source file \"%s\" documented to \"%s\"\n", argv[1], argv[2]);
    return EXIT_CODE_SUCCESS;
}

/// doc: desc: This is a test comment to make sure parsing works as intended.
/// doc: desc: Documentation should be capable of multiple lines for the same doc.
/// doc: desc: Triple slash to start it, oh and it should take the function name right below it and parse it using
/// doc: param(int i): Example parameter to test adding comments to parameters
/// doc: param(int x): Another example parameter
void test(int i, int x) {
    return;
}
