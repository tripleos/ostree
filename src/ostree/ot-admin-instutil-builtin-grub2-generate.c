/*
 * Copyright (C) 2014 Colin Walters <walters@verbum.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the licence or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library. If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib-unix.h>
#include <string.h>

#include "ostree-cmd-private.h"
#include "ot-admin-instutil-builtins.h"

#include "otutil.h"

static GOptionEntry options[] = { { NULL } };

gboolean
ot_admin_instutil_builtin_grub2_generate (int argc, char **argv,
                                          OstreeCommandInvocation *invocation,
                                          GCancellable *cancellable, GError **error)
{

  g_autoptr (GOptionContext) context = g_option_context_new ("[BOOTVERSION]");
  g_autoptr (OstreeSysroot) sysroot = NULL;
  if (!ostree_admin_option_context_parse (context, options, &argc, &argv,
                                          OSTREE_ADMIN_BUILTIN_FLAG_SUPERUSER
                                              | OSTREE_ADMIN_BUILTIN_FLAG_UNLOCKED,
                                          invocation, &sysroot, cancellable, error))
    return FALSE;

  guint bootversion;
  if (argc >= 2)
    {
      bootversion = (guint)g_ascii_strtoull (argv[1], NULL, 10);
      if (!(bootversion == 0 || bootversion == 1))
        return glnx_throw (error, "Invalid bootversion: %u", bootversion);
    }
  else
    {
      const char *bootversion_env = g_getenv ("_OSTREE_GRUB2_BOOTVERSION");
      if (bootversion_env)
        bootversion = g_ascii_strtoull (bootversion_env, NULL, 10);
      else
        bootversion = ostree_sysroot_get_bootversion (sysroot);
      g_assert (bootversion == 0 || bootversion == 1);
    }

  return ostree_cmd__private__ ()->ostree_generate_grub2_config (sysroot, bootversion, 1,
                                                                 cancellable, error);
}
