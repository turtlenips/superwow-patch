// SuperWoW Heal Text Disabler
// https://github.com/turtlenips/superwow-patch

#include <assert.h>
#include <stdbool.h>
#include <windows.h>

#define EXPECTED_FILE_SIZE 129024

#define BOUNDS_CHECK(OFFSET, SIZE) \
    (0 * sizeof(struct { _Static_assert(((OFFSET) + (SIZE) <= EXPECTED_FILE_SIZE), "bad patch"); char c; }))

#define PATCH(OFFSET, SIZE, OLD_DATA, NEW_DATA) { \
        .offset = (OFFSET) + BOUNDS_CHECK((OFFSET),(SIZE)), \
        .size = (SIZE), \
        .data = { \
            [PATCH_VERSION_OLD] = (const unsigned char[(SIZE)]){ OLD_DATA }, \
            [PATCH_VERSION_NEW] = (const unsigned char[(SIZE)]){ NEW_DATA }, \
        } \
    }

typedef enum { 
    PATCH_VERSION_OLD, 
    PATCH_VERSION_NEW, 
    PATCH_VERSION_UNKNOWN 
} patch_version_t;

typedef struct {
    size_t offset;
    size_t size;
    const unsigned char* data[2];
} patch_t;

static const patch_t patches[] = { 
    PATCH(
        0x00004F28, 15,
        "\x68\xF0\x3B\x00\x10\x68\x94\xD9\x01\x10\xE8\x39\x25\x00\x00",
        "\xEB\x0D\x7E\x49\x4C\x49\x4B\x45\x54\x55\x52\x54\x4C\x45\x53"
    ),
    PATCH(
        0x00019F79, 22,
        "SuperWoW 1.5 by Balake",
        "SuperWoW+1.5 by Balake"
    ),
    PATCH(
        0x0001E054, 4,
        "\x29\x3B\x2E\x3B",
        "\x00\x00\x00\x00"
    ),
};

static const size_t patches_count = sizeof(patches) / sizeof(patches[0]);

static const WCHAR WINDOW_CAPTION[] =
    L"SuperWoW Heal Text Disabler";

static const WCHAR HELP_MSG[] =
    L"This patch disables SuperWoW's floating healing text.\n"
    L"\n"
    L"To apply the patch, drag and drop SuperWoWhook.dll onto this program's .EXE file.\n"
    L"\n"
    L"SuperWoW version 1.5.1 is required. Get it at:\n"
    L"https://github.com/balakethelock/SuperWoW\n"
    L"\n"
    L"Patch by Sugarnips+Gretzky @ Nordanaar :)\n"
    L"https://github.com/turtlenips/superwow-patch";

static const WCHAR INVALID_DATA_MSG[] =
    L"Input file is not a valid SuperWoWhook.dll.\n"
    L"Make sure you are using version 1.5.1.";
    
static const WCHAR PATCH_APPLIED_MSG[] =
    L"Patch Applied";

static const WCHAR PATCH_REMOVED_MSG[] =
    L"Patch Removed";

static void apply_patch (unsigned char* data, size_t size, patch_version_t patch_version) {
    for (const patch_t* p = patches; p < patches + patches_count; ++p) {
        unsigned char* dst = data + p->offset;
        const unsigned char* end = dst + p->size;
        const unsigned char* src = p->data[patch_version];
        while (dst != end) {
            *dst++ = *src++;
        }
    }
}

static bool check_patch (const unsigned char* data, size_t size, patch_version_t patch_version) {
    for (const patch_t* p = patches; p < patches + patches_count; ++p) {
        const unsigned char* dst = data + p->offset;
        const unsigned char* end = dst + p->size;
        const unsigned char* src = p->data[patch_version];
        while (dst != end) {
            if (*dst++ != *src++) return false;
        }
    }
    return true;
}

static patch_version_t check_file_version (const unsigned char* data, size_t size) {
	if (size != EXPECTED_FILE_SIZE) return PATCH_VERSION_UNKNOWN;
    if (check_patch(data, size, PATCH_VERSION_OLD)) return PATCH_VERSION_OLD;
    if (check_patch(data, size, PATCH_VERSION_NEW)) return PATCH_VERSION_NEW;
    return PATCH_VERSION_UNKNOWN;
}

int WinMainCRTStartup () {
    DWORD exit_code = NO_ERROR;
    LPCWSTR exit_text = NULL;

    LPCWSTR filename = NULL;
    patch_version_t patch_version = PATCH_VERSION_NEW;

    HANDLE file = INVALID_HANDLE_VALUE;
    HANDLE mapping = NULL;
    LPVOID view_data = NULL;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == NULL) {
        exit_code = GetLastError();
        goto out;
    }
    for (int i = 1; i < argc; ++i) {
        LPCWSTR arg = argv[i];
        if (arg[0] == L'-' && arg[1] == L'r' && arg[2] == 0) {
            patch_version = PATCH_VERSION_OLD;
        } else if (arg[0] != L'-' && filename == NULL) {
            filename = arg;
        } else {
            exit_text = HELP_MSG;
            goto out;
        }
    }
    if (filename == NULL) {
        exit_text = HELP_MSG;
        goto out;
    }
    file = CreateFileW(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        exit_code = GetLastError();
        goto out;
    }
    LARGE_INTEGER file_size = { 0 };
    if (!GetFileSizeEx(file, &file_size)) {
        exit_code = GetLastError();
        goto out;
    }
    if (file_size.QuadPart != EXPECTED_FILE_SIZE) {
        exit_code = ERROR_INVALID_DATA;
        exit_text = INVALID_DATA_MSG;
        goto out;
    }
    SIZE_T view_size = (SIZE_T)file_size.QuadPart;
    mapping = CreateFileMappingW(file, NULL, PAGE_READWRITE, 0, 0, NULL);
    if (mapping == NULL) {
        exit_code = GetLastError();
        goto out;
    }
    view_data = MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, view_size);
    if (view_data == NULL) {
        exit_code = GetLastError();
        goto out;
    }
    patch_version_t current_version = check_file_version(view_data, view_size);
    if (current_version == PATCH_VERSION_UNKNOWN) {
        exit_code = ERROR_INVALID_DATA;
        exit_text = INVALID_DATA_MSG;
        goto out;
    }
    if (current_version != patch_version) {
        apply_patch(view_data, view_size, patch_version);
    }
    exit_text = patch_version == PATCH_VERSION_NEW ? PATCH_APPLIED_MSG : PATCH_REMOVED_MSG;

out:
    if (view_data != NULL) {
        FlushViewOfFile(view_data, 0);
        UnmapViewOfFile(view_data);
    }
    if (mapping != NULL) {
        CloseHandle(mapping);
    }
    if (file != INVALID_HANDLE_VALUE) {
        FlushFileBuffers(file);
        CloseHandle(file);
    }
    LPWSTR error_text = NULL;
    if (exit_text == NULL) {
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 
            NULL, exit_code, 
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
            (LPWSTR)&error_text, 0, NULL
        );
        exit_text = error_text;
    }
    MessageBoxW(
        NULL, exit_text, WINDOW_CAPTION, 
        MB_OK | (exit_code == NO_ERROR ? MB_ICONINFORMATION : MB_ICONERROR)
    );
    if (error_text) LocalFree(error_text);
    if (argv) LocalFree(argv);
    ExitProcess(exit_code);
    return exit_code;
}