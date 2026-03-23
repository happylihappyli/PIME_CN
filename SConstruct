import os
import re

env = Environment()

# Common flags
env.Append(CPPDEFINES=['UNICODE', '_UNICODE', 'WIN32', 'NOMINMAX', '_CRT_SECURE_NO_WARNINGS'])
env.Append(CCFLAGS=['/EHsc', '/W3', '/std:c++14', '/MT', '/utf-8'])

# Include paths
env.Append(CPPPATH=[
    'libIME2/src',
    'jsoncpp/include',
    'PIMETextService',
    '.'
])

# Read version
try:
    with open('version.txt', 'r') as f:
        version_str = f.read().strip()
        # Extract major, minor, patch
        parts = re.findall(r'\d+', version_str)
        if len(parts) >= 3:
            major, minor, patch = parts[0], parts[1], parts[2]
        else:
            major, minor, patch = '0', '0', '0'
except:
    major, minor, patch = '1', '0', '0'

env['MAJOR'] = major
env['MINOR'] = minor
env['PATCH'] = patch

# Builder to generate RC file
def generate_rc(target, source, env):
    with open(str(source[0]), 'r') as f:
        content = f.read()
    content = content.replace('@PIME_VERSION_MAJOR@', env['MAJOR'])
    content = content.replace('@PIME_VERSION_MINOR@', env['MINOR'])
    content = content.replace('@PIME_VERSION_PATCH@', env['PATCH'])
    with open(str(target[0]), 'w') as f:
        f.write(content)
    return 0

env.Append(BUILDERS={'GenerateRC': Builder(action=generate_rc)})

# -------------------------------------------------------------------------
# jsoncpp
# -------------------------------------------------------------------------
jsoncpp_src = [
    'jsoncpp/src/lib_json/json_reader.cpp',
    'jsoncpp/src/lib_json/json_value.cpp',
    'jsoncpp/src/lib_json/json_writer.cpp'
]
# Need to include src/lib_json for json_tool.h
env_json = env.Clone()
env_json.Append(CPPPATH=['jsoncpp/src/lib_json'])

jsoncpp_lib = env_json.StaticLibrary('build/jsoncpp', jsoncpp_src)

# -------------------------------------------------------------------------
# libIME2
# -------------------------------------------------------------------------
libime2_src = [
    'libIME2/src/ImeModule.cpp',
    'libIME2/src/libIME.cpp',
    'libIME2/src/TextService.cpp',
    'libIME2/src/KeyEvent.cpp',
    'libIME2/src/EditSession.cpp',
    'libIME2/src/DisplayAttributeInfo.cpp',
    'libIME2/src/DisplayAttributeInfoEnum.cpp',
    'libIME2/src/DisplayAttributeProvider.cpp',
    'libIME2/src/LangBarButton.cpp',
    'libIME2/src/Utils.cpp',
    'libIME2/src/DrawUtils.cpp',
    'libIME2/src/Window.cpp',
    'libIME2/src/ImeWindow.cpp',
    'libIME2/src/MessageWindow.cpp',
    'libIME2/src/CandidateWindow.cpp'
]

libime2_lib = env.StaticLibrary('build/libIME2', libime2_src)

# -------------------------------------------------------------------------
# PIMETextService
# -------------------------------------------------------------------------
# Generate RC
rc_target = env.GenerateRC('PIMETextService/PIMETextService.rc', 'PIMETextService/PIMETextService.rc.in')
res_target = env.RES(rc_target)

pime_ts_src = [
    'PIMETextService/PIMEImeModule.cpp',
    'PIMETextService/PIMETextService.cpp',
    'PIMETextService/PIMEClient.cpp',
    'PIMETextService/PIMELangBarButton.cpp',
    'PIMETextService/DllEntry.cpp',
    res_target[0],
    'PIMETextService/PIMETextService.def'
]

# Link with shlwapi.lib, etc.
env.Append(LIBS=['shlwapi', 'user32', 'gdi32', 'advapi32', 'ole32', 'oleaut32', 'uuid', 'imm32', 'shell32', 'Rpcrt4'])
env.Append(CPPPATH=['PIMELauncher'])

# PIMETextService is a DLL
# We link the static libs

# Compile Utils.cpp with env (for TextService) separately to avoid conflict with env_launcher
utils_ts_obj = env.StaticObject('PIMETextService/Utils_ts.obj', 'PIMELauncher/Utils.cpp')

pime_ts_dll = env.SharedLibrary('build/PIMETextService', pime_ts_src + utils_ts_obj, 
                                LIBS=env['LIBS'] + [jsoncpp_lib, libime2_lib])

Default(pime_ts_dll)

# -------------------------------------------------------------------------
# PIMELauncher
# -------------------------------------------------------------------------
# Build libuv
env_uv = env.Clone()
env_uv.Append(CPPPATH=['libuv/include', 'libuv/src'])
# Disable some warnings for libuv
env_uv.Append(CCFLAGS=['/wd4090', '/wd4996', '/wd4244', '/wd4267', '/wd4311', '/wd4312', '/wd4100'])
# Define required macros for libuv
env_uv.Append(CPPDEFINES=['_GNU_SOURCE', '_WIN32', 'WIN32', '_CRT_SECURE_NO_DEPRECATE', '_CRT_NONSTDC_NO_DEPRECATE'])

uv_src = Glob('libuv/src/*.c') + Glob('libuv/src/win/*.c')
uv_lib = env_uv.StaticLibrary('build/libuv', uv_src)

# Build PIMELauncher
env_launcher = env.Clone()
env_launcher.Append(CPPPATH=['libuv/include', 'spdlog-1.2.1/include', 'jsoncpp/include', 'PIMELauncher', '.'])
env_launcher.Append(CPPDEFINES=['SPDLOG_WCHAR_FILENAMES=1', 'SPDLOG_PREVENT_CHILD_FD=1', 'HAVE_UV_NAMED_PIPE', 'WIN32_LEAN_AND_MEAN', 'NOMINMAX'])

launcher_rc = env_launcher.GenerateRC('PIMELauncher/PIMELauncher.rc', 'PIMELauncher/version.rc.in')
launcher_res = env_launcher.RES(launcher_rc)

launcher_src = [
    'PIMELauncher/PIMELauncher.cpp',
    'PIMELauncher/PipeServer.cpp',
    'PIMELauncher/PipeClient.cpp',
    'PIMELauncher/PipeSecurity.cpp',
    'PIMELauncher/BackendServer.cpp',
    'PIMELauncher/Utils.cpp',
    'PIMELauncher/SettingsDialog.cpp',
    launcher_res[0]
]

# Link libraries
env_launcher.Append(LIBS=['Rpcrt4', 'shlwapi', 'user32', 'gdi32', 'advapi32', 'ole32', 'shell32', 'Ws2_32', 'Iphlpapi', 'Userenv', 'comdlg32', 'Psapi'])

launcher_exe = env_launcher.Program('build/PIMELauncher.exe', launcher_src, LIBS=env_launcher['LIBS'] + [jsoncpp_lib, uv_lib])

Default(launcher_exe)
