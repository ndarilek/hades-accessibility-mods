#include "tolk_loader.h"
#include "logger.h"

// Tolk function signatures
using fn_Tolk_Load              = void(*)();
using fn_Tolk_Unload            = void(*)();
using fn_Tolk_Output            = bool(*)(const wchar_t*, bool);
using fn_Tolk_Silence           = void(*)();
using fn_Tolk_IsLoaded          = bool(*)();
using fn_Tolk_HasSpeech         = bool(*)();
using fn_Tolk_TrySAPI           = void(*)(bool);
using fn_Tolk_DetectScreenReader = const wchar_t*(*)();

static HMODULE s_tolkModule = nullptr;

static fn_Tolk_Load              s_Load              = nullptr;
static fn_Tolk_Unload            s_Unload            = nullptr;
static fn_Tolk_Output            s_Output            = nullptr;
static fn_Tolk_Silence           s_Silence           = nullptr;
static fn_Tolk_IsLoaded          s_IsLoaded          = nullptr;
static fn_Tolk_HasSpeech         s_HasSpeech         = nullptr;
static fn_Tolk_TrySAPI           s_TrySAPI           = nullptr;
static fn_Tolk_DetectScreenReader s_DetectScreenReader = nullptr;

static bool s_available = false;

namespace TolkLoader {

bool Init(const wchar_t* tolkDllPath)
{
    if (!tolkDllPath || !tolkDllPath[0]) {
        Log::Warn("No Tolk.dll path provided");
        return false;
    }

    s_tolkModule = LoadLibraryW(tolkDllPath);
    if (!s_tolkModule) {
        Log::Error("Failed to load Tolk.dll from: %ls (error %lu)", tolkDllPath, GetLastError());
        return false;
    }

    s_Load              = reinterpret_cast<fn_Tolk_Load>(GetProcAddress(s_tolkModule, "Tolk_Load"));
    s_Unload            = reinterpret_cast<fn_Tolk_Unload>(GetProcAddress(s_tolkModule, "Tolk_Unload"));
    s_Output            = reinterpret_cast<fn_Tolk_Output>(GetProcAddress(s_tolkModule, "Tolk_Output"));
    s_Silence           = reinterpret_cast<fn_Tolk_Silence>(GetProcAddress(s_tolkModule, "Tolk_Silence"));
    s_IsLoaded          = reinterpret_cast<fn_Tolk_IsLoaded>(GetProcAddress(s_tolkModule, "Tolk_IsLoaded"));
    s_HasSpeech         = reinterpret_cast<fn_Tolk_HasSpeech>(GetProcAddress(s_tolkModule, "Tolk_HasSpeech"));
    s_TrySAPI           = reinterpret_cast<fn_Tolk_TrySAPI>(GetProcAddress(s_tolkModule, "Tolk_TrySAPI"));
    s_DetectScreenReader = reinterpret_cast<fn_Tolk_DetectScreenReader>(GetProcAddress(s_tolkModule, "Tolk_DetectScreenReader"));

    if (!s_Load || !s_Unload || !s_Output || !s_Silence) {
        Log::Error("Failed to resolve required Tolk functions");
        FreeLibrary(s_tolkModule);
        s_tolkModule = nullptr;
        return false;
    }

    s_available = true;
    Log::Info("Tolk.dll loaded successfully");
    return true;
}

void Shutdown()
{
    if (s_available && s_Unload) {
        s_Unload();
    }
    if (s_tolkModule) {
        FreeLibrary(s_tolkModule);
        s_tolkModule = nullptr;
    }
    s_available = false;
}

bool IsAvailable() { return s_available; }

void Load()
{
    if (s_Load) {
        // Allow SAPI as a last-resort output so speech works where no
        // screen reader is running (e.g. Proton's built-in voices)
        if (s_TrySAPI) {
            s_TrySAPI(true);
        }
        s_Load();
        Log::Info("Tolk_Load called");

        const wchar_t* sr = DetectScreenReader();
        if (sr) {
            Log::Info("Screen reader detected: %ls", sr);
        } else {
            Log::Warn("No screen reader detected");
        }
    }
}

void Unload()
{
    if (s_Unload) s_Unload();
}

bool Output(const wchar_t* text, bool interrupt)
{
    if (s_Output) {
        Log::Info("[TOLK] interrupt=%d text=\"%ls\"", interrupt ? 1 : 0, text ? text : L"(null)");
        return s_Output(text, interrupt);
    }
    return false;
}

void Silence()
{
    if (s_Silence) s_Silence();
}

bool IsLoaded()
{
    if (s_IsLoaded) return s_IsLoaded();
    return false;
}

bool HasSpeech()
{
    if (s_HasSpeech) return s_HasSpeech();
    return false;
}

const wchar_t* DetectScreenReader()
{
    if (s_DetectScreenReader) return s_DetectScreenReader();
    return nullptr;
}

}
