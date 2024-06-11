/*
CatLog
Copyright (C) 2024 Cat
Copyright (C) 2020 Princess of Sleeping
Copyright (C) 2020 Asakura Reiko

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, version 3 of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "catlog.h"

#include <string.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/net/net.h>
#include <psp2/registrymgr.h>
#include <psp2/vshbridge.h>
#include <taihen.h>

extern unsigned char _binary_network_settings_xml_start;
extern unsigned char _binary_network_settings_xml_size;

static CatLogConfig_t cfg;
static SceUID system_settings_core_modid = -1;

#define DECL_FUNC_HOOK(name, ...)                                                                      \
  static tai_hook_ref_t name##HookRef;                                                                 \
  static SceUID name##HookUid = -1;                                                                    \
  static int name##HookFunc(__VA_ARGS__)

#define BIND_FUNC_IMPORT_HOOK(name, module, lib_nid, func_nid)                                         \
  name##HookUid = taiHookFunctionImport(&name##HookRef, (module), (lib_nid), (func_nid),               \
                                                 (const void *)name##HookFunc)

#define UNBIND_FUNC_HOOK(name)                                                                         \
  ({                                                                                                   \
    if (name##HookUid > 0)                                                                             \
      taiHookRelease(name##HookUid, name##HookRef);                                                    \
  })

DECL_FUNC_HOOK(sceRegMgrGetKeyInt, const char *category, const char *name, int *value)
{
  if (sceClibStrncmp(category, "/CONFIG/CATLOG", 14) == 0)
  {
    if (value)
    {
      if (sceClibStrncmp(name, "level", 5) == 0)
      {
        *value = cfg.loglevel;
      }

      if (sceClibStrncmp(name, "port", 4) == 0)
      {
        *value = cfg.port;
      }

      if (sceClibStrncmp(name, "net", 3) == 0)
      {
        *value = cfg.net;
      }
    }
    return 0;
  }
  return TAI_CONTINUE(int, sceRegMgrGetKeyIntHookRef, category, name, value);
}

DECL_FUNC_HOOK(sceRegMgrGetKeyStr, const char *category, const char *name, char *value, int len)
{
  if (sceClibStrncmp(category, "/CONFIG/CATLOG", 14) == 0)
  {
    if (value && len)
    {
      if (sceClibStrncmp(name, "host", 4) == 0)
      {
        sceNetInetNtop(SCE_NET_AF_INET, &cfg.host, value, len);
        return 0;
      }
    }
    return 0;
  }
  return TAI_CONTINUE(int, sceRegMgrGetKeyStrHookRef, category, name, value, len);
}

DECL_FUNC_HOOK(sceRegMgrSetKeyInt, const char *category, const char *name, int value)
{
  if (sceClibStrncmp(category, "/CONFIG/CATLOG", 14) == 0)
  {
    if (sceClibStrncmp(name, "level", 5) == 0)
    {
      cfg.loglevel = value;
    }

    if (sceClibStrncmp(name, "port", 4) == 0)
    {
      cfg.port = value;
    }

    if (sceClibStrncmp(name, "net", 3) == 0)
    {
      cfg.net = value;
    }

    CatLogUpdateConfig(cfg.host, cfg.port, cfg.loglevel, cfg.net);

    return 0;
  }
  return TAI_CONTINUE(int, sceRegMgrSetKeyIntHookRef, category, name, value);
}

DECL_FUNC_HOOK(sceRegMgrSetKeyStr, const char *category, const char *name, char *value, const int len)
{
  if (sceClibStrncmp(category, "/CONFIG/CATLOG", 14) == 0)
  {
    if (sceClibStrncmp(name, "host", 4) == 0)
    {
      sceNetInetPton(SCE_NET_AF_INET, value, &cfg.host);
    }

    CatLogUpdateConfig(cfg.host, cfg.port, cfg.loglevel, cfg.net);
    return 0;
  }
  return TAI_CONTINUE(int, sceRegMgrSetKeyStrHookRef, category, name, value, len);
}

typedef struct {
  int size;
  const char *name;
  int type;
  int unk;
} SceRegMgrKeysInfo;

DECL_FUNC_HOOK(sceRegMgrGetKeysInfo, const char *category, SceRegMgrKeysInfo *info, int count)
{
  if (sceClibStrncmp(category, "/CONFIG/CATLOG", 14) == 0)
  {
    if (info)
    {
        if (sceClibStrncmp(info->name, "host", 4) == 0)
          info->type = 0x00100001; // type string
        else
          info->type = 0x00040000; // type integer
    }
    return 0;
  }
  return TAI_CONTINUE(int, sceRegMgrGetKeysInfoHookRef, category, info, count);
}

DECL_FUNC_HOOK(scePafMiscLoadXmlLayout, int a1, void *xml_buf, int xml_size, int a4)
{
  if (sceClibStrncmp(xml_buf+82, "network_settings_plugin", 23) == 0)
  {
    xml_buf = (void *)&_binary_network_settings_xml_start;
    xml_size = (int)&_binary_network_settings_xml_size;
  }
  return TAI_CONTINUE(int, scePafMiscLoadXmlLayoutHookRef, a1, xml_buf, xml_size, a4);
}

DECL_FUNC_HOOK(sceKernelLoadStartModule, char *path, SceSize args, void *argp, int flags, SceKernelLMOption *option, int *status)
{
  SceUID ret = TAI_CONTINUE(SceUID, sceKernelLoadStartModuleHookRef, path, args, argp, flags, option, status);
  if (ret >= 0 && sceClibStrncmp(path, "vs0:app/NPXS10015/system_settings_core.suprx", 44) == 0)
  {
    system_settings_core_modid = ret;

    BIND_FUNC_IMPORT_HOOK(scePafMiscLoadXmlLayout, "SceSettings", 0x3D643CE8, 0x19FE55A8);
    BIND_FUNC_IMPORT_HOOK(sceRegMgrGetKeysInfo, "SceSystemSettingsCore", 0xC436F916, 0x58421DD1);
    BIND_FUNC_IMPORT_HOOK(sceRegMgrGetKeyInt, "SceSystemSettingsCore", 0xC436F916, 0x16DDF3DC);
    BIND_FUNC_IMPORT_HOOK(sceRegMgrSetKeyInt, "SceSystemSettingsCore", 0xC436F916, 0xD72EA399);
    BIND_FUNC_IMPORT_HOOK(sceRegMgrGetKeyStr, "SceSystemSettingsCore", 0xC436F916, 0xE188382F);
    BIND_FUNC_IMPORT_HOOK(sceRegMgrSetKeyStr, "SceSystemSettingsCore", 0xC436F916, 0x41D320C5);
  }
  return ret;
}

DECL_FUNC_HOOK(sceKernelStopUnloadModule, SceUID modid, SceSize args, void *argp, int flags, SceKernelULMOption *option, int *status)
{
  if (modid == system_settings_core_modid)
  {
    system_settings_core_modid = -1;
    UNBIND_FUNC_HOOK(scePafMiscLoadXmlLayout);
    UNBIND_FUNC_HOOK(sceRegMgrGetKeysInfo);
    UNBIND_FUNC_HOOK(sceRegMgrGetKeyInt);
    UNBIND_FUNC_HOOK(sceRegMgrSetKeyInt);
    UNBIND_FUNC_HOOK(sceRegMgrGetKeyStr);
    UNBIND_FUNC_HOOK(sceRegMgrSetKeyStr);
  }
  return TAI_CONTINUE(int, sceKernelStopUnloadModuleHookRef, modid, args, argp, flags, option, status);
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args)
{
  int search_param[2];

  SceUID res = _vshKernelSearchModuleByName("CatLog", search_param);

  if(res <= 0)
  {
      return SCE_KERNEL_START_SUCCESS;
  }

  CatLogReadConfig(&cfg.host, &cfg.port, &cfg.loglevel, &cfg.net);

  BIND_FUNC_IMPORT_HOOK(sceKernelLoadStartModule, "SceSettings", 0xCAE9ACE6, 0x2DCC4AFA);
  BIND_FUNC_IMPORT_HOOK(sceKernelStopUnloadModule, "SceSettings", 0xCAE9ACE6, 0x2415F8A4);

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args)
{
  UNBIND_FUNC_HOOK(sceKernelLoadStartModule);
  UNBIND_FUNC_HOOK(sceKernelStopUnloadModule);
  UNBIND_FUNC_HOOK(scePafMiscLoadXmlLayout);
  UNBIND_FUNC_HOOK(sceRegMgrGetKeysInfo);
  UNBIND_FUNC_HOOK(sceRegMgrGetKeyInt);
  UNBIND_FUNC_HOOK(sceRegMgrSetKeyInt);
  UNBIND_FUNC_HOOK(sceRegMgrGetKeyStr);
  UNBIND_FUNC_HOOK(sceRegMgrSetKeyStr);

  return SCE_KERNEL_STOP_SUCCESS;
}
