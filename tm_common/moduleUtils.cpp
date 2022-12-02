#include <string.h>

#include <moduleUtils.h>

#if PSP_FW_VERSION == 100
typedef SceModule SCEMODULE_T;
#else
typedef SceModule2 SCEMODULE_T;
#endif

u32 GetModuleImportFuncAddr(char *moduleName, char *libName, u32 nid)
{
	SCEMODULE_T *mod = (SCEMODULE_T *)sceKernelFindModuleByName(moduleName);
	if (mod != NULL)
	{
		SceLibraryStubTable *libStubtable = reinterpret_cast<SceLibraryStubTable *>(mod->stub_top);
		int i;

		if (mod->stub_size <= 0)
			return 0;

		while ((u32)libStubtable < ((u32)mod->stub_top + mod->stub_size))
		{
			if (strcmp(libStubtable->libname, libName) == 0)
			{
				for (i = 0; i < libStubtable->stubcount; i++)
				{
					if (((int *)libStubtable->nidtable)[i] == nid)
					{
						return reinterpret_cast<u32>(&(reinterpret_cast<u32 *>(libStubtable->stubtable)[i*2]));
					}
				}
			}
			libStubtable = reinterpret_cast<SceLibraryStubTable *>(reinterpret_cast<u32>(libStubtable) + (libStubtable->len * 4));
		}
	}

	return 0;
}

u32 GetModuleExportFuncAddr(char *moduleName, char *libName, u32 nid)
{
	SCEMODULE_T *mod = (SCEMODULE_T *)sceKernelFindModuleByName(moduleName);
	if (mod != NULL)
	{
		SceLibraryEntryTable *libEntryTable = reinterpret_cast<SceLibraryEntryTable *>(mod->ent_top);
		int i;

		if (mod->ent_size <= 0)
			return 0;

		while ((u32)libEntryTable < ((u32)mod->ent_top + mod->ent_size))
		{
			if (strcmp(libEntryTable->libname, libName) == 0)
			{
				for (i = 0; i < libEntryTable->stubcount; i++)
				{
					if (((int *)libEntryTable->entrytable)[i] == nid)
					{
						return ((u32 *)libEntryTable->entrytable)[libEntryTable->stubcount + libEntryTable->vstubcount + i];
					}
				}
			}
			libEntryTable = reinterpret_cast<SceLibraryEntryTable *>(reinterpret_cast<u32>(libEntryTable) + (libEntryTable->len * 4));
		}
	}

	return 0;
}
